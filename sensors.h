#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <DHT.h>
#include <MHZ19.h>
#include <NOxGasIndexAlgorithm.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <Wire.h>
#include "config.h"

// External sensor objects (defined in main sketch)
extern DHT dht;
extern MHZ19 myMHZ19;
extern SensirionI2CSgp41 sgp41;
extern VOCGasIndexAlgorithm voc_helper;
extern NOxGasIndexAlgorithm nox_helper;

// Sensor variables
extern uint16_t pm1_0, pm2_5, pm10;
extern int32_t voc_index, nox_index;
extern int co2_ppm;
extern float humidity, temperature, CO_ppm, O3_ppm, wind_speed;
extern long wind_sum;
extern float mq7_raw, mq131_raw;

// Function declarations
bool readPMS5003();
void readSensors();

#endif // SENSORS_H
