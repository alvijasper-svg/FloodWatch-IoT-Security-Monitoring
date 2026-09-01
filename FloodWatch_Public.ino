/*
 * FloodWatch IoT - Public / Sanitized Portfolio Firmware
 *
 * Target: Arduino UNO R4 WiFi
 * Sensors: analog water-level sensor, LM393 rain module, DHT22/AM2302
 * Outputs: 16x2 I2C LCD, status LED, active buzzer
 * Cloud: Blynk IoT, virtual pins V0-V10
 *
 * IMPORTANT
 * ---------
 * This public version is reconstructed from the documented final hardware mapping,
 * test evidence and a recovered earlier source revision. Credentials are intentionally
 * removed. Exact private/demo configuration (including Blynk event codes and any
 * deployment-specific hysteresis tuning) is not published here.
 */

#define BLYNK_TEMPLATE_ID "TMPL_REPLACE_WITH_YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "FloodWatch IoT"
#define BLYNK_PRINT Serial

#include <Wire.h>
#include <WiFiS3.h>
#include <BlynkSimpleWifi.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include "secrets.h"

// -----------------------------------------------------------------------------
// Final documented pin mapping
// -----------------------------------------------------------------------------
#define WATER_PIN   A0
#define RAIN_PIN    A1
#define DHT_PIN     2
#define LED_PIN     3
#define BUZZER_PIN  8

#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;

// -----------------------------------------------------------------------------
// Final documented water thresholds (raw ADC)
// -----------------------------------------------------------------------------
#define WATER_LOW_RAW       200
#define WATER_MEDIUM_RAW    390
#define WATER_CRITICAL_RAW  500

// Piecewise calibration points recovered from the implementation evidence.
#define WATER_DRY_RAW   5
#define WATER_1CM_RAW   315
#define WATER_2CM_RAW   430
#define WATER_3CM_RAW   470
#define WATER_FULL_RAW  515

// Rain thresholds recovered from the latest available source revision.
#define RAIN_DRY_RAW     550
#define RAIN_LOW_RAW     465
#define RAIN_MEDIUM_RAW  340

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------
int waterRaw = 0;
int previousWaterRaw = 0;
int rainRaw = 1023;
int rainPercent = 0;

float physicalWaterCm = 0.0f;
float scaledWaterM = 0.0f;
float temperatureC = 0.0f;
float humidityPct = 0.0f;

String floodStatus = "NO FLOOD";
String rainStatus = "NO RAIN / DRY";
String tamperReason = "NO";
String sensorIssueReason = "NO";

bool tamperLatched = false;
bool sensorIssueLatched = false;
bool startupCheckCompleted = false;
bool previousWaterAvailable = false;
int dhtFailCount = 0;

unsigned long lastSensorRead = 0;
unsigned long lastDhtRead = 0;
unsigned long lastLcdUpdate = 0;
unsigned long lastSerialPrint = 0;
unsigned long modeStartTime = 0;

const unsigned long SENSOR_INTERVAL_MS = 500;
const unsigned long DHT_INTERVAL_MS = 2000;      // DHT22 safe acquisition interval
const unsigned long CLOUD_INTERVAL_MS = 2000;    // documented cloud telemetry interval
const unsigned long LCD_INTERVAL_MS = 1000;
const unsigned long SERIAL_INTERVAL_MS = 1000;

int lcdPage = 0;

// -----------------------------------------------------------------------------
// Operating modes
// -----------------------------------------------------------------------------
enum AlertMode {
  MODE_SAFE,
  MODE_LOW_RAIN,
  MODE_MEDIUM_RAIN,
  MODE_HIGH_RAIN,
  MODE_LOW_FLOOD,
  MODE_MEDIUM_FLOOD,
  MODE_CRITICAL_FLOOD,
  MODE_DATA_TAMPERING,
  MODE_SENSOR_ISSUE
};

AlertMode currentMode = MODE_SAFE;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
void setBuzzer(bool on) {
  digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
}

String modeText() {
  switch (currentMode) {
    case MODE_SAFE:           return "SAFE";
    case MODE_LOW_RAIN:       return "LOW RAIN";
    case MODE_MEDIUM_RAIN:    return "MEDIUM RAIN";
    case MODE_HIGH_RAIN:      return "HIGH RAIN";
    case MODE_LOW_FLOOD:      return "LOW FLOOD";
    case MODE_MEDIUM_FLOOD:   return "MEDIUM FLOOD";
    case MODE_CRITICAL_FLOOD: return "CRITICAL FLOOD";
    case MODE_DATA_TAMPERING: return "DATA TAMPER";
    case MODE_SENSOR_ISSUE:   return "SENSOR ISSUE";
    default:                  return "UNKNOWN";
  }
}

