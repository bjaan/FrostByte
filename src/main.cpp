/*  FrostByte – ESP‑01S temperature‑controlled relay
 *  -------------------------------------------------
 *  * Uses an ESP‑01S (ESP‑8266) as an access point
 *  * Reads a DHT22 every 2 s, keeps the last 30 samples
 *  * Turns the relay on/off based on the average temperature
 *  * Serves a very small web page with the status
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

/* ------------------------------------------------------------------
 *  CONFIGURATION – change these if you want a different SSID / password
 * ------------------------------------------------------------------ */
const char *ssid     = "FrostByte";
const char *password = "fridgelord";

const float desiredTemperature   = 6.0f;   // °C  (cooler‑controller mode)
const float precisionTemperature = 1.0f;   // ± 1 °C hysteresis

/* ------------------------------------------------------------------
 *  HARDWARE DEFINITIONS
 * ------------------------------------------------------------------ */
#define DHTPIN     2          // GPIO2
#define DHTTYPE    DHT22
#define RELAY_PIN  0          // GPIO0

/* ------------------------------------------------------------------
 *  GLOBALS
 * ------------------------------------------------------------------ */
AsyncWebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

const int NUM_SAMPLES = 30;            // 30 readings @ 2 s → 60 s
float  tempSamples[NUM_SAMPLES] = {0}; // sample buffer
int    sampleCount = 0;                // number of samples in buffer

bool  relayState   = false;   // current state of the relay (false = OFF)
bool  starting     = true;    // first cycle – always switch on if needed
bool  error        = false;   // DHT read error

float avgTemp      = 0.0f;
unsigned long lastSwitched = 0;   // time of the last state change

/* ------------------------------------------------------------------
 *  PROGMEM HTML – split into header/footer to keep the page small
 * ------------------------------------------------------------------ */
const char header_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>FrostByte</title>
  <style>
    body{background:#000;color:#fff;font-family:Arial,Helvetica,sans-serif;
         text-align:center;margin:0;padding:0;}
    h2{margin-top:1rem;}
    .status{font-size:1.3rem;margin:1rem;}
  </style>
</head>
<body>
<h2>FrostByte</h2>
)rawliteral";

const char footer_html[] PROGMEM = R"rawliteral(
</body>
</html>)rawliteral";

/* ------------------------------------------------------------------
 *  FUNCTION PROTOTYPES
 * ------------------------------------------------------------------ */
void createAccessPoint();
String buildPage(const String &content);
String msToDhms(unsigned long ms);

/* ------------------------------------------------------------------
 *  SETUP
 * ------------------------------------------------------------------ */
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);          // relay OFF (active LOW)

  dht.begin();

  createAccessPoint();
}

/* ------------------------------------------------------------------
 *  LOOP – main logic (no delay, async server works!)
 * ------------------------------------------------------------------ */
void loop() {
  static unsigned long lastRead = 0;
  const unsigned long READ_INTERVAL = 2000;   // 2 s

  if (millis() - lastRead >= READ_INTERVAL) {
    lastRead = millis();

    float currentTemp = dht.readTemperature(); // C
    if (isnan(currentTemp)) {
      error = true;
      return;          // skip the rest – no new sample
    }
    error = false;

    /*  store sample in circular buffer  */
    if (sampleCount < NUM_SAMPLES) {
        tempSamples[sampleCount++] = currentTemp;            // first 30 samples
    } else {
        // shift everything left by one
        memmove(tempSamples, tempSamples + 1, sizeof(float) * (NUM_SAMPLES - 1));
        tempSamples[NUM_SAMPLES - 1] = currentTemp;   // newest sample at the end
    }

    /*  average calculation  */
    float sum = 0;
    for (int i = 0; i < NUM_SAMPLES; ++i) sum += tempSamples[i];
    avgTemp = sum / NUM_SAMPLES;   // after the first 30 samples this is the true average

    /*  hysteresis – turn relay on/off  */
    if ((starting || !relayState) && avgTemp > desiredTemperature + precisionTemperature) {
      relayState = true;
      digitalWrite(RELAY_PIN, LOW);   // relay ON (active LOW)
      lastSwitched = millis();
    } else if ((starting || relayState) && avgTemp < desiredTemperature - precisionTemperature) {
      relayState = false;
      digitalWrite(RELAY_PIN, HIGH);  // relay OFF
      lastSwitched = millis();
    }
    starting = false;
  }
}

/* ------------------------------------------------------------------
 *  ACCESS POINT + WEB SERVER
 * ------------------------------------------------------------------ */
void createAccessPoint() {
  WiFi.softAP(ssid, password);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String state;
    if (!error) {
      state = "<div class='status'>Temperature: ";
      state += String(avgTemp, 2) + " °C</div>";

      state += "<div class='status'>Relay: ";
      state += relayState ? "ON" : "OFF";
      state += "</div>";

      state += "<div class='status'>Last switched ";
      state += relayState ? "ON" : "OFF";
      state += " " + msToDhms(millis() - lastSwitched) + " ago.</div>";

      state += "<div class='status'>Hysteresis zone: ";
      state += String(desiredTemperature - precisionTemperature, 2) + "–";
      state += String(desiredTemperature + precisionTemperature, 2) + " °C</div>";
    } else {
      state = "<div class='status'>Error reading temperature</div>";
    }
    request->send(200, "text/html", buildPage(state));
  });

  server.begin();
}

/* ------------------------------------------------------------------
 *  PAGE CONSTRUCTION
 * ------------------------------------------------------------------ */
String buildPage(const String &content) {
  /*  header/footer are stored in PROGMEM – we read them into
   *  temporary String objects.  The final page is a single
   *  heap string that we return by value (no manual delete needed).
   */
  String header((__FlashStringHelper*)header_html);
  String footer((__FlashStringHelper*)footer_html);

  return header + content + footer;
}

/* ------------------------------------------------------------------
 *  HELPER – milliseconds → “Xd Xh Xm Xs”
 * ------------------------------------------------------------------ */
String msToDhms(unsigned long ms) {
  const unsigned long MS_PER_DAY   = 86400000UL;
  const unsigned long MS_PER_HOUR  = 3600000UL;
  const unsigned long MS_PER_MIN   = 60000UL;
  const unsigned long MS_PER_SEC   = 1000UL;

  unsigned long days    = ms / MS_PER_DAY;      ms %= MS_PER_DAY;
  unsigned long hours   = ms / MS_PER_HOUR;     ms %= MS_PER_HOUR;
  unsigned long minutes = ms / MS_PER_MIN;      ms %= MS_PER_MIN;
  unsigned long seconds = ms / MS_PER_SEC;      // remaining ms ignored

  String out;
  out.reserve(30);
  out += String(days)   + "d ";
  out += String(hours)  + "h ";
  out += String(minutes)+ "m ";
  out += String(seconds)+"s";
  return out;
}