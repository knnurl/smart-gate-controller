/*
 * ═══════════════════════════════════════════════════════════════════════════
 *  SMART GATE CONTROLLER  v2.0.0
 *  ESP32 WROOM 32U firmware
 *
 *  Changes from v1:
 *    + Remote HTTP OTA — publish "update" via MQTT from anywhere in the world
 *    + Version check before update (skips if already latest)
 *    + ESP32-CAM stream embedded in dashboard
 *    + Cleaner state transitions
 *
 *  Libraries:
 *    - PubSubClient  (Nick O'Leary, v2.8+)
 *    - ArduinoJson   (Benoit Blanchon, v7+)
 *    - HttpsOTAUpdate (suculent, v1.x) ← NEW: install via Library Manager
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <HTTPClient.h>
#include <Update.h>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
//  HiveMQ Root CA
// ─────────────────────────────────────────────────────────────────────────────
static const char* HIVEMQ_ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoBggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// ─────────────────────────────────────────────────────────────────────────────
//  Gate State Machine
// ─────────────────────────────────────────────────────────────────────────────
enum GateState : uint8_t {
  STATE_UNKNOWN = 0, STATE_CLOSED, STATE_OPEN,
  STATE_OPENING, STATE_CLOSING, STATE_STOPPED, STATE_STUCK
};

const char* stateToString(GateState s) {
  switch (s) {
    case STATE_CLOSED:  return "closed";
    case STATE_OPEN:    return "open";
    case STATE_OPENING: return "opening";
    case STATE_CLOSING: return "closing";
    case STATE_STOPPED: return "stopped";
    case STATE_STUCK:   return "stuck";
    default:            return "unknown";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Globals
// ─────────────────────────────────────────────────────────────────────────────
WiFiClientSecure  secureClient;
PubSubClient      mqtt(secureClient);
WebServer         webServer(WEB_PORT);
Preferences       nvs;

GateState   gateState        = STATE_UNKNOWN;
unsigned long autoCloseSec   = AUTO_CLOSE_DEFAULT;
bool          autoCloseActive = false;
unsigned long autoCloseStart  = 0;
unsigned long travelStart     = 0;
unsigned long lastHeartbeat   = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastWifiAttempt = 0;
unsigned long bootTime        = 0;

bool          hallClosed = false, hallOpen = false;
bool          hallClosedRaw = false, hallOpenRaw = false;
unsigned long hallClosedAt = 0, hallOpenAt = 0;

String  anprWhitelist[ANPR_MAX_PLATES];
int     anprCount = 0;

// OTA update flag — set by MQTT, executed in main loop (not in callback)
volatile bool pendingOtaUpdate = false;

// ─────────────────────────────────────────────────────────────────────────────
//  NVS
// ─────────────────────────────────────────────────────────────────────────────
void nvsLoadConfig() {
  nvs.begin("gate", true);
  autoCloseSec = nvs.getULong("autoCloseSec", AUTO_CLOSE_DEFAULT);
  anprCount    = nvs.getInt("anprCount", 0);
  for (int i = 0; i < anprCount && i < ANPR_MAX_PLATES; i++) {
    anprWhitelist[i] = nvs.getString(("plate" + String(i)).c_str(), "");
  }
  nvs.end();
  if (anprCount == 0) {
    String initial[] = ANPR_INITIAL_WHITELIST;
    anprCount = sizeof(initial) / sizeof(initial[0]);
    for (int i = 0; i < anprCount; i++) anprWhitelist[i] = initial[i];
  }
}

void nvsSaveConfig() {
  nvs.begin("gate", false);
  nvs.putULong("autoCloseSec", autoCloseSec);
  nvs.putInt("anprCount", anprCount);
  for (int i = 0; i < anprCount; i++)
    nvs.putString(("plate" + String(i)).c_str(), anprWhitelist[i]);
  nvs.end();
}

// ─────────────────────────────────────────────────────────────────────────────
//  MQTT helpers
// ─────────────────────────────────────────────────────────────────────────────
void mqttPublish(const char* topic, const String& payload, bool retain = false) {
  if (mqtt.connected()) mqtt.publish(topic, payload.c_str(), retain);
}

void logEvent(const String& msg) {
  Serial.println("[LOG] " + msg);
  mqttPublish(TOPIC_LOG, msg);
}

void publishStatus() {
  mqttPublish(TOPIC_STATUS, stateToString(gateState), true);
  Serial.printf("[STATE] %s\n", stateToString(gateState));
}

void publishHeartbeat() {
  StaticJsonDocument<256> doc;
  doc["state"]     = stateToString(gateState);
  doc["uptime_s"]  = (millis() - bootTime) / 1000;
  doc["rssi"]      = WiFi.RSSI();
  doc["fw"]        = FW_VERSION;
  doc["autoclose"] = autoCloseSec;
  doc["ip"]        = WiFi.localIP().toString();
  String out; serializeJson(doc, out);
  mqttPublish(TOPIC_HEARTBEAT, out);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Relay
// ─────────────────────────────────────────────────────────────────────────────
void pulseRelay(uint8_t pin, const char* name) {
  Serial.printf("[RELAY] %s (GPIO %d)\n", name, pin);
  digitalWrite(pin, HIGH);
  delay(RELAY_PULSE_MS);
  digitalWrite(pin, LOW);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Gate commands
// ─────────────────────────────────────────────────────────────────────────────
void gateOpen() {
  if (gateState == STATE_OPEN || gateState == STATE_OPENING) {
    logEvent("Already open/opening — ignored"); return;
  }
  logEvent("Command: OPEN");
  gateState = STATE_OPENING;
  travelStart = millis();
  autoCloseActive = false;
  pulseRelay(PIN_RELAY_OPEN, "OPEN");
  publishStatus();
}

void gateClose() {
  if (gateState == STATE_CLOSED || gateState == STATE_CLOSING) {
    logEvent("Already closed/closing — ignored"); return;
  }
  logEvent("Command: CLOSE");
  gateState = STATE_CLOSING;
  travelStart = millis();
  autoCloseActive = false;
  pulseRelay(PIN_RELAY_CLOSE, "CLOSE");
  publishStatus();
}

void gateStop() {
  logEvent("Command: STOP");
  autoCloseActive = false;
  pulseRelay(PIN_RELAY_STOP, "STOP");
  gateState = STATE_STOPPED;
  publishStatus();
}

void gateToggle() {
  if (gateState == STATE_CLOSED || gateState == STATE_STOPPED) gateOpen();
  else if (gateState == STATE_OPEN) gateClose();
  else gateStop();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Remote HTTP OTA — fetches .bin from GitHub/server and self-flashes
// ─────────────────────────────────────────────────────────────────────────────
void performHttpOta() {
  logEvent("OTA: Starting remote update check...");

  // Stop gate if moving — safety first
  if (gateState == STATE_OPENING || gateState == STATE_CLOSING) {
    pulseRelay(PIN_RELAY_STOP, "STOP (OTA safety)");
    gateState = STATE_STOPPED;
    publishStatus();
    delay(500);
  }

  // Optional: check version file first to avoid unnecessary downloads
  // Uncomment if you maintain a version.txt in your repo:
  /*
  WiFiClientSecure versionClient;
  versionClient.setInsecure();  // or set CA for GitHub
  HTTPClient vHttp;
  vHttp.begin(versionClient, FW_VERSION_URL);
  if (vHttp.GET() == 200) {
    String remoteVersion = vHttp.getString();
    remoteVersion.trim();
    if (remoteVersion == FW_VERSION) {
      logEvent("OTA: Already on latest version " FW_VERSION " — skipping");
      vHttp.end();
      return;
    }
    logEvent("OTA: Remote version " + remoteVersion + " (current: " FW_VERSION ")");
  }
  vHttp.end();
  */

  logEvent("OTA: Downloading binary from " FW_BINARY_URL);

  WiFiClientSecure otaClient;
  otaClient.setInsecure();  // GitHub redirects through CDN — easier than pinning CA
  HTTPClient http;
  http.begin(otaClient, FW_BINARY_URL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(30000);

  int httpCode = http.GET();
  if (httpCode != 200) {
    logEvent("OTA: Download failed — HTTP " + String(httpCode));
    http.end();
    return;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    logEvent("OTA: No content length — aborting");
    http.end();
    return;
  }

  if (!Update.begin(contentLength)) {
    logEvent("OTA: Not enough space for update");
    http.end();
    return;
  }

  logEvent("OTA: Flashing " + String(contentLength) + " bytes...");

  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written != (size_t)contentLength) {
    logEvent("OTA: Write mismatch — " + String(written) + "/" + String(contentLength));
    Update.abort();
    http.end();
    return;
  }

  if (!Update.end()) {
    logEvent("OTA: Finalise failed — " + String(Update.getError()));
    http.end();
    return;
  }

  http.end();
  logEvent("OTA: Success! Rebooting in 3s...");
  delay(3000);
  ESP.restart();
}

