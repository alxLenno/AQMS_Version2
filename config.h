#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ----------------------
// Pin definitions
// ----------------------
#define DHTPIN 7
#define DHTTYPE DHT11
#define MQ7_PIN A0
#define MQ131_PIN A1 // MQ-131 Ozone sensor
#define WIND_PIN A2  // Wind speed sensor (0-5V, 0-30m/s)

#define SIM800L_SERIAL Serial1
const long GSM_BAUD = 9600;

// GSM CONFIGURATION
extern const char *APN;
extern const char *BACKEND_URL;
extern const int BACKEND_PORT;
extern const char *BACKEND_PATH;
extern const char *PROTOCOL;

extern const char *MQTT_BROKER;
extern const int MQTT_PORT;
extern const char *DEVICE_ID;

extern const char *TOPIC_DATA;
extern const char *TOPIC_CALIBRATION;
extern const char *TOPIC_STATUS;

extern unsigned long SEND_INTERVAL;       // Telemetry interval
extern const unsigned long CAL_CHECK_INTERVAL;  // Calibration poll interval (5s)
extern unsigned long lastSendTime;
extern unsigned long lastCalibrationCheck;
extern bool gsmReady;

// MQ Calibration Constants (Adjust these after your burn-in)
// R0 = Resistance in clean air (kOhms). Use Page 4 to find these.
extern const float MQ7_R0;
extern const float MQ131_R0;
extern const float RL_VALUE; // Load resistor in kOhms

// Environmental Offsets (applied after reading)
extern float TEMP_OFFSET;
extern float HUM_OFFSET;

// -------------------------------------------------------
// Per-sensor calibration: calibrated = gain * raw + offset
// Dummy reference values — replace with measured corrections
// -------------------------------------------------------
// MQ-7 CO
extern float CO_GAIN;
extern float CO_OFFSET;
// MQ-131 O3
extern float O3_GAIN;
extern float O3_OFFSET;
// MH-Z19C CO2
extern float CO2_GAIN;
extern float CO2_OFFSET;
// PMS5003 PM2.5
extern float PM25_GAIN;
extern float PM25_OFFSET;
// DHT11 — reuse TEMP_OFFSET / HUM_OFFSET above

extern uint16_t conditioning_s;

struct CalibrationData {
  uint32_t magic;
  uint32_t version;      // Server version timestamp — used to detect new updates
  float co_gain;
  float co_offset;
  float o3_gain;
  float o3_offset;
  float co2_gain;
  float co2_offset;
  float pm25_gain;
  float pm25_offset;
  float temp_offset;
  float hum_offset;
  uint32_t send_interval_s;
};

void checkSerialCommands();
void loadCalibration();
void customDelay(unsigned long ms);
extern uint32_t lastAppliedVersion;  // tracks the version the board last applied

#endif // CONFIG_H
