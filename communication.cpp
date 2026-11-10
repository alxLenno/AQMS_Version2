#include "communication.h"
#include <ArduinoJson.h>
#include <DueFlashStorage.h>

TinyGsm modem(SIM800L_SERIAL);
TinyGsmClient client(modem, 0); // MQTT client on mux 0
TinyGsmClient httpClient(modem, 1); // HTTP client on mux 1
PubSubClient mqtt(client);

// ---------------------------------------------------------
// MQTT CALLBACK - Handles incoming calibration
// ---------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println(msg);

  // Check if it's the calibration topic
  if (String(topic) == TOPIC_CALIBRATION) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    uint32_t serverVersion = doc["version"] | 0;
    if (serverVersion == 0 || serverVersion <= lastAppliedVersion) {
      Serial.println("Ignoring stale or invalid calibration version.");
      return;
    }

    // Apply new values
    CO_GAIN     = doc["co_gain"]     | CO_GAIN;
    CO_OFFSET   = doc["co_offset"]   | CO_OFFSET;
    O3_GAIN     = doc["o3_gain"]     | O3_GAIN;
    O3_OFFSET   = doc["o3_offset"]   | O3_OFFSET;
    CO2_GAIN    = doc["co2_gain"]    | CO2_GAIN;
    CO2_OFFSET  = doc["co2_offset"]  | CO2_OFFSET;
    PM25_GAIN   = doc["pm25_gain"]   | PM25_GAIN;
    PM25_OFFSET = doc["pm25_offset"] | PM25_OFFSET;
    TEMP_OFFSET = doc["temp_offset"] | TEMP_OFFSET;
    HUM_OFFSET  = doc["hum_offset"]  | HUM_OFFSET;
    
    if (doc.containsKey("send_interval_s")) {
      uint32_t new_interval = doc["send_interval_s"];
      if (new_interval >= 10 && new_interval <= 3600) {
        SEND_INTERVAL = new_interval * 1000;
      }
    }
    
    lastAppliedVersion = serverVersion;

    // Persist to flash
    CalibrationData calData;
    calData.magic       = 0x5341514D;
    calData.version     = lastAppliedVersion;
    calData.co_gain     = CO_GAIN;
    calData.co_offset   = CO_OFFSET;
    calData.o3_gain     = O3_GAIN;
    calData.o3_offset   = O3_OFFSET;
    calData.co2_gain    = CO2_GAIN;
    calData.co2_offset  = CO2_OFFSET;
    calData.pm25_gain   = PM25_GAIN;
    calData.pm25_offset = PM25_OFFSET;
    calData.temp_offset = TEMP_OFFSET;
    calData.hum_offset  = HUM_OFFSET;
    calData.send_interval_s = SEND_INTERVAL / 1000;

    DueFlashStorage dueFlashStorage;
    dueFlashStorage.write(0, (byte*)&calData, sizeof(calData));

    Serial.print("SUCCESS: Instant calibration applied (v");
    Serial.print(serverVersion);
    Serial.println(")");

    // Send ACK
    publishStatusACK(serverVersion);
  }
}

// ---------------------------------------------------------
// MQTT CONNECT RECONNECT
// ---------------------------------------------------------
bool mqttConnect() {
  if (!modem.isGprsConnected()) {
    Serial.println("GPRS disconnected! Forcing GSM re-initialization...");
    gsmReady = false; // Force main loop to call initGSM()
    return false;
  }

  // Generate a unique client ID to avoid session collisions on shared broker
  String clientId = String(DEVICE_ID) + "-" + String(random(0xffff), HEX);
  Serial.print("Connecting to ");
  Serial.print(MQTT_BROKER);
  Serial.print(" as ");
  Serial.print(clientId);

  boolean status = mqtt.connect(clientId.c_str());

  if (status == false) {
    Serial.println(" fail");
    delay(5000); // Back-off before the caller can retry
    return false;
  }
  Serial.println(" success");
  mqtt.subscribe(TOPIC_CALIBRATION);
  return mqtt.connected();
}

