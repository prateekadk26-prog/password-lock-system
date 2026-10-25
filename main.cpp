#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <Preferences.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 21
#define OLED_SCL 22

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

#define SERVO_PIN 15

Servo doorServo;



#define BUZZER_PIN 4

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {
  5,
  18,
  19,
  25
};

byte colPins[COLS] = {
  26,
  23,
  32,
  33
};

Keypad keypad = Keypad(
  makeKeymap(keys),
  rowPins,
  colPins,
  ROWS,
  COLS
);




Preferences preferences;

String password = "";
String enteredPassword = "";


enum Mode {
  SET_PASSWORD,
  ENTER_PASSWORD,
  CHANGE_OLD_PASSWORD,
  CHANGE_NEW_PASSWORD
};

Mode currentMode;



int wrongAttempts = 0;

const int MAX_ATTEMPTS = 3;

const unsigned long LOCK_TIME = 30000; // 30 seconds

unsigned long lockStartTime = 0;

bool systemLocked = false;


void setup() {

  Serial.begin(115200);


  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      )) {

    Serial.println("OLED not found!");

    while (true);
  }


  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);


  doorServo.attach(SERVO_PIN);

  doorServo.write(0);


  preferences.begin("doorlock", false);

  password = preferences.getString(
    "password",
    ""
  );



  if (password == "") {

    currentMode = SET_PASSWORD;

    showSetPassword();

  } else {

    currentMode = ENTER_PASSWORD;

    showEnterPassword();
  }


  Serial.println("================================");
  Serial.println("ESP32 PASSWORD DOOR LOCK");
  Serial.println("================================");
}


void loop() {

  if (systemLocked) {

    handleLockout();

    return;
  }


  char key = keypad.getKey();


  if (key == NO_KEY) {
    return;
  }


  Serial.print("Key pressed: ");
  Serial.println(key);

  if (currentMode == SET_PASSWORD) {

    handleSetPassword(key);

  }


  else if (currentMode == ENTER_PASSWORD) {

    handleEnterPassword(key);

  }


  else if (currentMode == CHANGE_OLD_PASSWORD) {

    handleOldPassword(key);

  }


  else if (currentMode == CHANGE_NEW_PASSWORD) {

    handleNewPassword(key);
  }
}

void handleSetPassword(char key) {

  if (key >= '0' && key <= '9') {

    if (enteredPassword.length() < 8) {

      enteredPassword += key;

      showPassword("SET PASSWORD");
    }
  }


  else if (key == '*') {

    enteredPassword = "";

    showSetPassword();
  }


  else if (key == '#') {

    if (enteredPassword.length() < 4) {

      showMessage(
        "PASSWORD",
        "MIN 4 DIGITS"
      );

      delay(1500);

      enteredPassword = "";

      showSetPassword();

      return;
    }


    password = enteredPassword;

    preferences.putString(
      "password",
      password
    );


    Serial.println("Password saved!");


    tone(
      BUZZER_PIN,
      2000,
      200
    );


    showMessage(
      "PASSWORD",
      "SAVED!"
    );

    delay(1500);


    enteredPassword = "";

    currentMode = ENTER_PASSWORD;

    showEnterPassword();
  }
}

void handleEnterPassword(char key) {

  // Number
  if (key >= '0' && key <= '9') {

    if (enteredPassword.length() < 8) {

      enteredPassword += key;

      showPassword("ENTER PASSWORD");
    }
  }


  else if (key == '*') {

    enteredPassword = "";

    showEnterPassword();
  }


  else if (key == '#') {

    checkPassword();
  }


  else if (key == 'A') {

    enteredPassword = "";

    currentMode = CHANGE_OLD_PASSWORD;

    showMessage(
      "CHANGE PASSWORD",
      "OLD PASSWORD"
    );
  }
}



void checkPassword() {

  Serial.print("Entered: ");
  Serial.println(enteredPassword);

  if (enteredPassword == password) {

    Serial.println("ACCESS GRANTED");

    wrongAttempts = 0;


    tone(
      BUZZER_PIN,
      2000,
      200
    );


    showMessage(
      "ACCESS",
      "GRANTED"
    );


    doorServo.write(90);


    delay(5000);


    doorServo.write(0);


    Serial.println("Door locked.");


    enteredPassword = "";

    showEnterPassword();

    return;
  }


  wrongAttempts++;


  Serial.print("Wrong attempt: ");
  Serial.println(wrongAttempts);



  tone(
    BUZZER_PIN,
    1000,
    150
  );

  delay(200);

  tone(
    BUZZER_PIN,
    1000,
    150
  );

  delay(200);

  tone(
    BUZZER_PIN,
    1000,
    150
  );


  if (wrongAttempts >= MAX_ATTEMPTS) {

    enteredPassword = "";

    systemLocked = true;

    lockStartTime = millis();


    Serial.println("================================");
    Serial.println("SECURITY LOCKOUT!");
    Serial.println("SYSTEM LOCKED FOR 30 SECONDS");
    Serial.println("================================");


    showMessage(
      "SECURITY",
      "LOCKED!"
    );


    return;
  }


  display.clearDisplay();

  display.setTextColor(WHITE);

  display.setTextSize(1);

  display.setCursor(15, 5);

  display.println("WRONG PASSWORD");


  display.setTextSize(2);

  display.setCursor(25, 25);

  display.print("TRY ");

  display.print(wrongAttempts);

  display.print("/");

  display.println(MAX_ATTEMPTS);


  display.setTextSize(1);

  display.setCursor(20, 50);

  display.println("TRY AGAIN");


  display.display();


  delay(1500);


  enteredPassword = "";

  showEnterPassword();
}

