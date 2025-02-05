#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Timer.h"
#include "LedControl.h"

//-------------------------------variaveis pong
#define PADSIZE 3
#define BALL_DELAY 500
#define GAME_DELAY 10
#define BOUNCE_VERTICAL 1
#define BOUNCE_HORIZONTAL -1
#define NEW_GAME_ANIMATION_SPEED 50
#define HIT_NONE 0
#define HIT_CENTER 1
#define HIT_LEFT 2
#define HIT_RIGHT 3
// ---------------------------------------------
#define VRX_PIN A1 // Arduino pin connected to joystick VRX
#define VRY_PIN A2 // Arduino pin connected to joystick VRY

// Button Pin
#define BUTTON_PIN 2 // Connect the button to pin 2

int xValue = 0; // To store joystick X-axis value
int yValue = 0; // To store joystick Y-axis value

// Initialize LCD with I2C address 0x27 (adjust this if your LCD uses a different address)
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// LED Matrix Pins: DIN, CLK, CS
LedControl matrix = LedControl(12,11,10,1);

// Menu variables
int currentOption = 0;              // Tracks the current menu option
const int numOptions = 3;           // Total number of menu options
String menuOptions[] = {"Go down for more options...", "1. Pong", "2. Flappy Pixel"}; // Menu options
String menuRestart[] = {"1. Yes", "2. No"};

//const int BUTTON_PIN = 2;     // Button for selection

// Thresholds for joystick movement
const int joystickThreshold = 500; // Joystick neutral is around 512
unsigned long lastDebounceTime = 0; // Debounce timer
const unsigned long debounceDelay = 200; // Delay for debounce

//----------------------------pong var

byte sad[] = {B00000000, B01000100, B00010000, B00010000, B00000000, B00111000, B01000100, B00000000};

byte smile[] = { B00000000, B01000100, B00010000, B00010000, B00010000, B01000100, B00111000, B00000000};

byte direction; 
int xball;
int yball;
int yball_prev;
byte xpad;
int ball_timer;

unsigned long lastBallMoveTime = 0; // Track the last time the ball moved

//------------------------------ Flappy var

// Game Variables
float birdY = 2.0;         // Bird's vertical position (0-7)
float birdVelocity = 0.0;  // Bird's velocity
const float gravity = 0.2; // Gravity pull
const float flapPower = -1.0; // Upward velocity when flapping
bool gameActive = true;    // Game state

// Pipe Variables
int pipeX = 7;           // Pipe's horizontal position
int pipeGapY;            // Vertical position of the gap in the pipe
const int pipeGapSize = 3; // Size of the gap (in rows)

// Game Timing
unsigned long previousMillis = 0;
const long frameDelay = 500; // Game refresh rate (in ms)

int pointsGame=0;

  static bool scored = false; // Flag to prevent multiple increments

void setFace(byte *sprite) { //faces appear in the beggining and ending of game
    for (int r = 0; r < 8; r++) {
        matrix.setRow(0, r, sprite[r]);
    }
}

void newGamePong() {
    matrix.clearDisplay(0);
    // Initial position
    xball = random(1, 7);
    yball = 1;
    direction = random(3, 6); 
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            matrix.setLed(0, r, c, HIGH);
            delay(NEW_GAME_ANIMATION_SPEED);
        }
    }
    setFace(smile);
    delay(1500);
    matrix.clearDisplay(0);
}

void setPad() {
    // Map joystick X-axis values to paddle position
    xValue = analogRead(VRX_PIN);
    xpad = map(xValue, 0, 1023, 0, 9 - PADSIZE);
}

int checkBounce() { //function determines if the ball has hit any borders (top, bottom, left, or right).
    if (!xball || !yball || xball == 7 || yball == 6) {
        return (yball == 0 || yball == 6) ? BOUNCE_HORIZONTAL : BOUNCE_VERTICAL;
    }
    return 0;
}

int getHit() { //checks if the ball hits the paddle in a specific region (left, center, or right).
    if (yball != 6 || xball < xpad || xball > xpad + PADSIZE) {
        return HIT_NONE;
    }
    if (xball == xpad + PADSIZE / 2) {
        return HIT_CENTER;
    }
    return xball < xpad + PADSIZE / 2 ? HIT_LEFT : HIT_RIGHT;
}

bool checkLoosePong() {
    return yball == 6 && getHit() == HIT_NONE;
}

