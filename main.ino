#include <EnvironmentCalculations.h>
#include <BME280I2C.h>
#include <Wire.h>

#include "FS.h"
#include "SD_MMC.h"

#define SERIAL_BAUD 115200

#define OUT_FILE "/BME280Data_1.txt"

// Assumed environmental values:
float referencePressure = 1018.6;  // hPa local QFF (official meteor-station reading)
float outdoorTemp = 20;           // °C  measured local outdoor temp.
float barometerAltitude = 60;  // meters ... map readings + barometer position

int clk = 36;
int cmd = 35;
int d0 = 37;
int d1 = 38;
int d2 = 33;
int d3 = 34;

BME280I2C::Settings settings(
  BME280::OSR_X1,
  BME280::OSR_X1,
  BME280::OSR_X1,
  BME280::Mode_Forced,
  BME280::StandbyTime_1000ms,
  BME280::Filter_16,
  BME280::SpiEnable_False,
  BME280I2C::I2CAddr_0x77     // Changed to 0x77
);

BME280I2C bme(settings);

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("Beginning");

  while(!Serial) {} // Wait

  // Open data and clock lines to BME280
  Wire.begin(19, 14);

  while (!bme.begin()) {
    Serial.println("BME280 sensor not found!");
    delay(1000);
  }

  Serial.printf("Assumed temperature: %f °C\n", outdoorTemp);
  Serial.printf("Assumed sea level pressure: %f hPa\n", referencePressure);
  Serial.printf("Assumed altitude: %f m", barometerAltitude);

  if(! SD_MMC.setPins(clk, cmd, d0, d1, d2, d3)){
    Serial.println("Pin change failed!");
    return;
  }

  if (!SD_MMC.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

    uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }

  Serial.print("SD_MMC Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  File file = SD_MMC.open(OUT_FILE, FILE_WRITE);
  file.print("begin_msg\n");
}

void printBME280data(Stream *client) {
  float temp(NAN), hum(NAN), pres(NAN);

  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_hPa);

  bme.read(pres, temp, hum, tempUnit, presUnit);

  EnvironmentCalculations::AltitudeUnit envAltUnit = EnvironmentCalculations::AltitudeUnit_Meters;
  EnvironmentCalculations::TempUnit envTempUnit = EnvironmentCalculations::TempUnit_Celsius;

  float altitude = EnvironmentCalculations::Altitude(pres, envAltUnit, referencePressure, outdoorTemp, envTempUnit);

  File file = SD_MMC.open(OUT_FILE, FILE_APPEND);
  if (!file) {
    client->printf("Error: failed to open file %s\n", OUT_FILE);
  } else {
    client->printf(".");
    file.printf("%f hPa / %f C / %f RH / %f m\n", pres, temp, hum, altitude);
  }
}

void loop() {
  printBME280data(&Serial);
  delay(100);
}
