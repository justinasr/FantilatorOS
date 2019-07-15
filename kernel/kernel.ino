/*
   FantilatorOS
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

#define PWM_PIN_LEFT 9
#define PWM_PIN_RIGHT 10

#define BUZZER_PIN A0

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

uint16_t iteration = 0;
// Current power percentage
int8_t currentPowerLeft = 0;
int8_t currentPowerRight = 0;
int8_t currentPower = 0;

int8_t currentMenuItem = 0;
int8_t settingItemIndex = -1;

int8_t buzzerPitch = 1;

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

  pinMode(BUZZER_PIN, INPUT);
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
      .suffix = F("%")
    },
    { .label = F(" left"),
      .modifiable = true,
      .value = &currentPowerLeft,
      .suffix = F("%")
    },
    { .label = F(" right"),
      .modifiable = true,
      .value = &currentPowerRight,
      .suffix = F("%")
    },
    { .label = F("Beep"),
      .modifiable = true,
      .value = &buzzerPitch,
      .suffix = F("")
    },
    { .label = F("Build"),
      .modifiable = false,
      .value = 0,
      .suffix = F(__DATE__)
    }
    ,
    { .label = F("Build"),
      .modifiable = false,
      .value = 0,
      .suffix = F(__TIME__)
    }
  };
  int totalMenuItems = sizeof(menuItems) / sizeof(menuItems[0]);

  buttonDown = updateButtonState(buttonDown);
  buttonUp = updateButtonState(buttonUp);
  buttonEnter = updateButtonState(buttonEnter);

  int8_t valueAdjustment = 0;
  bool buzz = false;
  bool errorBuzz = false;
  if (buttonDown.state == BUTTON_STATE_PRESS || buttonDown.state == BUTTON_STATE_HOLD) {
    valueAdjustment = -1;
    buzz = true;
  }

  if (buttonUp.state == BUTTON_STATE_PRESS || buttonUp.state == BUTTON_STATE_HOLD) {
    valueAdjustment = 1;
    buzz = true;
  }

  if (buttonEnter.state == BUTTON_STATE_PRESS) {
    buzz = true;
    if (settingItemIndex == -1) {
      MenuItem *item = &menuItems[currentMenuItem];
      if (item->modifiable) {
        settingItemIndex = currentMenuItem;
      } else {
        errorBuzz = true;
      }
    } else {
      settingItemIndex = -1;
    }
    displayRefresh = true;
  }

  if (valueAdjustment != 0) {
    if (settingItemIndex == -1) {
      currentMenuItem = (totalMenuItems + currentMenuItem - valueAdjustment) % totalMenuItems;
    } else {
      MenuItem *item = &menuItems[settingItemIndex];
      *(item->value) += valueAdjustment;
      if (*(item->value) < 0) {
        *(item->value) = 0;
        errorBuzz = true;
      } else if (*(item->value) > 100) {
        *(item->value) = 100;
        errorBuzz = true;
      }
      if (settingItemIndex == 0) {
        currentPowerLeft = currentPowerRight = currentPower;
      } else if (settingItemIndex == 1 || settingItemIndex == 2) {
        currentPower = *(item->value);
      } else if (settingItemIndex == 3) {
        if (buzzerPitch > 3) {
          buzzerPitch = 3;
          errorBuzz = true;
        }
      }
      if (settingItemIndex == 0 || settingItemIndex == 1 || settingItemIndex == 2) {
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
    display.println(F("Fantilator"));
    display.setTextSize(1);
    int startItem = 0;
    int endItem = 5;
    if (currentMenuItem < 3) {
      startItem = 0;
      endItem = 5;
    } else if (currentMenuItem > totalMenuItems - 3) {
      startItem = totalMenuItems - 5;
      endItem = totalMenuItems;
    } else {
      startItem = currentMenuItem - 2;
      endItem = currentMenuItem + 3;
    }
    for (int i = startItem; i < endItem; i++) {
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
        if (i == 3) {
          if (*item->value == 0) {
            value = F("off");
          } else if (*item->value == 1) {
            value = F("low");
          } else if (*item->value == 2) {
            value = F("medium");
          } else {
            value = F("high");
          }
        } else {
          value = String(*item->value);
        }
      } else {
        value = "";
      }
      int8_t neededSpaces = 19 - printedChars - value.length() - item->suffix.length();
      for (int8_t j = 0; j < neededSpaces; j++) {
        display.print(" ");
      }
      if (i == settingItemIndex) {
        display.print("\x11");
      } else {
        display.print(" ");
      }
      display.print(value);
      display.print(item->suffix);
      if (i == settingItemIndex) {
        display.println("\x10");
      } else {
        display.println(" ");
      }
    }
    display.display();

    if ((errorBuzz || buzz) && buzzerPitch > 0) {
      pinMode(BUZZER_PIN, OUTPUT);
      int iterationDelay = 1200 / buzzerPitch;
      int iterations = 8 * buzzerPitch;
      if (errorBuzz) {
        iterationDelay = 1500;
        iterations = 20;
      }
      for (int i = 0; i < iterations; i ++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(iterationDelay);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(iterationDelay);
      }
      pinMode(BUZZER_PIN, INPUT);
    }
  }

  delay(LOOP_DELAY);        // delay in between reads for stability
}
