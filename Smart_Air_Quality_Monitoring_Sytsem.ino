/*
 * SMART IOT AIR QUALITY MONITORING SYSTEM (CORE FIRMWARE)
 * Architecture: Arduino Due (32-bit ARM Cortex-M3)
 * Connectivity: SIM800L (2G GPRS)
 * Core Sensors: PMS5003, MH-Z19C, SGP41, MQ-7, MQ-131, DHT11
 * Documentation: 12-bit ADC Resolution | 180s Telemetry Cycle
 */

#include "DHT.h"
#include "RTClib.h"
#include <Arduino.h>
#include <MHZ19.h>
#include <NOxGasIndexAlgorithm.h>
#include <SensirionI2CSgp41.h>
#include <U8g2lib.h>
#include <VOCGasIndexAlgorithm.h>
#include <Wire.h>

// Include our modules
#include "config.h"
#include "sensors.h"
#include "display.h"
#include "communication.h"

// Devices & Algorithms
DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, 13, 11, 10, 8);
MHZ19 myMHZ19;
SensirionI2CSgp41 sgp41;
VOCGasIndexAlgorithm voc_helper;
NOxGasIndexAlgorithm nox_helper;

// ---------------------------------------------------------
// HARDWARE INITIALIZATION (setup())
// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);
  loadCalibration();
  u8g2.begin();
  dht.begin();
  Wire.begin();
  rtc.begin();
  sgp41.begin(Wire);

  // Upgrade ADC resolution to 12-bit (0-4095) for the Arduino Due
  analogReadResolution(12);

  // Serial Port Assignments
  Serial3.begin(9600);            // PMS5003
  Serial2.begin(9600);            // MH-Z19C
  myMHZ19.begin(Serial2);         // Link MHZ19 library
  myMHZ19.autoCalibration(false); // Disable auto-calibration to prevent drift

  SIM800L_SERIAL.begin(GSM_BAUD); // SIM800L on Serial1

  if (!rtc.begin()) {
//    Serial.println("RTC not found!");
  }
  // --- HOW TO SET THE TIME PERMANENTLY ---
  // STEP 1: Uncomment the line below, and upload the code to the Arduino.
  //         (This sets the RTC to your computer's exact current time).
  // STEP 2: Immediately add the '//' back to comment the line out again.
  // STEP 3: Upload the code a SECOND time. Now the time is saved forever!

  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  if (rtc.lostPower()) {
//    Serial.println("RTC lost power! Battery might be dead.");
  }
//  Serial.println(F("\n================================"));
//  Serial.println(F("Air Quality Monitor with GSM & CO2"));
//  Serial.println(F("================================\n"));

  customDelay(3000);
  initGSM();
}

// ----------------------
//  Main LOOP
// ----------------------
void loop() {
  checkSerialCommands();
  readSensors();
  updateDisplay();

  // ---------------------------------------------------------
  // TELEMETRY TIMING LOGIC
  // ---------------------------------------------------------
  unsigned long currentTime = millis();
  if (currentTime - lastSendTime >= SEND_INTERVAL) {
    if (gsmReady && conditioning_s == 0) {
      sendDataToBackend();
    } else if (!gsmReady) {
//      Serial.println(F("GSM not ready, attempting to reinitialize..."));
      initGSM();
    }
    lastSendTime = currentTime;
  }

  // ---------------------------------------------------------
  // MQTT LOOP - Processes incoming messages
  // ---------------------------------------------------------
  if (gsmReady) {
    if (!mqtt.connected()) {
      mqttConnect();
    }
    mqtt.loop();
  }

  customDelay(100);
}

