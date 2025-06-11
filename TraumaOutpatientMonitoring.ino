#include <Wire.h>
#include <MPU6050.h>
#include <SD.h>
#include <SPI.h>

#define SerialBT Serial

MPU6050 mpu;

// Pin Definitions
const int buzzerPin = 15;
const int sdChipSelect = 5;
const int pulsePin = 36;
const int soundPin = 39;

// Thresholds
const float fallThresholdLow = 0.85;
const float fallThresholdHigh = 1.15;
const float tempThreshold = 2.0;
const int pulseThreshold = 850;
const int soundThreshold = 700;

float previousTemp = 0.0;

void setup() {
  SerialBT.begin(115200);

  Wire.begin();
  mpu.initialize();

  pinMode(buzzerPin, OUTPUT);
  pinMode(pulsePin, INPUT);
  pinMode(soundPin, INPUT);

//Checking the working of SD card
  if (!SD.begin(sdChipSelect)) {
    SerialBT.println("SD Card initialization failed!");
    while (true);
  }

//Creating csv file
  File logFile = SD.open("/events.csv", FILE_WRITE);
  if (logFile) {
    logFile.println("Timestamp,Event,AccX,AccY,AccZ,AccMag,Temp,Pulse,Sound,Lat,Lon");
    logFile.close();
  }

  previousTemp = (mpu.getTemperature() / 340.0) + 36.53;
}

//Fall detection, Pulse and Sound analog sim, Temperature change detection
void loop() {
  // Read MPU6050
  int16_t ax, ay, az, tempRaw;
  mpu.getAcceleration(&ax, &ay, &az);
  tempRaw = mpu.getTemperature();

  float accX = ax / 16384.0;
  float accY = ay / 16384.0;
  float accZ = az / 16384.0;
  float accMag = sqrt(accX * accX + accY * accY + accZ * accZ);
  float currentTemp = (tempRaw / 340.0) + 36.53;
  float tempChange = abs(currentTemp - previousTemp);

  // Read analog sensors
  int pulseValue = analogRead(pulsePin);
  int soundValue = analogRead(soundPin);

  // Show live readings
  SerialBT.printf("[LIVE] Acc: %.2f,%.2f,%.2f | Mag: %.2f\n", accX, accY, accZ, accMag);
  SerialBT.printf("[LIVE] Temp: %.2f°C | ΔTemp: %.2f°C | Pulse: %d | Sound: %d\n",
                  currentTemp, tempChange, pulseValue, soundValue);

  bool eventOccurred = false;
  String eventType = "";

  // Default mock coordinates
  String lat = "12.9716";
  String lon = "77.5946";

  if (accMag < fallThresholdLow || accMag > fallThresholdHigh) {
    eventType = "FALL";
    triggerAlert(1000, 1500);
    SerialBT.printf("⚠️ FALL detected! Coordinates: %s, %s\n", lat.c_str(), lon.c_str());
    eventOccurred = true;
  }

  if (tempChange > tempThreshold) {
    eventType = "TEMP_ALERT";
    triggerAlert(2000, 1500);
    SerialBT.printf("⚠️ Temperature spike detected! ΔTemp: %.2f°C\n", tempChange);
    eventOccurred = true;
  }

  if (pulseValue > pulseThreshold) {
    eventType = "PULSE_ALERT";
    triggerAlert(1500, 1000);
    SerialBT.println("⚠️ Pulse abnormality detected!");
    eventOccurred = true;
  }

  if (soundValue > soundThreshold) {
    eventType = "SOUND_ALERT";
    triggerAlert(1800, 1000);
    SerialBT.println("⚠️ Loud sound detected!");
    eventOccurred = true;
  }

  if (eventOccurred) {
    logEvent(eventType, accX, accY, accZ, accMag, currentTemp, pulseValue, soundValue, lat, lon);
    SerialBT.println("✅ Event logged to SD card.\n");
  }

  previousTemp = currentTemp;
  delay(500);
}

//When online sim is being used

void triggerAlert(int frequency, int duration) {
  tone(buzzerPin, frequency);
  delay(duration);
  noTone(buzzerPin);
}

//Logging events into the csv file
void logEvent(String event, float ax, float ay, float az, float mag, float temp,
              int pulse, int sound, String lat, String lon) {
  File logFile = SD.open("/events.csv", FILE_WRITE);
  if (logFile) {
    logFile.print(millis()); logFile.print(",");
    logFile.print(event); logFile.print(",");
    logFile.print(ax, 2); logFile.print(",");
    logFile.print(ay, 2); logFile.print(",");
    logFile.print(az, 2); logFile.print(",");
    logFile.print(mag, 2); logFile.print(",");
    logFile.print(temp, 1); logFile.print(",");
    logFile.print(pulse); logFile.print(",");
    logFile.print(sound); logFile.print(",");
    logFile.print(lat); logFile.print(",");
    logFile.println(lon);
    logFile.close();
  }
}