void handleLockout() {

  unsigned long elapsed =
    millis() - lockStartTime;


  // Remaining time

  int remaining =
    (LOCK_TIME - elapsed) / 1000;

  if (elapsed >= LOCK_TIME) {

    systemLocked = false;

    wrongAttempts = 0;

    enteredPassword = "";


    Serial.println("Security lockout finished.");


    showMessage(
      "SYSTEM",
      "READY"
    );


    delay(1000);


    showEnterPassword();

    return;
  }


  display.clearDisplay();

  display.setTextColor(WHITE);


  display.setTextSize(1);

  display.setCursor(25, 5);

  display.println("SECURITY ALERT");


  display.setTextSize(2);

  display.setCursor(20, 22);

  display.println("LOCKED");


  display.setTextSize(1);

  display.setCursor(30, 48);

  display.print("WAIT ");

  display.print(remaining);

  display.println(" SEC");


  display.display();


  // Alarm beep every second

  static unsigned long lastBeep = 0;


  if (millis() - lastBeep >= 1000) {

    tone(
      BUZZER_PIN,
      1500,
      150
    );

    lastBeep = millis();
  }


  delay(50);
}


void handleOldPassword(char key) {

  if (key >= '0' && key <= '9') {

    if (enteredPassword.length() < 8) {

      enteredPassword += key;

      showPassword("OLD PASSWORD");
    }
  }


  else if (key == '*') {

    enteredPassword = "";

    showMessage(
      "CHANGE PASSWORD",
      "OLD PASSWORD"
    );
  }


  else if (key == '#') {

    if (enteredPassword == password) {

      enteredPassword = "";

      currentMode = CHANGE_NEW_PASSWORD;


      showMessage(
        "ENTER NEW",
        "PASSWORD"
      );

    } else {

      tone(
        BUZZER_PIN,
        1000,
        300
      );


      showMessage(
        "WRONG",
        "PASSWORD"
      );


      delay(1500);


      enteredPassword = "";

      currentMode = ENTER_PASSWORD;

      showEnterPassword();
    }
  }
}


void handleNewPassword(char key) {

  if (key >= '0' && key <= '9') {

    if (enteredPassword.length() < 8) {

      enteredPassword += key;

      showPassword("NEW PASSWORD");
    }
  }


  else if (key == '*') {

    enteredPassword = "";

    showMessage(
      "ENTER NEW",
      "PASSWORD"
    );
  }


  else if (key == '#') {

    if (enteredPassword.length() < 4) {

      showMessage(
        "PASSWORD",
        "MIN 4 DIGITS"
      );

      delay(1500);

      enteredPassword = "";

      showMessage(
        "ENTER NEW",
        "PASSWORD"
      );

      return;
    }


    password = enteredPassword;


    preferences.putString(
      "password",
      password
    );


    Serial.println("Password changed!");


    tone(
      BUZZER_PIN,
      2000,
      200
    );


    showMessage(
      "PASSWORD",
      "CHANGED!"
    );


    delay(1500);


    enteredPassword = "";

    currentMode = ENTER_PASSWORD;

    showEnterPassword();
  }
}

void showSetPassword() {

  display.clearDisplay();

  display.setTextColor(WHITE);

  display.setTextSize(1);


  display.setCursor(25, 5);

  display.println("WELCOME");


  display.setCursor(15, 20);

  display.println("SET PASSWORD");


  display.setCursor(10, 40);

  display.println("4-8 DIGITS");


  display.setCursor(10, 52);

  display.println("# = SAVE");


  display.display();
}


void showEnterPassword() {

  display.clearDisplay();

  display.setTextColor(WHITE);

  display.setTextSize(1);


  display.setCursor(20, 5);

  display.println("DOOR LOCK");


  display.setCursor(20, 20);

  display.println("ENTER PASSWORD");


  display.setTextSize(2);

  display.setCursor(35, 38);

  display.println("----");


  display.display();
}


void showPassword(String title) {

  display.clearDisplay();

  display.setTextColor(WHITE);


  display.setTextSize(1);

  display.setCursor(20, 5);

  display.println(title);


  display.setTextSize(2);

  display.setCursor(25, 30);


  for (int i = 0; i < enteredPassword.length(); i++) {

    display.print("*");

    display.print(" ");
  }


  display.display();
}

void showMessage(
  String line1,
  String line2
) {

  display.clearDisplay();

  display.setTextColor(WHITE);


  display.setTextSize(1);

  display.setCursor(20, 10);

  display.println(line1);


  display.setTextSize(2);

  display.setCursor(20, 32);

  display.println(line2);


  display.display();
}
