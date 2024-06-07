// Include the libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

// Define OLED parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Define push button connections
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35

// Define the humidity sensor connection
#define DHTPIN 12

// Define the alerting system connections
#define BUZZER 15
#define LED_1 5

// Define time importing peripherals
#define NTP_SERVER     "pool.ntp.org"
int UTC_OFFSET = 0;
#define UTC_OFFSET_DST 0

// Declare the objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

// Global variables
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

int zone_minute = 0;
int zone_hour = 0;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0, 0, 0};
int alarm_minutes[] = {0, 0, 0};
bool alarm_triggered[] = {false, false, false};

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode = 0;
int max_modes = 6;
String modes[] = {"1 - Set Time Zone", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - Set Alarm 3", "5 - Disable Alarms", "6 - Enable Alarms"};

void setup() {

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC  = generate display voltage from 3.3v internally
  if (! display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // Show the display buffer on the screen. You must call display() after
  // Drawing commands to make them visible on screen!
  display.display();
  delay(2000);

  // Connecting to WiFi
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 2);
  }

  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 2);
  delay(1000);

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  // Clear the buffer
  display.clearDisplay();

  print_line("Welcome to Medibox", 10, 20, 2);
  delay(1500);
  display.clearDisplay();

}

void loop() {

  upadate_time_with_check_alarm(); // Showing the time
  if (digitalRead(PB_OK) == LOW) {
    delay(200);
    go_to_menu();
  }

  check_temp(); // Showing an alert on the screen if temperature or the humidy is beyond its limits
  delay(10);

}

void print_line(String text, int column, int row, int text_size) { // Print content on the screen

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row); // (column, row)
  display.println(text);
  display.display();

}

void print_time_now(void) { // Printing time on the screen

  display.clearDisplay();
  String temp_string = String(days) + ":" + String(hours) + ":" + String(minutes) + ":" + String(seconds);
  print_line(temp_string, 0, 28, 2);

}

void update_time() { // Updating the time

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinutes[3];
  strftime(timeMinutes, 3, "%M", &timeinfo);
  minutes = atoi(timeMinutes);

  char timeSeconds[3];
  strftime(timeSeconds, 3, "%S", &timeinfo);
  seconds = atoi(timeSeconds);

  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay);

}

void ring_alarm() { // Activating the alarm sytem consisting the LED and the buzzer
  display.clearDisplay();
  print_line("Medicine Time!", 0, 0, 2);

  digitalWrite(LED_1, HIGH);

  bool break_happened = false;

  // Ring the buzzer
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  digitalWrite(LED_1, LOW);
  display.clearDisplay();

}

void upadate_time_with_check_alarm(void) { // Updating the time and checking whether the alarm needs to be triggered
  update_time();
  print_time_now();

  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }

}

int wait_for_button_press() { // Identifying which button is pressed
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    }
    if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }
    if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    }
    if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }
  }

  update_time();

}

void go_to_menu() { // Activating the menu

  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      run_mode(current_mode);
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

}

void set_alarm(int alarm) { // Setting the hour and the minute of the alarm

  int temp_hour = alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      alarm_triggered[alarm] = false;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  delay(1000);

}

void run_mode(int mode) { // Setting the mode to run

  if (mode == 0) {
    set_time_zone();
  }

  if (mode == 1 || mode == 2 || mode == 3) {
    set_alarm(mode - 1);
  }

  if (mode == 4) {
    alarm_enabled = false;
  }

  if (mode == 5) {
    alarm_enabled = true;
  }
}

void check_temp() { // Checking the temp and humidity to alert if they have gone beyond their limits

  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.temperature > 32) {
    display.clearDisplay();
    print_line("TEMP HIGH", 0, 20, 2);
    delay(500);
  }
  else if (data.temperature < 26) {
    display.clearDisplay();
    print_line("TEMP LOW", 0, 20, 2);
    delay(500);
  }

  if (data.humidity > 80) {
    display.clearDisplay();
    print_line("HUMIDITY HIGH", 0, 20, 2);
    delay(500);
  }
  else if (data.humidity < 60) {
    display.clearDisplay();
    print_line("HUMIDITY LOW", 0, 20, 2);
    delay(500);
  }
}

void set_time_zone() { // Setting the time zone

  display.clearDisplay();
  print_line("Set Time Zone", 0, 50, 1);

  int temp_hour = zone_hour;
  while (true) {
    display.clearDisplay();
    print_line("Enter zone hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
      if (temp_hour > 14) {
        temp_hour = 14;
      }
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < -12) {
        temp_hour = -12;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      zone_hour = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }


  int temp_minute = zone_minute;
  while (true && (temp_hour != -12 || temp_hour != 14)) {
    display.clearDisplay();
    print_line("Enter zone minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 15;
      temp_minute = temp_minute % 60;
      if (temp_minute > 45) {
        temp_minute = 45;
      }
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 15;
      if (temp_minute < 0) {
        temp_minute = 0;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      zone_minute = temp_minute;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int zone_hour_temp = zone_hour * 60 * 60;
  int zone_minute_temp = 0;
  if (zone_hour_temp >= 0) {
    zone_minute_temp = zone_minute * 60;
  }
  else {
    zone_minute_temp = (-1) * zone_minute * 60;
  }

  UTC_OFFSET = zone_hour_temp + zone_minute_temp;

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();
  print_line("Time zone is set", 0, 0, 2);
  delay(1000);
}