#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h> // or #include <WiFi.h> for ESP32
#include <DHT.h> // for the temperature sensor

const char* ssid     = "FrostByte";
const char* password = "fridgelord";

// fixed to cooler controller mode (switch < and > around below to turn in a thermostat)
float desiredTemperature = 6.0f; // celsius
float precisionTemperature = 1.0f; // celsius, i.e.  + and - range

#define DHTPIN 2      // GPIO2
#define DHTTYPE DHT22   // DHT22 = AM2302/M2302
#define RELAY_PIN 0   // GPIO0

const char header_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>FrostByte</title>
  <style>
    :root {
      --bg-color: #000000;
      --text-color: #ffffff;
      --accent-color: #00d0ff; /* optioneel: koel tech-accent */
      --font-family: 'Segoe UI', Roboto, sans-serif;
    }

    html {
      display: inline-block;
      margin: 0px auto;
      text-align: center;
    }

    .input {
      font-size: 20px;
      margin-bottom: 10px;
    }

    body {
      margin: 0;
      padding: 0;
      background-color: var(--bg-color);
      color: var(--text-color);
      font-family: var(--font-family);
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
    }

    h1 {
      font-size: 3rem;
      margin-bottom: 0.5rem;
    }

    p {
      font-size: 1.2rem;
      max-width: 600px;
      text-align: center;
      line-height: 1.6;
      opacity: 0.8;
    }

    .logo {
      max-width: 150px;
      margin-bottom: 1.5rem;
    }

    a.button {
      margin-top: 2rem;
      padding: 0.75rem 1.5rem;
      background-color: var(--accent-color);
      color: #000;
      text-decoration: none;
      font-weight: bold;
      border-radius: 6px;
      transition: background-color 0.3s ease;
    }

    a.button:hover {
      background-color: #00b4dc;
    }

    .wifi-form {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: space-around;
    }
  </style>
</head>
<body>
  <h2>FrostByte</h2>
  <p>Keeping It Cool, One Byte at a Time!<br />Smart cooler controller</p>)rawliteral";

const char footer_html[] PROGMEM = R"rawliteral(</html>)rawliteral";

AsyncWebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

const int NUM_SAMPLES = 30;          // 30 samples (every 2s = 1 minute)
float tempSamples[NUM_SAMPLES];      // buffer for samples
int sampleIndex = 0;
bool bufferFilled = false;
bool relayState = false;             // current state of the relay
bool starting = true;
float avgTemp = 0.0f;
unsigned long lastSwitched = millis();
bool error = false;

// put function declarations here:
void createAccessPoint();
char* buildPage(String);
void respondPage(AsyncWebServerRequest *, String);
String msToDhms(unsigned long ms);

// put your setup code here, to run once:
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // relais off

  dht.begin();

  createAccessPoint();
}

void loop() {

  float currentTemp = dht.readTemperature();

  if (!isnan(currentTemp)) {
    error = false;

    tempSamples[sampleIndex] = currentTemp;
    sampleIndex++;

    if (sampleIndex >= NUM_SAMPLES) {
      sampleIndex = 0;
      bufferFilled = true;
    }

    // calculate average temperature
    float sum = 0;
    int count = bufferFilled ? NUM_SAMPLES : sampleIndex;

    for (int i = 0; i < count; i++) {
      sum += tempSamples[i];
    }

    avgTemp = sum / count;

    // hysteresis logic
    if ((starting || !relayState) && avgTemp > desiredTemperature + precisionTemperature) {
      relayState = true;
      digitalWrite(RELAY_PIN, LOW);  // Relay ON
      lastSwitched = millis();
    } else if ((starting || relayState) && avgTemp < desiredTemperature - precisionTemperature) {
      relayState = false;
      digitalWrite(RELAY_PIN, HIGH); // Relay OFF
      lastSwitched = millis();
    }
    starting = false;
  } else {

    error = true;
  }

  delay(2000);  // avoid to fast repetition
}

double temperature = 20.4;

// put function definitions here:
void createAccessPoint() {
  WiFi.softAP(ssid, password);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String state;
    if (!error)
    {
      state = "<h3>Temperature: " + String(avgTemp, 2) + " °C</h3><br/>";
      if (relayState) {
        state += "Relay: ON<br/>";
        state += "Last switched ON " + msToDhms(millis() - lastSwitched) + " ago.<br/>";
      } else {
        state += "Relay: OFF<br/>";
        state += "Last switched OFF " + msToDhms(millis() - lastSwitched) + " ago.<br/>";
      }
      state += "Switches back ON when " + String(desiredTemperature + precisionTemperature, 2) + " °C is reached.<br/>";
      state += "Switches back OFF when " + String(desiredTemperature - precisionTemperature, 2) + " °C is reached.<br/>";
    }
    else
        state = "Error reading temperature<br/>";
    respondPage(request, state);
  });
  server.begin();
}

char* buildPage(String content) {
  // Load PROGMEM strings into temporary Strings
  String header((__FlashStringHelper*)header_html);
  String footer((__FlashStringHelper*)footer_html);

  // Combine all parts
  String fullPage = header + content + footer;

  // Allocate a char array to return (make sure it's long enough)
  char* result = new char[fullPage.length() + 1]; // +1 for null terminator
  fullPage.toCharArray(result, fullPage.length() + 1);

  return result;  // Remember: caller is responsible for deleting this (otherwise memory leak)
}

void respondPage(AsyncWebServerRequest *request, String content) {
    char* page = buildPage(content);
    request->send_P(200, "text/html", page);
    delete[] page;
}

String msToDhms(unsigned long ms) {
  const unsigned long MS_PER_DAY   = 86400000UL;
  const unsigned long MS_PER_HOUR  = 3600000UL;
  const unsigned long long MS_PER_MIN   = 60000UL;
  const unsigned MS_PER_SEC   = 1000UL;

  unsigned long days    = ms / MS_PER_DAY;      ms %= MS_PER_DAY;
  unsigned long hours   = ms / MS_PER_HOUR;     ms %= MS_PER_HOUR;
  unsigned long minutes = ms / MS_PER_MIN;      ms %= MS_PER_MIN;
  unsigned long seconds = ms / MS_PER_SEC;      // remaining ms are ignored

  // Build the string.  Using String concatenation is fine for small
  // programs; for larger code bases you might prefer a fixed‑size
  // char buffer and snprintf().
  String out;
  out.reserve(30);            // avoid reallocations

  out += String(days)   + "d ";
  out += String(hours)  + "h ";
  out += String(minutes)+ "m ";
  out += String(seconds)+"s";

  return out;
}