void setSystemMode(AlertMode newMode) {
  if (newMode == currentMode) return;

  currentMode = newMode;
  modeStartTime = millis();
  digitalWrite(LED_PIN, LOW);
  setBuzzer(false);

  // Transition-aware logging: a mode message is produced only when the mode changes.
  Serial.print("MODE CHANGED: ");
  Serial.println(modeText());
}

// -----------------------------------------------------------------------------
// Sensor acquisition
// -----------------------------------------------------------------------------
void readWaterSensor() {
  if (previousWaterAvailable) {
    previousWaterRaw = waterRaw;
  }

  // 15-sample median filter, matching the documented final implementation.
  const int sampleCount = 15;
  int samples[sampleCount];

  for (int i = 0; i < sampleCount; i++) {
    samples[i] = analogRead(WATER_PIN);
    delay(10);
  }

  for (int i = 0; i < sampleCount - 1; i++) {
    for (int j = i + 1; j < sampleCount; j++) {
      if (samples[j] < samples[i]) {
        int tmp = samples[i];
        samples[i] = samples[j];
        samples[j] = tmp;
      }
    }
  }

  waterRaw = samples[sampleCount / 2];

  if (!previousWaterAvailable) {
    previousWaterRaw = waterRaw;
    previousWaterAvailable = true;
  }

  // Piecewise scaling to the 0.00-1.00 m prototype display range.
  if (waterRaw <= WATER_DRY_RAW) {
    physicalWaterCm = 0.0f;
    scaledWaterM = 0.0f;
  } else if (waterRaw <= WATER_1CM_RAW) {
    float r = (waterRaw - WATER_DRY_RAW) /
              (float)(WATER_1CM_RAW - WATER_DRY_RAW);
    physicalWaterCm = r;
    scaledWaterM = r * 0.40f;
  } else if (waterRaw <= WATER_2CM_RAW) {
    float r = (waterRaw - WATER_1CM_RAW) /
              (float)(WATER_2CM_RAW - WATER_1CM_RAW);
    physicalWaterCm = 1.0f + r;
    scaledWaterM = 0.40f + (r * 0.20f);
  } else if (waterRaw <= WATER_3CM_RAW) {
    float r = (waterRaw - WATER_2CM_RAW) /
              (float)(WATER_3CM_RAW - WATER_2CM_RAW);
    physicalWaterCm = 2.0f + r;
    scaledWaterM = 0.60f + (r * 0.20f);
  } else if (waterRaw <= WATER_FULL_RAW) {
    float r = (waterRaw - WATER_3CM_RAW) /
              (float)(WATER_FULL_RAW - WATER_3CM_RAW);
    physicalWaterCm = 3.0f + (r * 0.5f);
    scaledWaterM = 0.80f + (r * 0.20f);
  } else {
    physicalWaterCm = 3.5f;
    scaledWaterM = 1.0f;
  }

  physicalWaterCm = constrain(physicalWaterCm, 0.0f, 3.5f);
  scaledWaterM = constrain(scaledWaterM, 0.0f, 1.0f);

  if (waterRaw >= WATER_CRITICAL_RAW)      floodStatus = "CRITICAL FLOOD";
  else if (waterRaw >= WATER_MEDIUM_RAW)   floodStatus = "MEDIUM FLOOD";
  else if (waterRaw >= WATER_LOW_RAW)      floodStatus = "LOW FLOOD";
  else                                     floodStatus = "NO FLOOD";
}

void readRainSensor() {
  rainRaw = analogRead(RAIN_PIN);
  rainPercent = map(rainRaw, 1023, 0, 0, 100);
  rainPercent = constrain(rainPercent, 0, 100);

  if (rainRaw >= RAIN_DRY_RAW)             rainStatus = "NO RAIN / DRY";
  else if (rainRaw >= RAIN_LOW_RAW)        rainStatus = "LOW RAIN";
  else if (rainRaw >= RAIN_MEDIUM_RAW)     rainStatus = "MEDIUM RAIN";
  else                                     rainStatus = "HIGH RAIN";
}

void readDht22() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    dhtFailCount++;
    Serial.println("DHT22 ERROR: invalid sensor reading");

    // The documented final design separates repeated sensor failure from tampering.
    if (dhtFailCount >= 3) {
      sensorIssueLatched = true;
      sensorIssueReason = "DHT22 invalid/offline";
    }
    return; // retain last valid temperature/humidity values
  }

  dhtFailCount = 0;
  temperatureC = t;
  humidityPct = h;
}