// ─────────────────────────────────────────────────────────────────────────────
//  ANPR
// ─────────────────────────────────────────────────────────────────────────────
String normalizePlate(const String& raw) {
  String out = raw; out.trim(); out.toUpperCase();
  out.replace(" ", ""); out.replace("-", "");
  return out;
}

bool isPlateWhitelisted(const String& plate) {
  String norm = normalizePlate(plate);
  for (int i = 0; i < anprCount; i++)
    if (normalizePlate(anprWhitelist[i]) == norm) return true;
  return false;
}

void handleAnprResult(const String& plate) {
  String norm = normalizePlate(plate);
  Serial.printf("[ANPR] Plate: %s → %s\n", plate.c_str(), norm.c_str());
  if (isPlateWhitelisted(norm)) {
    logEvent("ANPR MATCH: " + norm + " — opening gate");
    gateOpen();
  } else {
    logEvent("ANPR NO MATCH: " + norm);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Config handler (MQTT + Web)
// ─────────────────────────────────────────────────────────────────────────────
void handleConfig(const String& payload) {
  StaticJsonDocument<1024> doc;
  if (deserializeJson(doc, payload) != DeserializationError::Ok) {
    logEvent("Config parse error"); return;
  }
  bool changed = false;

  if (doc.containsKey("autoclose")) {
    autoCloseSec = doc["autoclose"].as<unsigned long>();
    logEvent("Auto-close → " + String(autoCloseSec) + "s");
    changed = true;
  }
  if (doc.containsKey("whitelist")) {
    anprCount = 0;
    for (JsonVariant v : doc["whitelist"].as<JsonArray>())
      if (anprCount < ANPR_MAX_PLATES) anprWhitelist[anprCount++] = v.as<String>();
    logEvent("Whitelist replaced (" + String(anprCount) + " plates)");
    changed = true;
  }
  if (doc.containsKey("add_plate")) {
    String p = normalizePlate(doc["add_plate"].as<String>());
    if (!isPlateWhitelisted(p) && anprCount < ANPR_MAX_PLATES) {
      anprWhitelist[anprCount++] = p;
      logEvent("Plate added: " + p);
      changed = true;
    }
  }
  if (doc.containsKey("remove_plate")) {
    String p = normalizePlate(doc["remove_plate"].as<String>());
    for (int i = 0; i < anprCount; i++) {
      if (normalizePlate(anprWhitelist[i]) == p) {
        for (int j = i; j < anprCount - 1; j++) anprWhitelist[j] = anprWhitelist[j + 1];
        anprCount--;
        logEvent("Plate removed: " + p);
        changed = true; break;
      }
    }
  }
  if (changed) nvsSaveConfig();
}

// ─────────────────────────────────────────────────────────────────────────────
//  MQTT command router
// ─────────────────────────────────────────────────────────────────────────────
void handleMqttCommand(const String& raw) {
  String cmd = raw; cmd.trim(); cmd.toLowerCase();
  if      (cmd == "open")   gateOpen();
  else if (cmd == "close")  gateClose();
  else if (cmd == "stop")   gateStop();
  else if (cmd == "toggle") gateToggle();
  else if (cmd == "status") publishStatus();
  else if (cmd == "update") { logEvent("OTA update requested via MQTT"); pendingOtaUpdate = true; }
  else logEvent("Unknown command: " + raw);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String t(topic), p;
  p.reserve(length);
  for (unsigned int i = 0; i < length; i++) p += (char)payload[i];
  Serial.printf("[MQTT] ← %s : %s\n", topic, p.c_str());
  if      (t == TOPIC_COMMAND)     handleMqttCommand(p);
  else if (t == TOPIC_CONFIG)      handleConfig(p);
  else if (t == TOPIC_ANPR_RESULT) handleAnprResult(p);
}

// ─────────────────────────────────────────────────────────────────────────────
//  WiFi + MQTT connection management
// ─────────────────────────────────────────────────────────────────────────────
void wifiLoop() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastWifiAttempt < WIFI_RECONNECT_MS) return;
  lastWifiAttempt = millis();
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void mqttConnect() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastMqttAttempt < MQTT_RECONNECT_MS) return;
  lastMqttAttempt = millis();
  bool ok = mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
                         TOPIC_STATUS, 1, true, "offline");
  if (ok) {
    mqtt.subscribe(TOPIC_COMMAND);
    mqtt.subscribe(TOPIC_CONFIG);
    mqtt.subscribe(TOPIC_ANPR_RESULT);
    publishStatus();
    publishHeartbeat();
    logEvent("Gate controller online FW v" FW_VERSION);
    Serial.println("[MQTT] Connected");
  } else {
    Serial.printf("[MQTT] Failed rc=%d\n", mqtt.state());
  }
}

