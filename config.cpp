#include "config.h"
#include <DueFlashStorage.h>

// GSM CONFIGURATION
const char *APN = "safaricom";
const char *BACKEND_URL = "aqmrg.pythonanywhere.com";
const int BACKEND_PORT = 80;
const char *BACKEND_PATH = "/api/v1/data/ingest";
const char *PROTOCOL = "http://";

const char *MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char *DEVICE_ID = "NODE001";

const char *TOPIC_DATA = "node/NODE001/data";
const char *TOPIC_CALIBRATION = "node/NODE001/calibration";
const char *TOPIC_STATUS = "node/NODE001/status";

unsigned long SEND_INTERVAL     = 60000;  // Telemetry every 60 seconds
const unsigned long CAL_CHECK_INTERVAL = 5000;   // Check for new calibration every 5 seconds
unsigned long lastSendTime          = 0;
unsigned long lastCalibrationCheck  = 0;
bool gsmReady = false;
uint32_t lastAppliedVersion = 0;  // Version of the last calibration the board applied

// MQ Calibration Constants
const float MQ7_R0 = 5.0;
const float MQ131_R0 = 10.0;
const float RL_VALUE = 10.0; // Load resistor in kOhms

// Environmental Offsets
float TEMP_OFFSET = 0.0;  // degrees C  — dummy: 0 means no shift
float HUM_OFFSET  = 0.0;  // % RH        — dummy: 0 means no shift

// -------------------------------------------------------
// Two-point linear calibration constants
// Formula applied in sensors.cpp:
//   calibrated = gain * raw_value + offset
//
// HOW TO FIND REAL VALUES (two reference points a & b):
//   gain   = (ref_b - ref_a) / (raw_b - raw_a)
//   offset = ref_a - gain * raw_a
//
// Until you have reference data, gain=1.0 / offset=0.0
// gives a perfect pass-through (no correction applied).
// -------------------------------------------------------

// MQ-7 CO  [dummy]
float CO_GAIN   = 1.0;   // scale factor  (replace after ref comparison)
float CO_OFFSET = 0.0;   // ppm shift     (replace after ref comparison)

// MQ-131 O3  [dummy]
float O3_GAIN   = 1.0;   // scale factor
float O3_OFFSET = 0.0;   // ppm shift

// MH-Z19C CO2  [dummy]
float CO2_GAIN   = 1.0;  // scale factor
float CO2_OFFSET = 0.0;  // ppm shift

// PMS5003 PM2.5  [dummy]
float PM25_GAIN   = 1.0; // scale factor
float PM25_OFFSET = 0.0; // ug/m3 shift

uint16_t conditioning_s = 10;

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.startsWith("CAL:")) {
      String params = line.substring(4);
      // Format: CAL:CO_GAIN,CO_OFFSET,O3_GAIN,O3_OFFSET,CO2_GAIN,CO2_OFFSET,PM25_GAIN,PM25_OFFSET,TEMP_OFFSET,HUM_OFFSET
      float values[10];
      int valIndex = 0;
      int lastComma = 0;
      
      for (int i = 0; i <= params.length(); i++) {
        if (i == params.length() || params.charAt(i) == ',') {
          String valStr = params.substring(lastComma, i);
          valStr.trim();
          values[valIndex++] = valStr.toFloat();
          lastComma = i + 1;
          if (valIndex >= 10) break;
        }
      }
      
      if (valIndex == 10) {
        CO_GAIN = values[0];
        CO_OFFSET = values[1];
        O3_GAIN = values[2];
        O3_OFFSET = values[3];
        CO2_GAIN = values[4];
        CO2_OFFSET = values[5];
        PM25_GAIN = values[6];
        PM25_OFFSET = values[7];
        TEMP_OFFSET = values[8];
        HUM_OFFSET = values[9];

        // Save to flash memory for persistence
        CalibrationData cal;
        cal.magic   = 0x5341514D; // "SAQM" magic bytes
        cal.version = lastAppliedVersion; // preserve server version
        cal.co_gain = CO_GAIN;
        cal.co_offset = CO_OFFSET;
        cal.o3_gain = O3_GAIN;
        cal.o3_offset = O3_OFFSET;
        cal.co2_gain = CO2_GAIN;
        cal.co2_offset = CO2_OFFSET;
        cal.pm25_gain = PM25_GAIN;
        cal.pm25_offset = PM25_OFFSET;
        cal.temp_offset = TEMP_OFFSET;
        cal.hum_offset = HUM_OFFSET;
        cal.send_interval_s = SEND_INTERVAL / 1000;

        DueFlashStorage dueFlashStorage;
        dueFlashStorage.write(0, (byte*)&cal, sizeof(cal));

        Serial.println("SUCCESS: Calibration updated");
      } else {
        Serial.print("ERROR: Expected 10 values, got ");
        Serial.println(valIndex);
      }
    }
  }
}

void loadCalibration() {
  DueFlashStorage dueFlashStorage;
  byte* flashBytes = dueFlashStorage.readAddress(0);
  CalibrationData* cal = (CalibrationData*)flashBytes;
  
  if (cal->magic == 0x5341514D) { // "SAQM"
    lastAppliedVersion = cal->version;
    CO_GAIN     = cal->co_gain;
    CO_OFFSET   = cal->co_offset;
    O3_GAIN     = cal->o3_gain;
    O3_OFFSET   = cal->o3_offset;
    CO2_GAIN    = cal->co2_gain;
    CO2_OFFSET  = cal->co2_offset;
    PM25_GAIN   = cal->pm25_gain;
    PM25_OFFSET = cal->pm25_offset;
    TEMP_OFFSET = cal->temp_offset;
    HUM_OFFSET  = cal->hum_offset;
    if (cal->send_interval_s >= 10 && cal->send_interval_s <= 3600) {
      SEND_INTERVAL = cal->send_interval_s * 1000;
    }
    Serial.println("SUCCESS: Calibration loaded from Flash memory");
  } else {
    Serial.println("INFO: No calibration record in Flash. Using firmware defaults.");
  }
}

void customDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    checkSerialCommands();
    delay(1);
  }
}