// -----------------------------------------------------------------------------
// Reliability / integrity checks
// -----------------------------------------------------------------------------
bool detectSensorIssue() {
  if (sensorIssueLatched) return true;

  sensorIssueReason = "NO";
  if (dhtFailCount >= 3) {
    sensorIssueLatched = true;
    sensorIssueReason = "DHT22 invalid/offline";
    return true;
  }
  return false;
}

bool detectTampering() {
  if (tamperLatched) return true;

  tamperReason = "NO";

  // One-time startup consistency checks recovered from the implementation logic.
  if (!startupCheckCompleted) {
    startupCheckCompleted = true;

    if (waterRaw >= WATER_MEDIUM_RAW && rainRaw >= RAIN_DRY_RAW) {
      tamperReason = "Startup flood/no rain";
      tamperLatched = true;
      return true;
    }

    if (rainRaw < RAIN_LOW_RAW && waterRaw < WATER_LOW_RAW) {
      tamperReason = "Startup rain/no flood";
      tamperLatched = true;
      return true;
    }

    if (rainRaw < RAIN_LOW_RAW && waterRaw >= WATER_MEDIUM_RAW) {
      tamperReason = "Startup rain/flood";
      tamperLatched = true;
      return true;
    }

    return false;
  }

  // Documented latched anomaly rule: sudden water change >350 raw units.
  if (previousWaterAvailable && abs(waterRaw - previousWaterRaw) > 350) {
    tamperReason = "Sudden water jump";
    tamperLatched = true;
    return true;
  }

  return false;
}

void evaluateSystem() {
  // Priority is explicit so a diagnostic fault cannot be hidden by a normal flood label.
  if (detectSensorIssue())                     setSystemMode(MODE_SENSOR_ISSUE);
  else if (detectTampering())                  setSystemMode(MODE_DATA_TAMPERING);
  else if (waterRaw >= WATER_CRITICAL_RAW)     setSystemMode(MODE_CRITICAL_FLOOD);
  else if (waterRaw >= WATER_MEDIUM_RAW)       setSystemMode(MODE_MEDIUM_FLOOD);
  else if (waterRaw >= WATER_LOW_RAW)          setSystemMode(MODE_LOW_FLOOD);
  else if (rainRaw < RAIN_MEDIUM_RAW)          setSystemMode(MODE_HIGH_RAIN);
  else if (rainRaw < RAIN_LOW_RAW)             setSystemMode(MODE_MEDIUM_RAIN);
  else if (rainRaw < RAIN_DRY_RAW)             setSystemMode(MODE_LOW_RAIN);
  else                                         setSystemMode(MODE_SAFE);
}

// -----------------------------------------------------------------------------
// Blynk V0-V10 telemetry
// -----------------------------------------------------------------------------
void sendToBlynk() {
  if (!Blynk.connected()) return;

  Blynk.virtualWrite(V0, scaledWaterM);
  Blynk.virtualWrite(V1, rainPercent);
  Blynk.virtualWrite(V2, temperatureC);
  Blynk.virtualWrite(V3, humidityPct);
  Blynk.virtualWrite(V4, modeText());
  Blynk.virtualWrite(V5, tamperLatched ? "YES" : "NO");
  Blynk.virtualWrite(V6, sensorIssueLatched ? "YES" : "NO");
  Blynk.virtualWrite(V7, waterRaw);
  Blynk.virtualWrite(V8, rainRaw);
  Blynk.virtualWrite(V9, tamperReason);
  Blynk.virtualWrite(V10, sensorIssueReason);
}

// -----------------------------------------------------------------------------
// Local alert patterns
// -----------------------------------------------------------------------------
void runAlertPattern() {
  bool ledOn = false;
  bool buzzerOn = false;
  unsigned long elapsed = millis() - modeStartTime;

  switch (currentMode) {
    case MODE_SAFE:
      break;
    case MODE_LOW_RAIN:
      ledOn = true;
      break;
    case MODE_MEDIUM_RAIN:
      ledOn = (elapsed % 2000UL) < 1000UL;
      buzzerOn = (elapsed % 4000UL) < 250UL;
      break;
    case MODE_HIGH_RAIN:
      ledOn = (elapsed % 1000UL) < 500UL;
      buzzerOn = (elapsed % 2000UL) < 350UL;
      break;
    case MODE_LOW_FLOOD:
      ledOn = true;
      buzzerOn = (elapsed % 3000UL) < 300UL;
      break;
    case MODE_MEDIUM_FLOOD:
      ledOn = (elapsed % 1000UL) < 500UL;
      buzzerOn = (elapsed % 1000UL) < 500UL;
      break;
    case MODE_CRITICAL_FLOOD:
      ledOn = (elapsed % 300UL) < 150UL;
      buzzerOn = true;
      break;
    case MODE_DATA_TAMPERING:
      ledOn = (elapsed % 400UL) < 200UL;
      buzzerOn = (elapsed % 400UL) < 200UL;
      break;
    case MODE_SENSOR_ISSUE:
      ledOn = true;
      buzzerOn = (elapsed % 5000UL) < 1500UL;
      break;
  }

  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  setBuzzer(buzzerOn);
}