void moveBall() {
    int bounce = checkBounce();
    if (bounce) {
        switch (direction) {
            case 0:
                direction = 4;
                break;
            case 1:
                direction = (bounce == BOUNCE_VERTICAL) ? 7 : 3;
                break;
            case 2:
                direction = 6;
                break;
            case 6:
                direction = 2;
                break;
            case 7:
                direction = (bounce == BOUNCE_VERTICAL) ? 1 : 5;
                break;
            case 5:
                direction = (bounce == BOUNCE_VERTICAL) ? 3 : 7;
                break;
            case 3:
                direction = (bounce == BOUNCE_VERTICAL) ? 5 : 1;
                break;
            case 4:
                direction = 0;
                break;
        }
    }

    // Check hit: modify direction if left or right
    switch (getHit()) {
        case HIT_LEFT:
            if (direction == 0) {
                direction = 7;
            } else if (direction == 1) {
                direction = 0;
            }
            break;
        case HIT_RIGHT:
            if (direction == 0) {
                direction = 1;
            } else if (direction == 7) {
                direction = 0;
            }
            break;
    }

    // Check orthogonal directions and borders ...
    if ((direction == 0 && xball == 0) || (direction == 4 && xball == 7)) {
        direction++;
    }
    if (direction == 0 && xball == 7) {
        direction = 7;
    }
    if (direction == 4 && xball == 0) {
        direction = 3;
    }
    if (direction == 2 && yball == 0) {
        direction = 3;
    }
    if (direction == 2 && yball == 6) {
        direction = 1;
    }
    if (direction == 6 && yball == 0) {
        direction = 5;
    }
    if (direction == 6 && yball == 6) {
        direction = 7;
    }

    // "Corner" case
    if (xball == 0 && yball == 0) {
        direction = 3;
    }
    if (xball == 0 && yball == 6) {
        direction = 1;
    }
    if (xball == 7 && yball == 6) {
        direction = 7;
    }
    if (xball == 7 && yball == 0) {
        direction = 5;
    }

    yball_prev = yball;
    if (2 < direction && direction < 6) {
        yball++;
    } else if (direction != 6 && direction != 2) {
        yball--;
    }
    if (0 < direction && direction < 4) {
        xball++;
    } else if (direction != 0 && direction != 4) {
        xball--;
    }
    xball = max(0, min(7, xball));
    yball = max(0, min(6, yball));
}

void gameOverPong() {
    setFace(sad);
    delay(1500);
    matrix.clearDisplay(0);
}

void drawGamePong() {
    if (yball_prev != yball) {
        matrix.setRow(0, yball_prev, 0);
    }
    matrix.setRow(0, yball, byte(1 << (xball)));
    byte padmap = byte(0xFF >> (8 - PADSIZE) << xpad);
    matrix.setRow(0, 7, padmap);
}

//--------------------------Flappy
// Update game logic
void gameFlappyBirdRun() {

  // Check if the button is pressed
  if (digitalRead(BUTTON_PIN) == HIGH) { // Button is pressed
    birdVelocity = flapPower; // Apply upward force
  }


  // Apply gravity
  birdVelocity += gravity;
  birdY += birdVelocity;

  // Check if the bird hits the ground or ceiling
  if (birdY > 7 || birdY < -1) {
    gameOverFlappy();
    return;
  }

  // Move pipe
  movePipe();

  // Check for collisions
  if (pipeX == 2) { // Bird is at the pipe's position
    int birdRow = (int)birdY;
    if (birdRow < pipeGapY || birdRow > pipeGapY + pipeGapSize - 1) {
      gameOverFlappy();
      return;
    }
  }
  pointsGame=pointsGame+10;
  // Redraw the bird and pipe
  drawGameFlappy();
  if(pipeX==2){
    lcd.setCursor(8,1);
    lcd.print(pointsGame);
  }
}

// Move the pipe left and reset if it goes off-screen
void movePipe() {
  pipeX--; // Move the pipe one column to the left
  if (pipeX < 0) {
    resetPipe(); // Reset the pipe when it goes off-screen
  }
}

// Reset pipe to the right side with a random gap
void resetPipe() {
  pipeX = 7; // Reset to the far right
  pipeGapY = random(0, 6 - pipeGapSize); // Random gap position
}

