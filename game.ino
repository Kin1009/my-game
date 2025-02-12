#include <EEPROM.h>
#include <LiquidCrystal.h>

// Pin Definitions
const int adcPin = A0;

// Game Modes
enum GameMode { Normal, Challenging, Blindfolded, Endless };
GameMode currentMode = Normal;

// Game Variables
unsigned long startTime = 0;
unsigned long elapsedTime = 0;
int score = 0;
int highScore[3] = {0, 0, 0};
int currentCode[8];
int position = 0;
static unsigned long blindfoldStartTime = 0;
static bool codeVisible = true;
// LCD Setup
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Button Enum
enum Button { NONE, RIGHT, UP, DOWN, LEFT, SEL };

int lastButton = NONE;
unsigned long lastPressTime = 0;
const int debounceTime = 250;  // Adjust debounce time

Button receiveBtt() {
  int adc_key_in = analogRead(adcPin);

  if (adc_key_in > 1000) {  // No button pressed
    lastButton = NONE;
    return NONE;
  }

  // Get current button state
  Button currentButton = NONE;
  if (adc_key_in < 50) currentButton = RIGHT;
  else if (adc_key_in < 250) currentButton = UP;
  else if (adc_key_in < 450) currentButton = DOWN;
  else if (adc_key_in < 650) currentButton = LEFT;
  else if (adc_key_in < 850) currentButton = SEL;

  // If the current button press is the same as the last one, return NONE (debouncing)
  if (currentButton == lastButton && millis() - lastPressTime < debounceTime) {
    return NONE;
  }

  // Update last button press and debounce time
  lastPressTime = millis();
  lastButton = currentButton;

  return currentButton;  // Return the stable button press
}

void setup() {
  pinMode(adcPin, INPUT);
  pinMode(A7, INPUT);
  randomSeed(analogRead(A7));
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  Serial.begin(9600);

  // Read high scores from EEPROM
  for (int i = 0; i < 3; i++) {
    highScore[i] = EEPROM.read(i); // Assuming addresses 0, 1, 2
  }

  displayTitleScreen();
}

void loop() {
  switch (currentMode) {
    case Normal:
    case Challenging:
    case Blindfolded:
    case Endless:
      runGame();
      break;
  }
}

void displayTitleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("My Game");
  delay(1000);
  showMenu();
}

void showMenu() {
  int menuSelection = 0;
  lcd.clear();
  while (true) {
    String modeName[4] = {"Normal", "Challenging", "Blindfolded", "Endless"};
    lcd.setCursor(0, 0);
    lcd.print(modeName[menuSelection]);
    lcd.setCursor(0, 1);
    lcd.print("High Score: ");
    lcd.print(highScore[menuSelection]);


    Button btn = receiveBtt();

    if (btn == LEFT) {
      menuSelection--;
      if (menuSelection < 0) menuSelection = 3;
      lcd.clear();
    } else if (btn == RIGHT) {
      menuSelection++;
      if (menuSelection > 3) menuSelection = 0;
      lcd.clear();
    } else if (btn == SEL) {
      startGame(menuSelection);
      break;
    } else if (btn == NONE) {
    }
  }
}

void startGame(int modeSelection) {
  currentMode = (GameMode)modeSelection;
  score = 0;
  position = 0;
  randomizeCode();
  if (currentMode == Blindfolded) {
    blindfoldStartTime = millis();
    codeVisible = true;
  } else {
    startTime = millis();
  }
}

void randomizeCode() {
  for (int i = 0; i < 8; i++) {
    currentCode[i] = random(0, 2);
  }
}

void endGame(bool gameOver = false) {
  if (gameOver) {
    lcd.clear();
    lcd.print("Game Over!");
    delay(2500);
  }
  if (score > highScore[(int)currentMode]) {
    highScore[(int)currentMode] = score;
    EEPROM.write((int)currentMode, score);
  }
  showMenu();
}

void runGame() {
  static unsigned long lastUpdateTime = 0;
  static bool displayNeedsUpdate = true;
  static bool codeChanged = false;

  // Track elapsed time for blindfolded mode
  if (currentMode != Blindfolded || !codeVisible) {
    elapsedTime = (millis() - startTime) / 1000;
    if (elapsedTime != lastUpdateTime) {
      lastUpdateTime = elapsedTime;
      displayNeedsUpdate = true;
    }
  }

  Button btn = receiveBtt();
  if (btn != NONE) {
    displayNeedsUpdate = true;

    if (btn == LEFT) {
      position = (position > 0) ? position - 1 : 7;
      codeChanged = true;
    } else if (btn == RIGHT) {
      position = (position < 7) ? position + 1 : 0;
      codeChanged = true;
    } else if (btn == SEL) {
      // Flip the bit and mark it as changed
      if (currentCode[position] == 1) {
        currentCode[position] = 0;  // Mark as pressed (flipped)
        codeChanged = true;
      } else {
        // Game over if user presses the wrong bit (wrong action)
        endGame(true);
        return;
      }
    }
  }

  // Display the game screen
  if (displayNeedsUpdate) {
    lcd.clear();
    if (currentMode == Blindfolded) {
      // Show code for the first 3 seconds
      if (codeVisible) {
        // Show the actual code for the first 3 seconds
        for (int i = 0; i < 8; i++) {
          lcd.print(currentCode[i]);
        }
        lcd.print(" P: ");
        lcd.print(position);
        lcd.setCursor(0, 1);
        lcd.print("S: ");
        lcd.print(score);
        lcd.print(" T: ");
        lcd.print(getModeTime() / 1000 - elapsedTime);
        
        // After 3 seconds, hide the code
        if (millis() - blindfoldStartTime >= 3000) {
          codeVisible = false;  // Hide the code
          startTime = millis();  // Restart the timer
          lastUpdateTime = 0;
          lcd.setCursor(0, 0);
          lcd.print('\xff');  // Show the placeholder for hidden code
        }
      } else {
        // Show the hidden code (\xff) after 3 seconds
        for (int i = 0; i < 8; i++) {
          lcd.print('\xff');
        }
        lcd.print(" P: ");
        lcd.print(position);
        lcd.setCursor(0, 1);
        lcd.print("S: ");
        lcd.print(score);
        lcd.print(" T: ");
        lcd.print(getModeTime() / 1000 - elapsedTime);
      }
    } else {
      // Non-blindfolded mode, display the full code
      for (int i = 0; i < 8; i++) {
        lcd.print(currentCode[i]);
      }
      lcd.print(" P: ");
      lcd.print(position);
      lcd.setCursor(0, 1);
      lcd.print("S: ");
      lcd.print(score);
      lcd.print(" T: ");
      lcd.print(getModeTime() / 1000 - elapsedTime);
    }

    displayNeedsUpdate = false;
  }

  // Check if all bits in the code are flipped (set to 0)
  bool allZeros = true;
  for (int i = 0; i < 8; i++) {
    if (currentCode[i] == 1) {
      allZeros = false;
      break;
    }
  }

  if (allZeros) {
    score++;
    randomizeCode();  // Randomize code for the next level
    codeChanged = false;
    position = 0;
    codeVisible = true;  // Show the code at the start of the next round
    blindfoldStartTime = millis();  // Reset the blindfold timer for next round
  }

  // End the game if time is up in the current mode
  if (currentMode != Endless && (elapsedTime >= getModeTime() / 1000)) {
    endGame();
  }
}

unsigned long getModeTime() {
  switch (currentMode) {
    case Normal:
      return 120000;
    case Challenging:
      return 45000;
    case Blindfolded:
      return 180000;
    case Endless:
      return 0;
    default:
      return 0;
  }
}
