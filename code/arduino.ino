#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RTC_DS3231 rtc;

const int trigPin = 9;
const int echoPin = 10;
const int buzzer = 8;

const float BOTTLE_HEIGHT = 20.0;
const float SENSOR_OFFSET = 1.5;
const float MAX_WATER_LEVEL = BOTTLE_HEIGHT - SENSOR_OFFSET;
const float LOW_WATER_THRESHOLD = 5.0;

bool isReminding = false;
unsigned long reminderStartTime = 0;
const unsigned long REMINDER_DURATION = 60000;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000);

  unsigned long oledStart = millis();
  while (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    if (millis() - oledStart > 3000) {
      Serial.println("OLED init failed!");
      while (1);
    }
  }
  display.clearDisplay();

  if (!rtc.begin()) {
    Serial.println("RTC init failed!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting default time.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(trigPin, LOW);

  displayWelcome();
}

void loop() {
  DateTime now = rtc.now();
  unsigned long currentMillis = millis();
  float waterHeight = measureWaterLevel();

  if (waterHeight < LOW_WATER_THRESHOLD) {
    triggerLowWaterAlert();
  } else if (now.minute() == 0 && now.second() < 2) {
    triggerDrinkReminder(waterHeight);
  }

  if (isReminding && (currentMillis - reminderStartTime >= REMINDER_DURATION)) {
    isReminding = false;
    noTone(buzzer);
  }

  if (!isReminding) {
    updateDisplay(now, waterHeight);
  }

  delay(200);
}

float measureWaterLevel() {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(12);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  float distance = duration * 0.034 / 2;
  float waterHeight = MAX_WATER_LEVEL - distance;
  return constrain(waterHeight, 0, MAX_WATER_LEVEL);
}

void displayWelcome() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor((SCREEN_WIDTH - 72) / 2, 2);
  display.println("SARU'S TRACKER");
  display.display();
  delay(1000);
}

void updateDisplay(DateTime now, float waterHeight) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(30, 0);
  display.println("SARU'S TRACKER");
  display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

  display.setCursor(0, 13);
  display.print("Time: ");
  uint8_t hour = now.hour() % 12;
  if (hour == 0) hour = 12;
  display.print(hour);
  display.print(":");
  if (now.minute() < 10) display.print("0");
  display.print(now.minute());
  display.print(":");
  if (now.second() < 10) display.print("0");
  display.print(now.second());

  display.setCursor(100, 13);
  display.print(now.hour() < 12 ? " AM" : " PM");

  display.setCursor(0, 25);
  display.print("Date: ");
  display.print(now.day());
  display.print("/");
  display.print(now.month());
  display.print("/");
  display.print(now.year());

  display.setCursor(0, 37);
  display.print("Level: ");
  display.print(waterHeight, 1);
  display.print("cm");

  display.setCursor(0, 49);
  display.print("(");
  display.print((waterHeight / MAX_WATER_LEVEL) * 100, 0);
  display.print("%)");

  const int bottleX = 85;
  const int bottleY = 15;
  const int bottleW = 20;
  const int bottleH = 40;
  display.drawRect(bottleX, bottleY, bottleW, bottleH, WHITE);
  int fillHeight = map(waterHeight * 100, 0, MAX_WATER_LEVEL * 100, 0, bottleH);
  display.fillRect(bottleX, bottleY + (bottleH - fillHeight), bottleW, fillHeight, WHITE);

  display.display();
}

void triggerLowWaterAlert() {
  display.clearDisplay();
  display.drawRect(15, 10, 98, 44, WHITE);
  display.setTextSize(2);
  display.setCursor(35, 20);
  display.println("REFILL!");
  display.setTextSize(1);
  display.setCursor(30, 45);
  display.print("Water Low!");
  display.display();
  tone(buzzer, 1500, 500);
  delay(600);
}

void triggerDrinkReminder(float waterHeight) {
  isReminding = true;
  reminderStartTime = millis();
  display.clearDisplay();
  display.drawRect(15, 10, 98, 44, WHITE);
  display.setTextSize(2);
  display.setCursor(35, 20);
  display.println("DRINK!");
  display.setTextSize(1);
  display.setCursor(20, 45);
  display.print("Hourly Reminder");
  display.display();
  tone(buzzer, 1000, 200);
  delay(300);
  tone(buzzer, 1200, 200);
  delay(300);
  tone(buzzer, 1000, 200);
}
