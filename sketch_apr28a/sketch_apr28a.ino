#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#define IR_START_PIN 2  
#define IR_STOP_PIN 3   
#define TOGGLE_PIN 4
#define BUZZER_PIN 6
#define LED_PIN 5

const int maxLaps = 50;
float lapTimes[maxLaps];
float lapSpeeds[maxLaps];

unsigned long startTime = 0;
unsigned long lapStartTime = 0;
int currentLap = 0;
float trackDistance = 0;
int totalLaps = 0;
bool raceStarted = false;
bool isReversed = false;
bool waitingForStart = true;
bool raceCompleted = false;
int lapDisplayIndex = 0;
bool switchRunningMode = false;  
bool inSettings = false;         
bool inMenu = true;             

LiquidCrystal_I2C lcd(0x20, 16, 2);

// 4x4 Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {10, 9, 8, 7};
byte colPins[COLS] = {13, 12, 11, A0};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String input = "";
bool enteringDistance = true;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(IR_START_PIN, INPUT);
  pinMode(IR_STOP_PIN, INPUT);
  pinMode(TOGGLE_PIN, INPUT);

  digitalWrite(LED_PIN, HIGH);

  lcd.begin();
  lcd.backlight();
  showMainMenu();
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    if (key == 'A') {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("resetting...");
      delay(1000);
      resetRace();
      return;
    }

    if (inMenu) {
      handleMenuInput(key);
      return;
    }
    if (inSettings) {
      handleSettingsInput(key);
      return;
    }

    if (key == '*') {
      input = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(enteringDistance ? "enter distance:" : "enter laps:");
    } 
    else if (key == '#') {
      if (raceCompleted) {
        showLapDetails();
      } 
      else if (enteringDistance) {
        trackDistance = input.toFloat();
        enteringDistance = false;
        input = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("enter laps:");
      } 
      else {
        totalLaps = input.toInt();
        input = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ready to start!");
      }
    } 
    else {
      if (input.length() < 5) {
        input += key;
      }
      lcd.setCursor(0, 1);
      lcd.print(input);
    }
  }

  int toggleState = digitalRead(TOGGLE_PIN);
  isReversed = (toggleState == HIGH);

  if (switchRunningMode) {
    isReversed = (currentLap % 2 != 0);
  }

  int startSensor = digitalRead(IR_START_PIN);
  int stopSensor = digitalRead(IR_STOP_PIN);

  if (isReversed) {
    int temp = startSensor;
    startSensor = stopSensor;
    stopSensor = temp;
  }

  if (startSensor == HIGH && !raceStarted) {
    startTime = millis();
    lapStartTime = millis();
    raceStarted = true;
    waitingForStart = false;
    
    digitalWrite(LED_PIN, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("race started!");
  }

  if (stopSensor == HIGH && raceStarted && !waitingForStart) {
    unsigned long lapEndTime = millis();
    float lapTime = (lapEndTime - lapStartTime) / 1000.0;
    float speed = trackDistance / lapTime;

    lapTimes[currentLap] = lapTime;
    lapSpeeds[currentLap] = speed;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("lap ");
    lcd.print(currentLap + 1);
    lcd.print(": ");
    lcd.print(lapTime);
    lcd.print("s");

    lcd.setCursor(0, 1);
    lcd.print("speed: ");
    lcd.print(speed);
    lcd.print(" m/s");

    currentLap++;
    waitingForStart = true;

    if (currentLap == totalLaps) {
      raceStarted = false;
      raceCompleted = true;
      lapDisplayIndex = 0;
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("race completed!");
      lcd.setCursor(0, 1);
      lcd.print("press # for laps");

      delay(3000);
      digitalWrite(BUZZER_PIN, LOW);
    }
  }

  if (startSensor == HIGH && waitingForStart && raceStarted) {
    lapStartTime = millis();
    waitingForStart = false;
  }
}

void showLapDetails() {
  if (lapDisplayIndex < totalLaps) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("lap ");
    lcd.print(lapDisplayIndex + 1);
    lcd.print(": ");
    lcd.print(lapTimes[lapDisplayIndex]);
    lcd.print("s");

    lcd.setCursor(0, 1);
    lcd.print("Speed: ");
    lcd.print(lapSpeeds[lapDisplayIndex]);
    lcd.print(" m/s");

    lapDisplayIndex++;
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("race summary");
    lcd.setCursor(0, 1);
    lcd.print("completed!");
    delay(2000);
    resetRace();
  }
}

void resetRace() {
  input = "";
  currentLap = 0;
  lapDisplayIndex = 0;
  raceStarted = false;
  raceCompleted = false;
  enteringDistance = true;
  waitingForStart = true;
  inMenu = true;

  digitalWrite(LED_PIN, HIGH);

  showMainMenu();
}

void showMainMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1. practice");
  lcd.setCursor(0, 1);
  lcd.print("2. settings");
}

void handleMenuInput(char key) {
  if (key == '1') {
    inMenu = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("enter distance:");
  } 
  else if (key == '2') {
    inMenu = false;
    inSettings = true;
    showSettings();
  }
}

void showSettings() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("switch run:");
  lcd.setCursor(0, 1);
  lcd.print(switchRunningMode ? "enabled" : "disabled");
}

void handleSettingsInput(char key) {
  if (key == '#') {
    inSettings = false;
    inMenu = true;
    showMainMenu();
  } 
  else if (key == '1') {
    switchRunningMode = !switchRunningMode;
    showSettings();
  }
}