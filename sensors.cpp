#include "sensors.h"

// Sensor variables
uint16_t pm1_0 = 0, pm2_5 = 0, pm10 = 0;
int32_t voc_index = 0, nox_index = 0;
int co2_ppm = 0;
long wind_sum=0;
float humidity = 0, temperature = 0, CO_ppm = 0, O3_ppm = 0, wind_speed = 0;
float mq7_raw = 0.0, mq131_raw = 0.0;

// ----------------------
// Read PMS5003 (Serial 13)
// ----------------------
bool readPMS5003() {
  if (!Serial3.available()) return false;

  // Sync with the frame header 0x42 0x4D
  while (Serial3.available() > 0) {
    if (Serial3.peek() != 0x42) {
      Serial3.read(); // Discard garbage byte
    } else {
      break;
    }
  }

  // Ensure we have a full 32-byte frame waiting
  if (Serial3.available() < 32) return false;

  uint8_t data[32];
  Serial3.readBytes(data, 32);

  if (data[0] == 0x42 && data[1] == 0x4D) {
    pm1_0 = (data[10] << 8) | data[11];
    pm2_5 = (uint16_t)(PM25_GAIN * ((data[12] << 8) | data[13]) + PM25_OFFSET);
    pm10  = (data[14] << 8) | data[15];
    return true;
  }
  return false;
}

// Read all sensors
void readSensors() {
  readPMS5003();
  // Read MH-Z19C from Serial2 then apply calibration
  co2_ppm = (int)(CO2_GAIN * myMHZ19.getCO2() + CO2_OFFSET);
  // SGP41 Reading
  uint16_t srawVoc = 0, srawNox = 0;
  if (conditioning_s > 0) {
    sgp41.executeConditioning(0x8000, 0x6666, srawVoc);
    conditioning_s--;
  } else {
    sgp41.measureRawSignals(0x8000, 0x6666, srawVoc, srawNox);
  }
  voc_index = voc_helper.process(srawVoc);
  nox_index = nox_helper.process(srawNox);

  // Environment Readings
  humidity = dht.readHumidity() + HUM_OFFSET;
  temperature = dht.readTemperature() + TEMP_OFFSET;

  // --- CALIBRATED & SMOOTHED MQ-7 READ (Carbon Monoxide) ---
  long mq7_sum = 0;
  for (int i = 0; i < 10; i++) {
    mq7_sum += analogRead(MQ7_PIN);
    delay(2);
  }
  mq7_raw = mq7_sum / 10.0;

  float v7 = (mq7_raw * 3.3) / 4095.0;
  // Lowered threshold to 0.02V to prevent aggressively flatlining readings in clean air
  if (v7 > 0.02) {
    float rs7 = ((3.3 * RL_VALUE) / v7) - RL_VALUE;
    float ratio7 = rs7 / MQ7_R0;
    // MQ-7 correct log-log curve for CO: m = -0.65, b = 1.30
    CO_ppm = CO_GAIN * pow(10, ((log10(ratio7) - 1.30) / -0.65)) + CO_OFFSET;
  } else {
    // Even below threshold, the offset still applies as the minimum baseline
    CO_ppm = CO_OFFSET;
  }

  // --- CALIBRATED & SMOOTHED MQ-131 READ (Ozone) ---
  long mq131_sum = 0;
  for (int i = 0; i < 10; i++) {
    mq131_sum += analogRead(MQ131_PIN);
    delay(2);
  }
  mq131_raw = mq131_sum / 10.0;

  float v131 = (mq131_raw * 3.3) / 4095.0;
  if (v131 > 0.02) {
    float rs131 = ((3.3 * RL_VALUE) / v131) - RL_VALUE;
    float ratio131 = rs131 / MQ131_R0;
    // MQ-131 correct log-log curve for O3: m = 0.90, b = -0.90
    // O3 is an oxidizing gas, resistance INCREASES with concentration, so slope is positive.
    O3_ppm = O3_GAIN * pow(10, ((log10(ratio131) + 0.90) / 0.90)) + O3_OFFSET;
  } else {
    // Even below threshold, the offset still applies as the minimum baseline
    O3_ppm = O3_OFFSET;
  }

  // --- WIND SPEED SENSOR (0-5V, 0-30m/s) ---
  // IMPORTANT: Arduino Due operates at 3.3V. You MUST use a voltage divider to drop the 5V signal to <= 3.3V!
  // Example divider: R1 = 10k ohm (to sensor), R2 = 15k ohm (to ground) -> max 3.0V at pin.
  // Divider Ratio = (R1 + R2) / R2 = (10k + 15k) / 15k = 1.667
  const float WIND_DIVIDER_RATIO = 1.667; // Change this based on your actual resistor values!
  
  wind_sum = 0;
  for (int i = 0; i < 10; i++) {
    wind_sum += analogRead(WIND_PIN);
    delay(2);
  }
  float wind_avg = wind_sum / 10.0;
  float wind_adc_voltage = (wind_avg * 3.3) / 4095.0;
  float wind_sensor_voltage = wind_adc_voltage * WIND_DIVIDER_RATIO;
  wind_speed = (wind_sensor_voltage / 5.0) * 30.0;
  if (wind_speed < 0) wind_speed = 0;
  if (wind_speed > 30) wind_speed = 30; // Clamp to max 30m/s
}
