/*
 * FantilatorOS
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
#define HOLD_THRESHOLD (500/LOOP_DELAY) 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

typedef struct{
    int pin;
    int state;
    int holdCounter;
} Button;

// Digital pin 2 is left button
Button buttonLeft = {2, BUTTON_STATE_OFF, 0};
// Digital pin 3 is right button
Button buttonRight = {3, BUTTON_STATE_OFF, 0};
// Digital pin 4 is right button
Button buttonUp = {4, BUTTON_STATE_OFF, 0};

int currentPower = 0;
int pwmPin = 9;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // Make the button pins an input:
  pinMode(buttonLeft.pin, INPUT);
  pinMode(buttonRight.pin, INPUT);
  pinMode(buttonUp.pin, INPUT);
  // Make the pwm pin as output
  pinMode(pwmPin, OUTPUT);

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
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
}

Button updateButtonState(Button button, int newState) {
  if (newState == 0) {
    button.state = BUTTON_STATE_OFF;
    button.holdCounter = 0;
  } else {
    if (button.state == BUTTON_STATE_OFF) {
      button.state = BUTTON_STATE_PRESS;
    } else {
      button.holdCounter++;
      if (button.holdCounter >= HOLD_THRESHOLD) {
        button.state = BUTTON_STATE_HOLD;
      } else {
        button.state = BUTTON_STATE_WAIT;
      }
    }
  }
  return button;
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input pin:
  int newButtonLeftState = digitalRead(buttonLeft.pin);
  int newButtonRightState = digitalRead(buttonRight.pin);
  int newButtonUpState = digitalRead(buttonUp.pin);

  buttonLeft = updateButtonState(buttonLeft, newButtonLeftState);
  buttonRight = updateButtonState(buttonRight, newButtonRightState);
  buttonUp = updateButtonState(buttonUp, newButtonUpState);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("L: ");
  if (buttonLeft.state == BUTTON_STATE_OFF) {
    display.println("OFF");
  } else if (buttonLeft.state == BUTTON_STATE_PRESS) {
    display.println("CLICK");
  } else if (buttonLeft.state == BUTTON_STATE_WAIT) {
    display.println("WAIT");
  } else {
    display.println("HOLD");
  }
  display.print("R: ");
  if (buttonRight.state == BUTTON_STATE_OFF) {
    display.println("OFF");
  } else if (buttonRight.state == BUTTON_STATE_PRESS) {
    display.println("CLICK");
  } else if (buttonRight.state == BUTTON_STATE_WAIT) {
    display.println("WAIT");
  } else {
    display.println("HOLD");
  }
  display.print("U: ");
  if (buttonUp.state == BUTTON_STATE_OFF) {
    display.println("OFF");
  } else if (buttonUp.state == BUTTON_STATE_PRESS) {
    display.println("CLICK");
  } else if (buttonUp.state == BUTTON_STATE_WAIT) {
    display.println("WAIT");
  } else {
    display.println("HOLD");
  }

  display.display();
  delay(LOOP_DELAY);        // delay in between reads for stability
}