// ---------------------------------------------------------
// INITIALIZE GSM & MQTT
// ---------------------------------------------------------
void initGSM() {
  gsmReady = false;
  Serial.println("Initializing GSM module (TinyGSM)...");
  
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  if (!modem.init()) {
    Serial.println("Failed to init modem, delaying 10s and retrying");
    return;
  }

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }

  Serial.print("Connecting to ");
  Serial.print(APN);
  if (!modem.gprsConnect(APN, "", "")) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  if (modem.isGprsConnected()) {
    Serial.println("GPRS connected");
  }

  // Setup MQTT
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512); // Required to handle the large JSON telemetry payload

  if (mqttConnect()) {
    gsmReady = true;
    Serial.println("GSM/MQTT Initialized Successfully.");
  }
}

// ---------------------------------------------------------
// PUBLISH ACKNOWLEDGE
// ---------------------------------------------------------
void publishStatusACK(uint32_t version) {
  if (!mqtt.connected()) return;
  
  String payload = "{";
  payload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"status\":\"SUCCESS\",";
  payload += "\"version\":" + String(version);
  payload += "}";

  mqtt.publish(TOPIC_STATUS, payload.c_str());
  Serial.println("Published ACK to status topic.");
}

// ---------------------------------------------------------
// SEND DATA TO BACKEND
// ---------------------------------------------------------
void sendDataToBackend() {
  if (!gsmReady) {
    Serial.println("Cannot send telemetry. GSM not initialized.");
    return;
  }

  if (!mqtt.connected()) {
    Serial.println("MQTT disconnected. Attempting to reconnect...");
    if (!mqttConnect()) {
      return;
    }
  }

  Serial.println("Preparing MQTT Telemetry Payload...");

  // Build JSON payload with ALL sensors
  String jsonData = "{";
  jsonData += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  jsonData += "\"location\":{";
  jsonData += "\"latitude\":-1.2921,";
  jsonData += "\"longitude\":36.8219";
  jsonData += "},";
  jsonData += "\"applied_cal_version\":" + String(lastAppliedVersion) + ",";
  jsonData += "\"metrics\":{";
  jsonData += "\"pm1\":"        + String(pm1_0)        + ",";
  jsonData += "\"pm25\":"       + String(pm2_5)        + ",";
  jsonData += "\"pm10\":"       + String(pm10)         + ",";
  jsonData += "\"co\":"         + String(CO_ppm, 2)    + ",";
  jsonData += "\"co2\":"        + String(co2_ppm)      + ",";
  jsonData += "\"o3\":"         + String(O3_ppm)       + ",";
  jsonData += "\"wind_speed\":" + String(wind_speed, 1)+ ",";
  jsonData += "\"temperature\":" + String(temperature, 1) + ",";
  jsonData += "\"humidity\":"   + String(humidity, 1)  + ",";
  jsonData += "\"voc_index\":"  + String(voc_index)    + ",";
  jsonData += "\"nox_index\":"  + String(nox_index);
  jsonData += "}}";

  // Publish to MQTT
  if (mqtt.publish(TOPIC_DATA, jsonData.c_str())) {
    Serial.println("Telemetry published successfully via MQTT!");
  } else {
    Serial.println("Failed to publish telemetry.");
  }

  // ---------------------------------------------------------
  // DUAL TRANSMISSION: HTTP POST TO PYTHONANYWHERE
  // ---------------------------------------------------------
  Serial.print("Connecting to HTTP backend: ");
  Serial.println(BACKEND_URL);
  
  if (httpClient.connect(BACKEND_URL, BACKEND_PORT)) {
    Serial.println("Connected to PythonAnywhere. Sending POST...");
    
    // HTTP Request Headers
    httpClient.print("POST ");
    httpClient.print(BACKEND_PATH);
    httpClient.println(" HTTP/1.1");
    
    httpClient.print("Host: ");
    httpClient.println(BACKEND_URL);
    
    httpClient.println("Content-Type: application/json");
    
    httpClient.print("Content-Length: ");
    httpClient.println(jsonData.length());
    
    httpClient.println(); // Blank line indicates end of headers
    
    // HTTP Request Body
    httpClient.println(jsonData);
    
    // Wait for server response (optional but good for debugging)
    long timeout = millis();
    while (httpClient.connected() && millis() - timeout < 5000L) {
      while (httpClient.available()) {
        char c = httpClient.read();
        Serial.print(c);
        timeout = millis();
      }
    }
    Serial.println();
    httpClient.stop();
    Serial.println("HTTP POST complete.");
  } else {
    Serial.println("Failed to connect to HTTP backend.");
  }
}
