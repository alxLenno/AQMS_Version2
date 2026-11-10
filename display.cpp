#include "display.h"
#include <RTClib.h>

// External RTC object
extern RTC_DS3231 rtc;

// Display variables
int page = 0;
unsigned long lastSwitchTime = 0;
const unsigned long pageInterval = 4000;

// Helper macro: horizontal centre x for a single string
#define CENTRE_X(s) ((128 - u8g2.getStrWidth(s)) / 2)

void updateDisplay() {
  DateTime now = rtc.now();
  char timeStr[10], dateStr[12];
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  sprintf(dateStr, "%02d/%02d/%d", now.day(), now.month(), now.year());

  if (millis() - lastSwitchTime > pageInterval) {
    page = (page + 1) % 6;
    lastSwitchTime = millis();
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  // HEADER – time flush left, date flush right
  u8g2.drawStr(0, 10, timeStr);
  u8g2.drawStr(128 - u8g2.getStrWidth(dateStr), 10, dateStr);
  u8g2.drawHLine(0, 13, 128);

  // --- PAGE 0: PARTICULATE MATTER ---
  if (page == 0) {
    const char* title0 = "[ PM LEVELS ]";
    u8g2.drawStr(CENTRE_X(title0), 25, title0);

    char p1[20], p25[20], p10[20];
    sprintf(p1,  "PM1.0: %d ug/m3", pm1_0);
    sprintf(p25, "PM2.5: %d ug/m3", pm2_5);
    sprintf(p10, "PM10 : %d ug/m3", pm10);

    // Block-centre: anchor all 3 lines to the widest string's left edge
    int x0 = (128 - (int)max(u8g2.getStrWidth(p1), max(u8g2.getStrWidth(p25), u8g2.getStrWidth(p10)))) / 2;
    u8g2.drawStr(x0, 40, p1);
    u8g2.drawStr(x0, 52, p25);
    u8g2.drawStr(x0, 64, p10);
  }

  // --- PAGE 1: SGP41 VOC & NOX ---
  else if (page == 1) {
    const char* title1 = "[ VOC & NOX ]";
    u8g2.drawStr(CENTRE_X(title1), 25, title1);

    if (conditioning_s > 0) {
      char wuStr[24];
      sprintf(wuStr, "Warming up... %ds", conditioning_s);
      u8g2.drawStr(CENTRE_X(wuStr), 45, wuStr);
    } else {
      char vStr[20], nStr[20];
      sprintf(vStr, "VOC Index: %ld", voc_index);
      sprintf(nStr, "NOx Index: %ld", nox_index);

      int x1 = (128 - (int)max(u8g2.getStrWidth(vStr), u8g2.getStrWidth(nStr))) / 2;
      u8g2.drawStr(x1, 45, vStr);
      u8g2.drawStr(x1, 60, nStr);
    }
  }

  // --- PAGE 2: ENVIRONMENT (Temp, Hum, CO, CO2, O3) ---
  else if (page == 2) {
    u8g2.setFont(u8g2_font_ncenB08_tr);

    char tStr[20], hStr[20], coStr[20], co2Str[20], o3Str[20];
    sprintf(tStr,   "Temp: %.1f C",    temperature);
    sprintf(hStr,   "Hum : %.1f %%",   humidity);
    sprintf(coStr,  "CO  : %.2f ppm",  CO_ppm);
    sprintf(co2Str, "CO2 : %d ppm",    co2_ppm);
    sprintf(o3Str,  "O3  : %.2f ppm",  O3_ppm);

    // Block-centre: widest of all 5 lines sets the shared left edge
    uint16_t wA = max(u8g2.getStrWidth(tStr),   u8g2.getStrWidth(hStr));
    uint16_t wB = max(u8g2.getStrWidth(coStr),  u8g2.getStrWidth(co2Str));
    int x2 = (128 - (int)max((uint16_t)max(wA, wB), u8g2.getStrWidth(o3Str))) / 2;
    u8g2.drawStr(x2, 24, tStr);
    u8g2.drawStr(x2, 34, hStr);
    u8g2.drawStr(x2, 44, coStr);
    u8g2.drawStr(x2, 54, co2Str);
    u8g2.drawStr(x2, 64, o3Str);
  }

  // --- PAGE 3: ADVICE ---
  else if (page == 3) {
    const char* title3 = "[ ADVICE ]";
    u8g2.drawStr(CENTRE_X(title3), 25, title3);

    const char* advice;
    if (CO_ppm < 5 && pm2_5 < 12 && voc_index < 150 && co2_ppm < 800) {
      advice = "Air Quality: GOOD";
    } else if (CO_ppm > 9 || voc_index > 300 || pm2_5 > 35 || co2_ppm > 1200) {
      advice = "DANGER: VENTILATE!";
    } else {
      advice = "Quality: MODERATE";
    }
    u8g2.drawStr(CENTRE_X(advice), 50, advice);
  }

  // --- PAGE 4: RAW SENSORS ---
  else if (page == 4) {
    const char* title4 = "[ RAW SENSORS ]";
    u8g2.drawStr(CENTRE_X(title4), 25, title4);

    char r7[20], r131[20], rCO2[20];
    sprintf(r7,   "Raw MQ7 : %.1f", mq7_raw);
    sprintf(r131, "Raw O3  : %.1f", mq131_raw);
    sprintf(rCO2, "Raw CO2 : %d", co2_ppm);

    int x4 = (128 - (int)max(u8g2.getStrWidth(r7), max(u8g2.getStrWidth(r131), u8g2.getStrWidth(rCO2)))) / 2;
    u8g2.drawStr(x4, 40, r7);
    u8g2.drawStr(x4, 52, r131);
    u8g2.drawStr(x4, 64, rCO2);
  }

  // --- PAGE 5: WIND SPEED ---
  else if (page == 5) {
    const char* title5 = "[ WIND DATA ]";
    u8g2.drawStr(CENTRE_X(title5), 25, title5);

    char wStr[24], wStr1[24];
    sprintf(wStr,  "Speed: %.1f m/s",     wind_speed);
    //sprintf(wStr1, "Raw Speed: %.1f m/s", wind_sum);

    int x5 = (128 - (int)max(u8g2.getStrWidth(wStr), u8g2.getStrWidth(wStr1))) / 2;
    u8g2.drawStr(x5, 48, wStr);
    //u8g2.drawStr(x5, 62, wStr1);
  }

  u8g2.sendBuffer();
}
