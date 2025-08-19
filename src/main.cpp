/*  FrostByte – ESP‑01S temperature‑controlled relay
 *  -------------------------------------------------
 *  * Uses an ESP‑01S (ESP‑8266) as an access point
 *  * Reads a DHT22 every 10 s, keeps the last 30 samples
 *  * Turns the relay on/off based on the average temperature
 *  * Serves a very small web page with the status
 */

#include <Arduino.h>
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
DHT dht(DHTPIN, DHTTYPE);

const int NUM_SAMPLES = 30;            // 30 readings @ 10 s → 300 s
float  tempSamples[NUM_SAMPLES] = {0}; // sample buffer
int    sampleCount = 0;                // number of samples in buffer

bool  relayState   = false;   // current state of the relay (false = OFF)
bool  starting     = true;    // first cycle – always switch on if needed
bool  error        = false;   // DHT read error

float avgTemp      = 0.0f;
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
}

/* ------------------------------------------------------------------
 *  LOOP – main logic (no delay, async server works!)
 * ------------------------------------------------------------------ */
void loop() {
  static unsigned long lastRead = 0;
  const unsigned long READ_INTERVAL = 10000;   // 10 s

  if (millis() - lastRead >= READ_INTERVAL) {
    lastRead = millis();

    float currentTemp = dht.readTemperature(); // C
    if (isnan(currentTemp)) {
      return;          // skip the rest – no new sample
    }

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
    } else if ((starting || relayState) && avgTemp < desiredTemperature - precisionTemperature) {
      relayState = false;
      digitalWrite(RELAY_PIN, HIGH);  // relay OFF
    }
    starting = false;
  }
}