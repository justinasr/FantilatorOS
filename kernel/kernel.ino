/*
   FantilatorOS
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Temperature measurement
#include <dht.h>

// Predefined button states
#define BUTTON_STATE_OFF 0
#define BUTTON_STATE_PRESS 1
#define BUTTON_STATE_WAIT 2
#define BUTTON_STATE_HOLD 3

// Delay for main loop in ms
#define LOOP_DELAY 25
// Delay before WAIT becomes HOLD in loops
#define HOLD_THRESHOLD (300/LOOP_DELAY)

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHT_PIN A0 // Temperature sensor
#define DHT_DELAY (5000/LOOP_DELAY)

#define PWM_PIN_LEFT 9
#define PWM_PIN_RIGHT 10

typedef struct {
  uint8_t pin;
  uint8_t state;
  int holdCounter;
} Button;

typedef struct {
  String label;
  // Qui peut être modifié
  bool modifiable;
  int8_t* value;
  String suffix;
} MenuItem;

// Digital pin 2 is down button
Button buttonDown = {2, BUTTON_STATE_OFF, 0};
// Digital pin 3 is up button
Button buttonUp = {3, BUTTON_STATE_OFF, 0};
// Digital pin 4 is enter button
Button buttonEnter = {4, BUTTON_STATE_OFF, 0};

// Temperature and humidity sensor
dht DHT;

uint16_t iteration = 0;
// Current power percentage
int8_t currentPowerLeft = 0;
int8_t currentPowerRight = 0;
int8_t currentPower = 0;

int8_t currentMenuItem = 0;
int8_t settingItemIndex = 0;

int8_t temperature = 0;
int8_t humidity = 0;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // Make the button pins an input:
  pinMode(buttonDown.pin, INPUT);
  pinMode(buttonUp.pin, INPUT);
  pinMode(buttonEnter.pin, INPUT);
  // Make the pwm pin as output
  pinMode(PWM_PIN_LEFT, OUTPUT);
  pinMode(PWM_PIN_RIGHT, OUTPUT);

  pinMode(DHT_PIN, INPUT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(1); // Pause for 1 millisecond

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(WHITE);

  analogWrite(PWM_PIN_LEFT, currentPowerLeft);
  analogWrite(PWM_PIN_RIGHT, currentPowerRight);
}

Button updateButtonState(Button button) {
  uint8_t newState = digitalRead(button.pin);
  if (newState == 0) {
    button.state = BUTTON_STATE_OFF;
    button.holdCounter = 0;
  } else {
    if (button.state == BUTTON_STATE_OFF) {
      button.state = BUTTON_STATE_PRESS;
    } else {
      if (button.holdCounter >= HOLD_THRESHOLD) {
        button.state = BUTTON_STATE_HOLD;
      } else {
        button.holdCounter++;
        button.state = BUTTON_STATE_WAIT;
      }
    }
  }
  return button;
}

// the loop routine runs over and over again forever:
void loop() {
  bool displayRefresh = (iteration == 0);
  iteration++;

  MenuItem menuItems[] = {{
      .label = F("Speed"),
      .modifiable = true,
      .value = &currentPower,
      .suffix = "%"
    },
    { .label = F("Speed left"),
      .modifiable = true,
      .value = &currentPowerLeft,
      .suffix = "%"
    },
    { .label = F("Speed right"),
      .modifiable = true,
      .value = &currentPowerRight,
      .suffix = "%"
    },
    { .label = F("Curr temp"),
      .modifiable = false,
      .value = &temperature,
      .suffix = "\xF7\x43"
    },
    { .label = F("Humidity"),
      .modifiable = false,
      .value = &humidity,
      .suffix = "%"
    },
    { .label = F("Build"),
      .modifiable = false,
      .value = 0,
      .suffix = F(__DATE__)
    }
  };
  int totalMenuItems = sizeof(menuItems) / sizeof(menuItems[0]);

  buttonDown = updateButtonState(buttonDown);
  buttonUp = updateButtonState(buttonUp);
  buttonEnter = updateButtonState(buttonEnter);

  if (iteration % DHT_DELAY == 0) {
    DHT.read11(DHT_PIN);
    temperature = DHT.temperature;
    humidity = DHT.humidity;
    displayRefresh = true;
  }

  int8_t valueAdjustment = 0;
  if (buttonDown.state == BUTTON_STATE_PRESS || buttonDown.state == BUTTON_STATE_HOLD) {
    valueAdjustment = -1;
  }

  if (buttonUp.state == BUTTON_STATE_PRESS || buttonUp.state == BUTTON_STATE_HOLD) {
    valueAdjustment = 1;
  }

  if (buttonEnter.state == BUTTON_STATE_PRESS) {
    if (settingItemIndex == 0) {
      MenuItem *item = &menuItems[currentMenuItem];
      if (item->modifiable) {
        settingItemIndex = currentMenuItem + 1;
      }
    } else {
      settingItemIndex = 0;
    }
    displayRefresh = true;
  }

  if (valueAdjustment != 0) {
    if (settingItemIndex == 0) {
      currentMenuItem = (totalMenuItems + currentMenuItem - valueAdjustment) % totalMenuItems;
    } else {
      MenuItem *item = &menuItems[settingItemIndex - 1];
      *(item->value) += valueAdjustment;
      if (*(item->value) < 0) {
        *(item->value) = 0;
      } else if (*(item->value) > 100) {
        *(item->value) = 100;
      }
      if (settingItemIndex == 1) {
        currentPowerLeft = currentPowerRight = currentPower;
      } else if (settingItemIndex == 2 || settingItemIndex == 3) {
        currentPower = *(item->value);
      }
      if (settingItemIndex == 1 || settingItemIndex == 2 || settingItemIndex == 3) {
        analogWrite(PWM_PIN_LEFT, (int)(2.55 * currentPowerLeft));
        analogWrite(PWM_PIN_RIGHT, (int)(2.55 * currentPowerRight));
      }
    }
    displayRefresh = true;
  }

  if (displayRefresh) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.println("Fantilator");
    display.setTextSize(1);
    for (int i = 0; i < totalMenuItems; i++) {
      if (i == currentMenuItem) {
        display.setTextColor(BLACK, WHITE);
        display.print("\x10");
      } else {
        display.setTextColor(WHITE);
        display.print(" ");
      }
      int8_t printedChars = 1;
      MenuItem *item = &menuItems[i];
      display.print(item->label);
      printedChars += item->label.length();
      String value;
      if (i == 0 && currentPowerLeft != currentPowerRight) {
        value = "--";
      } else if (item->value) {
        value = String(*item->value);
      } else {
        value = "";
      }
      int8_t neededSpaces = 19 - printedChars - value.length() - item->suffix.length();
      for (int8_t j = 0; j < neededSpaces; j++) {
        display.print(" ");
      }
      if (i == settingItemIndex - 1) {
        display.print("\x11");
      } else {
        display.print(" ");
      }
      display.print(value);
      display.print(item->suffix);
      if (i == settingItemIndex - 1) {
        display.println("\x10");
      } else {
        display.println(" ");
      }
    }
    display.display();
  }

  delay(LOOP_DELAY);        // delay in between reads for stability
}