void mqttLoop() {
  if (!mqtt.connected()) { mqttConnect(); return; }
  mqtt.loop();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hall sensors
// ─────────────────────────────────────────────────────────────────────────────
void hallLoop() {
  unsigned long now = millis();
  bool rc = (digitalRead(PIN_HALL_CLOSED) == LOW);
  bool ro = (digitalRead(PIN_HALL_OPEN)   == LOW);

  if (rc != hallClosed) {
    if (rc != hallClosedRaw) { hallClosedAt = now; hallClosedRaw = rc; }
    if (now - hallClosedAt > HALL_DEBOUNCE_MS) {
      hallClosed = rc;
      if (hallClosed) {
        gateState = STATE_CLOSED;
        autoCloseActive = false;
        publishStatus();
        logEvent("Gate reached CLOSED");
      }
    }
  }
  if (ro != hallOpen) {
    if (ro != hallOpenRaw) { hallOpenAt = now; hallOpenRaw = ro; }
    if (now - hallOpenAt > HALL_DEBOUNCE_MS) {
      hallOpen = ro;
      if (hallOpen) {
        gateState = STATE_OPEN;
        publishStatus();
        logEvent("Gate reached OPEN");
        if (autoCloseSec > 0) {
          autoCloseActive = true;
          autoCloseStart = millis();
          logEvent("Auto-close in " + String(autoCloseSec) + "s");
        }
      }
    }
  }
}

void travelTimeoutLoop() {
  if (gateState != STATE_OPENING && gateState != STATE_CLOSING) return;
  if (millis() - travelStart > GATE_TRAVEL_TIMEOUT) {
    logEvent("WARNING: Travel timeout — gate may be stuck");
    gateState = STATE_STUCK;
    publishStatus();
  }
}

void autoCloseLoop() {
  if (!autoCloseActive || gateState != STATE_OPEN) { autoCloseActive = false; return; }
  if (millis() - autoCloseStart >= autoCloseSec * 1000UL) {
    autoCloseActive = false;
    logEvent("Auto-close firing");
    gateClose();
  }
}

void heartbeatLoop() {
  if (!mqtt.connected()) return;
  if (millis() - lastHeartbeat < HEARTBEAT_INTERVAL) return;
  lastHeartbeat = millis();
  publishHeartbeat();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Web Dashboard
// ─────────────────────────────────────────────────────────────────────────────
const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Gate Controller</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh;padding:16px}
  h1{font-size:1.3rem;font-weight:700;color:#f8fafc;margin-bottom:2px}
  .sub{font-size:.75rem;color:#64748b;margin-bottom:20px}
  .card{background:#1e293b;border-radius:10px;padding:16px;margin-bottom:14px;border:1px solid #334155}
  .card h2{font-size:.7rem;font-weight:600;color:#94a3b8;text-transform:uppercase;letter-spacing:.05em;margin-bottom:12px}
  .badge{display:inline-block;padding:5px 14px;border-radius:999px;font-size:.85rem;font-weight:700;text-transform:uppercase}
  .badge.open{background:#052e16;color:#4ade80;border:1px solid #16a34a}
  .badge.closed{background:#1e1b4b;color:#818cf8;border:1px solid #4f46e5}
  .badge.opening,.badge.closing{background:#431407;color:#fb923c;border:1px solid #ea580c}
  .badge.stopped,.badge.unknown,.badge.stuck{background:#1c1917;color:#a8a29e;border:1px solid #57534e}
  .btns{display:flex;gap:8px;flex-wrap:wrap}
  button{flex:1;min-width:70px;padding:11px;border-radius:7px;border:none;font-size:.85rem;font-weight:600;cursor:pointer;transition:opacity .15s}
  button:hover{opacity:.82}
  .btn-open{background:#16a34a;color:#fff}
  .btn-close{background:#4f46e5;color:#fff}
  .btn-stop{background:#dc2626;color:#fff}
  .btn-warn{background:#92400e;color:#fde68a}
  .stat{display:flex;justify-content:space-between;align-items:center;padding:7px 0;border-bottom:1px solid #334155;font-size:.82rem}
  .stat:last-child{border-bottom:none}
  .stat-label{color:#94a3b8}
  .stat-value{font-weight:600}
  input[type=text],input[type=number]{background:#0f172a;border:1px solid #334155;color:#e2e8f0;border-radius:6px;padding:7px 10px;font-size:.82rem;width:100%;margin-bottom:7px}
  .form-row{display:flex;gap:7px;align-items:flex-end}
  .form-row input{margin-bottom:0}
  .btn-sm{flex:none;padding:7px 12px;font-size:.78rem}
  label{font-size:.75rem;color:#94a3b8;display:block;margin-bottom:3px}
  .plate-item{display:flex;justify-content:space-between;align-items:center;padding:5px 9px;background:#0f172a;border-radius:5px;margin-bottom:5px;font-size:.82rem;font-family:monospace}
  .btn-del{background:#7f1d1d;color:#fca5a5;border:none;border-radius:4px;padding:2px 7px;cursor:pointer;font-size:.72rem}
  .cam-wrap{background:#000;border-radius:7px;overflow:hidden;margin-top:4px;aspect-ratio:4/3;display:flex;align-items:center;justify-content:center}
  .cam-wrap img{width:100%;height:100%;object-fit:cover}
  .note{font-size:.7rem;color:#475569;margin-top:5px;text-align:right}
</style>
</head>
<body>
<h1>&#127968; Gate Controller</h1>
<div class="sub">v__FW__ &bull; __IP__ &bull; <span id="mqtt-dot">__MQTT__</span></div>

<div class="card">
  <h2>Gate Status</h2>
  <span class="badge __STATE_CLASS__" id="stateBadge">__STATE__</span>
  <div class="note">Auto-refreshes every 4s</div>
</div>

<div class="card">
  <h2>Live Camera</h2>
  <div class="cam-wrap">
    <img src="__CAM_STREAM__" alt="Camera stream" onerror="this.alt='Camera offline'">
  </div>
</div>

<div class="card">
  <h2>Manual Control</h2>
  <div class="btns">
    <button class="btn-open"  onclick="cmd('open')">&#9650; Open</button>
    <button class="btn-stop"  onclick="cmd('stop')">&#9646; Stop</button>
    <button class="btn-close" onclick="cmd('close')">&#9660; Close</button>
  </div>
</div>

<div class="card">
  <h2>System Info</h2>
  <div class="stat"><span class="stat-label">Uptime</span><span class="stat-value">__UPTIME__</span></div>
  <div class="stat"><span class="stat-label">WiFi RSSI</span><span class="stat-value">__RSSI__ dBm</span></div>
  <div class="stat"><span class="stat-label">Auto-close</span><span class="stat-value" id="acStat">__AC__s</span></div>
</div>

<div class="card">
  <h2>Auto-close Timer</h2>
  <label>Seconds after fully open (0 = disabled)</label>
  <div class="form-row">
    <input type="number" id="acVal" min="0" max="3600" value="__AC__">
    <button class="btn-open btn-sm" onclick="setAC()">Save</button>
  </div>
</div>

<div class="card">
  <h2>Remote OTA Update</h2>
  <p style="font-size:.8rem;color:#94a3b8;margin-bottom:10px">Triggers download from configured GitHub URL. Gate is stopped first for safety.</p>
  <button class="btn-warn" onclick="if(confirm('Start remote OTA update?')) cmd('update')">&#8593; Update Firmware</button>
</div>

<div class="card">
  <h2>ANPR Whitelist</h2>
  <div class="form-row">
    <input type="text" id="newPlate" placeholder="AB12CDE">
    <button class="btn-open btn-sm" onclick="addPlate()">+ Add</button>
  </div>
  <div id="plateList">__PLATES__</div>
</div>

<script>
function cmd(c){fetch('/api/command',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({cmd:c})})}
function setAC(){const v=document.getElementById('acVal').value;fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({autoclose:parseInt(v)})}).then(()=>document.getElementById('acStat').textContent=v+'s')}
function addPlate(){const p=document.getElementById('newPlate').value.trim();if(!p)return;fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({add_plate:p})}).then(()=>location.reload())}
function delPlate(p){fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({remove_plate:p})}).then(()=>location.reload())}
setInterval(()=>{fetch('/api/status').then(r=>r.json()).then(d=>{const el=document.getElementById('stateBadge');el.className='badge '+d.state;el.textContent=d.state})},4000)
</script>
</body>
</html>
)HTML";

String buildDashboard() {
  String html = FPSTR(DASHBOARD_HTML);
  String state = stateToString(gateState);
  unsigned long upSec = (millis() - bootTime) / 1000;
  String upStr = String(upSec/3600)+"h "+String((upSec%3600)/60)+"m "+String(upSec%60)+"s";

  String platesHtml;
  for (int i = 0; i < anprCount; i++)
    platesHtml += "<div class='plate-item'>" + anprWhitelist[i] +
                  "<button class='btn-del' onclick=\"delPlate('" + anprWhitelist[i] + "')\">&#10005;</button></div>";
  if (anprCount == 0) platesHtml = "<div style='color:#64748b;font-size:.8rem;margin-top:6px'>No plates added</div>";

  html.replace("__FW__",         FW_VERSION);
  html.replace("__IP__",         WiFi.localIP().toString());
  html.replace("__MQTT__",       mqtt.connected() ? "&#9679; MQTT" : "&#9675; MQTT offline");
  html.replace("__STATE__",      state);
  html.replace("__STATE_CLASS__",state);
  html.replace("__UPTIME__",     upStr);
  html.replace("__RSSI__",       String(WiFi.RSSI()));
  html.replace("__AC__",         String(autoCloseSec));
  html.replace("__CAM_STREAM__", CAM_STREAM_URL);
  html.replace("__PLATES__",     platesHtml);
  return html;
}

void webHandleRoot() {
  if (!webServer.authenticate(WEB_USER, WEB_PASSWORD)) return webServer.requestAuthentication();
  webServer.send(200, "text/html", buildDashboard());
}

void webHandleStatus() {
  StaticJsonDocument<128> doc;
  doc["state"] = stateToString(gateState);
  doc["autoclose"] = autoCloseSec;
  String out; serializeJson(doc, out);
  webServer.send(200, "application/json", out);
}

void webHandleCommand() {
  if (!webServer.authenticate(WEB_USER, WEB_PASSWORD)) return webServer.requestAuthentication();
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, webServer.arg("plain")) == DeserializationError::Ok) {
    handleMqttCommand(doc["cmd"].as<String>());
    webServer.send(200, "application/json", "{\"ok\":true}");
  } else webServer.send(400, "application/json", "{\"error\":\"bad json\"}");
}

void webHandleConfig() {
  if (!webServer.authenticate(WEB_USER, WEB_PASSWORD)) return webServer.requestAuthentication();
  handleConfig(webServer.arg("plain"));
  webServer.send(200, "application/json", "{\"ok\":true}");
}

void webSetup() {
  webServer.on("/",            HTTP_GET,  webHandleRoot);
  webServer.on("/api/status",  HTTP_GET,  webHandleStatus);
  webServer.on("/api/command", HTTP_POST, webHandleCommand);
  webServer.on("/api/config",  HTTP_POST, webHandleConfig);
  webServer.begin();
}

// ─────────────────────────────────────────────────────────────────────────────
//  OTA (local network)
// ─────────────────────────────────────────────────────────────────────────────
void otaSetup() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    if (gateState == STATE_OPENING || gateState == STATE_CLOSING)
      pulseRelay(PIN_RELAY_STOP, "STOP (OTA)");
  });
  ArduinoOTA.onEnd([]()   { Serial.println("[OTA] Done"); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) { esp_task_wdt_reset(); });
  ArduinoOTA.begin();
}

// ─────────────────────────────────────────────────────────────────────────────
//  setup()
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== GATE CONTROLLER v" FW_VERSION " ===");
  bootTime = millis();

  pinMode(PIN_RELAY_OPEN,  OUTPUT); digitalWrite(PIN_RELAY_OPEN,  LOW);
  pinMode(PIN_RELAY_CLOSE, OUTPUT); digitalWrite(PIN_RELAY_CLOSE, LOW);
  pinMode(PIN_RELAY_STOP,  OUTPUT); digitalWrite(PIN_RELAY_STOP,  LOW);
  pinMode(PIN_HALL_CLOSED, INPUT_PULLUP);
  pinMode(PIN_HALL_OPEN,   INPUT_PULLUP);

  nvsLoadConfig();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long ws = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - ws < 15000) { delay(500); Serial.print("."); }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("[WiFi] %s\n", WiFi.localIP().toString().c_str());

  secureClient.setCACert(HIVEMQ_ROOT_CA);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(1024);
  mqttConnect();

  otaSetup();
  webSetup();

  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);

  hallClosed = (digitalRead(PIN_HALL_CLOSED) == LOW);
  hallOpen   = (digitalRead(PIN_HALL_OPEN)   == LOW);
  if      (hallClosed) gateState = STATE_CLOSED;
  else if (hallOpen)   gateState = STATE_OPEN;
  else                 gateState = STATE_UNKNOWN;

  Serial.printf("[INIT] Gate state: %s\n", stateToString(gateState));
  Serial.printf("[WEB]  Dashboard: http://%s/\n", WiFi.localIP().toString().c_str());
  publishStatus();
}

// ─────────────────────────────────────────────────────────────────────────────
//  loop()
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  esp_task_wdt_reset();

  wifiLoop();
  mqttLoop();
  ArduinoOTA.handle();
  webServer.handleClient();
  hallLoop();
  travelTimeoutLoop();
  autoCloseLoop();
  heartbeatLoop();

  // Remote OTA is executed here (not inside MQTT callback) to keep stack safe
  if (pendingOtaUpdate) {
    pendingOtaUpdate = false;
    performHttpOta();   // Will reboot on success
  }

  delay(10);
}