// Draw the game elements (bird and pipe) on the LED matrix
void drawGameFlappy() {
  // Clear the display
  matrix.clearDisplay(0);

  // Draw the pipe
  for (int row = 0; row < 8; row++) {
    if (row < pipeGapY || row > pipeGapY + pipeGapSize - 1) {
      // Draw pipe (filled cells)
      matrix.setLed(0, row, pipeX, true);
    }
  }

  // Draw the bird (make sure it doesn't conflict with the pipe rendering)
  int birdRow = (int)birdY;
  matrix.setLed(0, birdRow, 2, true); // Bird is at column 2
}
// Game over logic
void gameOverFlappy() {
  gameActive = false;

  // Display game over animation
  for (int i = 0; i < 8; i++) {
    matrix.setRow(0, i, B11111111); // Fill the screen
    delay(100);
  }

  // Clear the display
  matrix.clearDisplay(0);

  // // Restart the game
  restartGameFlappy();
}

// Restart the game
void restartGameFlappy() {
  birdY = 3.0;
  birdVelocity = 0.0;
  //gameActive = true;
  resetPipe();
  matrix.clearDisplay(0);
  //drawGameFlappy();
}





//------------------------------ MENU

void handleJoystickInput() {
  int yValue = analogRead(VRY_PIN);
  // Move up if joystick is pushed upward
  if (yValue < joystickThreshold - 100) {
    if (millis() - lastDebounceTime > debounceDelay) {
      currentOption = (currentOption - 1 + numOptions) % numOptions;
      displayMenu(menuOptions);
      lastDebounceTime = millis();
      Serial.println(yValue);
    }
  }
  
  // Move down if joystick is pushed downward
  if (yValue > joystickThreshold + 100) {
    if (millis() - lastDebounceTime > debounceDelay) {
      currentOption = (currentOption + 1) % numOptions;
      displayMenu(menuOptions);
      lastDebounceTime = millis();
    }
  }
}

void handleButtonInput() {
  if (digitalRead(BUTTON_PIN) == HIGH) { // Button pressed
    if (millis() - lastDebounceTime > debounceDelay) {
      // Select current option
      if (currentOption == 2) {
         // Call Game 1
         flappyGame();
         menuGameOver();
         pointsGame=0;
         delay(1500);
         displayMenu(menuOptions);
      } else if (currentOption == 1) {
         // Call Game 2
        pongGame();
        menuGameOver();
        pointsGame=0;
        delay(1500);
        displayMenu(menuOptions);
      }
      lastDebounceTime = millis();
    }
  }
}

void menuGameOver(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GAME OVER");
  lcd.setCursor(0, 1);
  lcd.print("Going back...");
}

void displayMenu(String menu[]) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select an Option:");
  lcd.setCursor(0, 1);
  lcd.print(menu[currentOption]);
}

void pongGame() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Game 1 Starting!");
  delay(2000); // Simulate game loading
  //instruçoes
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Use the joystick");
  delay(1000); // Simulate game loading
  lcd.setCursor(0,1);
  lcd.print("Points: ");
  newGamePong();
  while(1){
    if (millis() - lastBallMoveTime >= BALL_DELAY) {
      lastBallMoveTime = millis();
      moveBall();
    }

    setPad();
    drawGamePong();

    if (checkLoosePong()) {
        gameOverPong();
        break;
    }


    if (yball == 6 && yball_prev == 5 && getHit() != HIT_NONE && !scored) {
      pointsGame += 10;
      lcd.setCursor(8,1);
      lcd.print(pointsGame);
      scored = true; // Prevent further increments until the ball moves away
    } else if (yball < 6) {
      scored = false; // Reset flag when the ball moves up
    }
    delay(GAME_DELAY);
  }
}

void flappyGame() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Game 2 Starting!");
  delay(2000); // Simulate game loading
  //instruçoes
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Button to go up");
  delay(1000); // Simulate game loading
  lcd.setCursor(0,1);
  lcd.print("Points: ");

  gameActive=true;
  resetPipe();
  drawGameFlappy();
  while(1){
    if (gameActive) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= frameDelay) {
        previousMillis = currentMillis;
        gameFlappyBirdRun();
      }
    }
    else break;
  }
}


void setup() {
  Serial.begin(9600);
  // Initialize LCD
  lcd.init(); // Use lcd.init() for most libraries
  lcd.backlight();
  displayMenu(menuOptions); // Show the menu initially

  // Set up pins
  pinMode(BUTTON_PIN, INPUT);
  pinMode(VRX_PIN, INPUT);
  pinMode(VRY_PIN, INPUT);
  matrix.shutdown(0, false);
  matrix.setIntensity(0, 2);
  matrix.clearDisplay(0);
  randomSeed(analogRead(0));
}

void loop() {
  handleJoystickInput();
  handleButtonInput();
}
