#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>

// Select your modem:
#define TINY_GSM_MODEM_SIM800

#include <TinyGsmClient.h>

// Increase max packet size for the large JSON telemetry payload
#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h>

#include "config.h"
#include "sensors.h"

// Expose the MQTT client and Modem globally if needed
extern TinyGsm modem;
extern TinyGsmClient client;
extern PubSubClient mqtt;

// Function declarations
void initGSM();
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool mqttConnect();
void sendDataToBackend();
void publishStatusACK(uint32_t version);

#endif // COMMUNICATION_H