// -----------------------------------------------------------------------------
// Local display / diagnostics
// -----------------------------------------------------------------------------
void updateLCD() {
  lcd.clear();

  if (sensorIssueLatched) {
    lcd.setCursor(0, 0); lcd.print("SENSOR ISSUE");
    lcd.setCursor(0, 1); lcd.print("Check DHT22");
    return;
  }

  if (tamperLatched) {
    lcd.setCursor(0, 0); lcd.print("DATA TAMPER");
    lcd.setCursor(0, 1);
    if (tamperReason == "Sudden water jump") lcd.print("Water Jump");
    else if (tamperReason == "Startup flood/no rain") lcd.print("Flood/No Rain");
    else lcd.print("Check Sensors");
    return;
  }

  switch (lcdPage) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("Water:"); lcd.print(scaledWaterM, 2); lcd.print("m");
      lcd.setCursor(0, 1); lcd.print(floodStatus);
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print("Rain:"); lcd.print(rainPercent); lcd.print("%");
      lcd.setCursor(0, 1); lcd.print(rainStatus);
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print("T:"); lcd.print(temperatureC, 1); lcd.print("C");
      lcd.setCursor(0, 1); lcd.print("RH:"); lcd.print(humidityPct, 1); lcd.print("%");
      break;
    default:
      lcd.setCursor(0, 0); lcd.print("Mode:");
      lcd.setCursor(0, 1); lcd.print(modeText());
      break;
  }

  lcdPage = (lcdPage + 1) % 4;
}

void printStatus() {
  Serial.println("-----------------------------------");
  Serial.print("Water raw: "); Serial.println(waterRaw);
  Serial.print("Scaled water: "); Serial.print(scaledWaterM, 3); Serial.println(" m");
  Serial.print("Rain raw: "); Serial.println(rainRaw);
  Serial.print("Rain: "); Serial.print(rainPercent); Serial.println(" %");
  Serial.print("Temperature: "); Serial.print(temperatureC, 1); Serial.println(" C");
  Serial.print("Humidity: "); Serial.print(humidityPct, 1); Serial.println(" %");
  Serial.print("Mode: "); Serial.println(modeText());
  Serial.print("Tamper: "); Serial.print(tamperLatched ? "YES" : "NO");
  Serial.print(" / "); Serial.println(tamperReason);
  Serial.print("Sensor issue: "); Serial.print(sensorIssueLatched ? "YES" : "NO");
  Serial.print(" / "); Serial.println(sensorIssueReason);
  Serial.print("Blynk: "); Serial.println(Blynk.connected() ? "ONLINE" : "OFFLINE");
}

BLYNK_CONNECTED() {
  Serial.println("Blynk lifecycle: ONLINE");
}

// -----------------------------------------------------------------------------
// Setup / loop
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  setBuzzer(false);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("FloodWatch IoT");
  lcd.setCursor(0, 1); lcd.print("Connecting...");

  Blynk.begin(BLYNK_DEVICE_TOKEN, WIFI_SSID, WIFI_PASSWORD);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Blynk Connected");
  lcd.setCursor(0, 1); lcd.print("System Ready");
  delay(1200);

  modeStartTime = millis();
  timer.setInterval(CLOUD_INTERVAL_MS, sendToBlynk);

  Serial.println("FloodWatch IoT - sanitized public build started");
}

void loop() {
  Blynk.run();
  timer.run();

  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = now;
    readWaterSensor();
    readRainSensor();
    evaluateSystem();
  }

  if (now - lastDhtRead >= DHT_INTERVAL_MS) {
    lastDhtRead = now;
    readDht22();
    evaluateSystem();
  }

  if (now - lastLcdUpdate >= LCD_INTERVAL_MS) {
    lastLcdUpdate = now;
    updateLCD();
  }

  if (now - lastSerialPrint >= SERIAL_INTERVAL_MS) {
    lastSerialPrint = now;
    printStatus();
  }

  runAlertPattern();
}
