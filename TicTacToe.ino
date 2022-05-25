#include <Arduino.h>
#include <SPI.h>
#include <Ucglib.h>
#include <XPT2046.h>
#include "Buzzer.h"

#define X_WIN_RESULT 0
#define O_WIN_RESULT 1
#define DRAW_RESULT 2

#define BLUE_SELECT_BUTTON 10
#define BLUE_RIGHT_BUTTON 11
#define BLUE_DOWN_BUTTON 15
#define BLUE_UP_BUTTON 12
#define BLUE_LEFT_BUTTON 13

#define RED_SELECT_BUTTON 21
#define RED_RIGHT_BUTTON 20
#define RED_DOWN_BUTTON 2
#define RED_UP_BUTTON 19
#define RED_LEFT_BUTTON 18

#define buzzer 3

volatile Ucglib_ILI9341_18x240x320_HWSPI ucg(/*cd=*/ 45 , /*cs=*/ 42, /*reset=*/ 44);
XPT2046 touch(/*cs=*/ 43, /*irq=*/ 41);

char gameMap[3][3] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
uint8_t x_coord[3] = {113, 183, 253};
uint8_t y_coord[3] = {71, 141, 211};

uint8_t color_red[3] = {255, 0, 0};
uint8_t color_blue[3] = {0, 0, 205};
uint8_t color_gameScreen[3] = {0, 0, 0};
uint8_t color_startingScreen[3] = {153, 0, 0};
uint8_t *color_X, *color_O;

bool is_X_turn = true;
bool is_bot_turn = true;
bool is_bot_red = false;
volatile bool isInGameScreen = false;
volatile bool isInEndGameScreen = false;
volatile bool isInStartingScreen = false;
volatile bool isInColorChoiceScreen = false;
volatile bool redToMove = false;
volatile bool selection_button = false;
volatile bool buzzer_playing = false;

char last_bot_symbol;

uint8_t score_blue = 0;
uint8_t score_red = 0;
uint8_t score_draw = 0;
uint8_t no_moves = 0;
uint8_t gameMode;
volatile uint8_t startingSelection;
volatile uint8_t gameSelection_x, gameSelection_y;
volatile uint8_t endGameSelection;
volatile uint8_t color_selected;


void setup() {
  setupButtons();
  ucg.begin(UCG_FONT_MODE_TRANSPARENT);
  touch.begin(ucg.getWidth(), ucg.getHeight());
  ucg.setRotate90();
  touch.setRotation(touch.ROT90);
  ucg.clearScreen();

  touch.setCalibration(208, 336, 1768, 1768);
  drawStartingScreen();
  randomSeed(analogRead(A1));
}

void loop() {
  if (isInStartingScreen && selection_button) {
    if (startingSelection == 0) {
      gameMode = 0;
      isInStartingScreen = false;
      prepareGame(true);
    } else if (startingSelection == 1) {
      gameMode = 1;
      isInStartingScreen = false;
      drawColorChoiceBox();
      prepareGame(true);
    } else if (startingSelection == 2) {
      gameMode = 2;
      isInStartingScreen = false;
      drawColorChoiceBox();
      prepareGame(true);
    } else {
      gameMode = 3;
      isInStartingScreen = false;
      drawColorChoiceBox();
      prepareGame(true);
    }
    selection_button = false;
    return;
  }
  if (isInStartingScreen && touch.isTouching()) {
    uint16_t x, y;
    touch.getPosition(x, y);
    checkGameModeButton(x, y);
    return;
  }
  if (isInGameScreen && gameMode > 0 && is_bot_turn) {
    if (gameMode == 1)
      makeEasyMove();
    else if (gameMode == 2)
      makeMediumMove();
    else
      makeHardMove();
    return;
  }
  if (isInGameScreen && selection_button) {
    selection_button = false;
    if (is_X_turn && gameMap[gameSelection_y][gameSelection_x] == ' ') {
      make_X_move(gameSelection_y, gameSelection_x);
    } else if (!is_X_turn && gameMap[gameSelection_y][gameSelection_x] == ' ') {
      make_O_move(gameSelection_y, gameSelection_x);
    } else {
      playWrongMoveTone();
    }
  }
  if (isInGameScreen && touch.isTouching() && !is_bot_turn) {
    uint16_t x, y;
    touch.getPosition(x, y);
    bool isOnMap = getMapPosition(x, y);
    if (isOnMap) {
      if (is_X_turn && gameMap[y][x] == ' ') {
        make_X_move(y, x);
        if (gameMode == 0)
          delay(300);
      } else if (!is_X_turn && gameMap[y][x] == ' ') {
        make_O_move(y, x);
        if (gameMode == 0)
          delay(300);
      } else {
        // wrong move
        // buzzer sound
        playWrongMoveTone();
      }
    }
  }
}


void drawStartingScreen() {
  ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
  ucg.drawBox(0, 0, 320, 240);

  ucg.setFont(ucg_font_fub25_hf);
  ucg.setColor(255, 255, 255);
  ucg.setPrintPos(62, 45);
  ucg.print("Tic-Tac-Toe");

  ucg.setColor(255, 215, 0);
  ucg.drawRBox(40, 125, 110, 45, 10);
  ucg.drawRBox(170, 70, 110, 45, 10);
  ucg.drawRBox(170, 125, 110, 45, 10);
  ucg.drawRBox(170, 180, 110, 45, 10);

  ucg.setColor(0, 0, 0);
  ucg.drawRFrame(40, 125, 110, 45, 10);
  ucg.drawRFrame(170, 70, 110, 45, 10);
  ucg.drawRFrame(170, 125, 110, 45, 10);
  ucg.drawRFrame(170, 180, 110, 45, 10);

  ucg.setFont(ucg_font_timB14_tf);
  ucg.setColor(255, 255, 255);

  ucg.setPrintPos(81, 154);
  ucg.print("PvP");
  ucg.setPrintPos(207, 99);
  ucg.print("Easy");
  ucg.setPrintPos(195, 154);
  ucg.print("Medium");
  ucg.setPrintPos(182, 209);
  ucg.print("Impossible");

  // draw initial selection frame
  ucg.setColor(0, 255, 0);
  ucg.drawRFrame(37, 122, 116, 51, 10);

  startingSelection = 0;
  isInStartingScreen = true;
}

void drawColorChoiceBox() {
  ucg.setColor(255, 255, 255);
  ucg.drawRBox(20, 50, 280, 170, 15);

  ucg.setColor(0, 0, 0);
  ucg.drawRFrame(20, 50, 280, 170, 15);

  ucg.setFont(ucg_font_timB14_tf);
  ucg.setPrintPos(83, 70);
  ucg.print("Choose your color:");

  ucg.setColor(255, 0, 0);
  ucg.drawRBox(30, 90, 125, 120, 10);

  ucg.setColor(0, 0, 205);
  ucg.drawRBox(165, 90, 125, 120, 10);

  isInColorChoiceScreen = true;
  color_selected = 0;
  // wait for a color selection
  while (color_selected == 0) {
    if (touch.isTouching()) {
      uint16_t x, y;
      touch.getPosition(x, y);
      if (ColorChoiceButtonPressed(x, y))
        break;
    }
  }
  if (color_selected == 1)
    is_bot_red = false;
  else if (color_selected == 2)
    is_bot_red = true;
  isInColorChoiceScreen = false; 
}

bool ColorChoiceButtonPressed(uint16_t x, uint16_t y) {
  if (x >= 30 && x <= 155 && y >= 90 && y <= 210) {
    is_bot_red = false;
    return true;
  }
  if (x >= 165 && x <= 285 && y >= 90 && y <= 210) {
    is_bot_red = true;
    return true;
  }
  return false;
}

void checkGameModeButton(uint16_t x, uint16_t y) {
  if (x >= 40 && x <= 150 && y >= 125 && y <= 170) {
    gameMode = 0;
    prepareGame(true);
    return;
  }
  if (x >= 170 && x <= 280 && y >= 70 && y <= 115) {
    gameMode = 1;
    drawColorChoiceBox();
    prepareGame(true);
    return;
  }
  if (x >= 170 && x <= 280 && y >= 125 && y <= 170) {
    gameMode = 2;
    drawColorChoiceBox();
    prepareGame(true);
    return;
  }
  if (x >= 170 && x <= 280 && y >= 180 && y <= 225) {
    gameMode = 3;
    drawColorChoiceBox();
    prepareGame(true);
    return;
  }
}

void printScoreBlue() {
  // clear previous score
  ucg.setFontMode(UCG_FONT_MODE_SOLID);
  ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
  ucg.drawBox(50, 185, 35, 35);

  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
  ucg.setFont(ucg_font_logisoso20_tf);
  ucg.setColor(255, 255, 255);
  ucg.setPrintPos(55, 215);
  ucg.print(score_blue);
}

void printScoreRed() {
  // clear previous score
  ucg.setFontMode(UCG_FONT_MODE_SOLID);
  ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
  ucg.drawBox(50, 95, 35, 35);

  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
  ucg.setFont(ucg_font_logisoso20_tf);
  ucg.setColor(255, 255, 255);
  ucg.setPrintPos(55, 125);
  ucg.print(score_red);
}

void printScoreDraw() {
  // clear previous score
  ucg.setFontMode(UCG_FONT_MODE_SOLID);
  ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
  ucg.drawRBox(50, 140, 35, 35, 5);

  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
  ucg.setFont(ucg_font_logisoso20_tf);
  ucg.setColor(255, 255, 255);
  ucg.setPrintPos(55, 170);
  ucg.print(score_draw);
}

void drawGameScreen() {
  // set font color by drawing a box
  ucg.setFontMode(UCG_FONT_MODE_SOLID);
  ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
  ucg.drawBox(0, 0, 320, 240);

  ucg.setColor(255, 255, 255);
  ucg.drawVLine(165, 15, 210);
  ucg.drawVLine(235, 15, 210);
  ucg.drawHLine(95, 85, 210);
  ucg.drawHLine(95, 155, 210);

  // score
  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
  ucg.setFont(ucg_font_logisoso20_tf);
  ucg.setColor(255, 255, 255);
  ucg.setPrintPos(5, 85);
  ucg.print("Score:");

  ucg.setColor(255, 0, 0);
  ucg.drawBox(5, 95, 45, 35);
  printScoreRed();

  ucg.setColor(0, 0, 205);
  ucg.drawBox(5, 185, 45, 35);
  printScoreBlue();

  ucg.setColor(255, 0, 0);
  ucg.drawTriangle(5, 140, 50, 140, 5, 175);
  ucg.setColor(0, 0, 205);
  ucg.drawTriangle(50, 140, 5, 175, 50, 175);
  printScoreDraw();

  // draw initial selection frame
  ucg.setColor(0, 255, 0);
  ucg.drawFrame(168, 88, 64, 64);

  gameSelection_x = 1;
  gameSelection_y = 1;
}

bool EndGameButtonPressed(uint16_t x, uint16_t y) {
  if (x >= 40 && x <= 150 && y >= 110 && y <= 170) {
    // Continue playing selected
    prepareGame(false);
    return true;
  }
  if (x >= 170 && x <= 280 && y >= 110 && y <= 170) {
    isInGameScreen = false;
    drawStartingScreen();
    return true;
  }
  return false;
}

void drawEndGameBox(uint8_t result) {
  bool red_won = false, blue_won = false;
  if ((result == X_WIN_RESULT && color_X == color_red) || (result == O_WIN_RESULT && color_O == color_red)) {
    red_won = true;
    if (is_bot_turn)
      playBotMelody();
    else
      playNokiaTone();
  } else if ((result == X_WIN_RESULT && color_X == color_blue) || (result == O_WIN_RESULT && color_O == color_blue)) {
    blue_won = true;
    if (is_bot_turn)
      playBotMelody();
    else
      playTakeOnMe();
  } else {
    // DRAW
    playPeacefulMelody();
  }

  ucg.setColor(255, 255, 255);
  ucg.drawRBox(30, 30, 260, 160, 15);

  ucg.setColor(138, 43, 226);
  ucg.drawRFrame(30, 30, 260, 160, 15);

  ucg.setFont(ucg_font_courB18_mr);
  if (red_won) {
    ucg.setColor(255, 0, 0);
    ucg.setPrintPos(100, 70);
    ucg.print("RED WON!");
  } else if (blue_won) {
    ucg.setColor(0, 0, 205);
    ucg.setPrintPos(92, 70);
    ucg.print("BLUE WON!");
  } else {
    ucg.setColor(255, 215, 0);
    ucg.setPrintPos(122, 70);
    ucg.print("DRAW!");
  }

  ucg.setColor(138, 43, 226);
  ucg.drawRBox(40, 110, 110, 60, 5);
  ucg.drawRBox(170, 110, 110, 60, 5);

  ucg.setFont(ucg_font_courB14_mr);
  ucg.setColor(255, 255, 255);

  ucg.setPrintPos(51, 135);
  ucg.print("Continue");
  ucg.setPrintPos(56, 156);
  ucg.print("playing");

  ucg.setPrintPos(186, 128);
  ucg.print("Go back");
  ucg.setPrintPos(214, 146);
  ucg.print("to");
  ucg.setPrintPos(173, 164);
  ucg.print("main menu");

  // draw initial selection frame
  ucg.setColor(0, 255, 0);
  ucg.drawRFrame(37, 107, 116, 66, 5);

  isInGameScreen = false;
  isInEndGameScreen = true;

  endGameSelection = 0;
  // wait for a button selection
  while (!selection_button) {
    if (touch.isTouching()) {
      uint16_t x, y;
      touch.getPosition(x, y);
      if (EndGameButtonPressed(x, y))
        return;
    }
  }
  selection_button = false;
  isInEndGameScreen = false;
  // selection button was pressed
  if (endGameSelection == 0) {
    // "Continue playing" selected
    prepareGame(false);
  } else {
    // "Go back to main menu" selected
    drawStartingScreen();
  }
}

void prepareGame(bool reset) {
  for (uint8_t i = 0; i < 3; i++)
    for (uint8_t j = 0; j < 3; j++)
      gameMap[i][j] = ' ';
  is_X_turn = true;
  no_moves = 0;
  if (reset) {
    score_blue = 0;
    score_red = 0;
    score_draw = 0;
    if (gameMode > 0)
      is_bot_turn = true;
    else
      is_bot_turn = false;
    if (gameMode == 0) {
      color_X = color_red;
      color_O = color_blue;
      redToMove = true;
    } else {
      if (is_bot_red) {
        color_X = color_red;
        color_O = color_blue;
        redToMove = true;
      } else {
        color_X = color_blue;
        color_O = color_red;
        redToMove = false;
      }
    }
    last_bot_symbol = 'x';
  } else {
    uint8_t *aux = color_X;
    color_X = color_O;
    color_O = aux;
    redToMove = (color_X == color_red);
    if (gameMode > 0) {
      if (last_bot_symbol == 'x') {
        is_bot_turn = false;
        last_bot_symbol = 'o';
      } else {
        is_bot_turn = true;
        last_bot_symbol = 'x';
      }
    } else {
      is_bot_turn = false;
    }
  }
  drawGameScreen();
  isInGameScreen = true;
  isInStartingScreen = false;
  isInEndGameScreen = false;
  isInColorChoiceScreen = false;
}

bool getMapPosition(uint16_t &x, uint16_t &y) {
  // check if the touch was not on the map
  if (x <= 95 || x >= 305 || y <=  15 || y >= 225)
    return false;

  x = (x - 95) / 70;
  y = (y - 15) / 70;
  return true;
}

void change_score_X() {
  if (color_X[0] == 255) {
    score_red++;
    printScoreRed();
  } else {
    score_blue++;
    printScoreBlue();
  }
}

void change_score_O() {
  if (color_O[0] == 255) {
    score_red++;
    printScoreRed();
  } else {
    score_blue++;
    printScoreBlue();
  }
}

void change_score_draw() {
  score_draw++;
  printScoreDraw();
}

bool check_X_win() {
  // check if X won
  // check lines
  for (int i = 0; i < 3; i++)
    if (gameMap[i][0] == 'x' && gameMap[i][1] == 'x' && gameMap[i][2] == 'x') {
      // draw a winning line
      ucg.setColor(color_X[0], color_X[1], color_X[2]);
      ucg.drawHLine(95, 50 + (i * 70), 210);

      change_score_X();
      return true;
    }
  // check columns
  for (int j = 0; j < 3; j++)
    if (gameMap[0][j] == 'x' && gameMap[1][j] == 'x' && gameMap[2][j] == 'x') {
      // draw a winning line
      ucg.setColor(color_X[0], color_X[1], color_X[2]);
      ucg.drawVLine(130 + (j * 70), 15, 210);

      change_score_X();
      return true;
    }
  // check diagonals
  if (gameMap[0][0] == 'x' && gameMap[1][1] == 'x' && gameMap[2][2] == 'x') {
    // draw a winning line
    ucg.setColor(color_X[0], color_X[1], color_X[2]);
    ucg.drawLine(95, 15, 305, 225);

    change_score_X();
    return true;
  }

  if (gameMap[0][2] == 'x' && gameMap[1][1] == 'x' && gameMap[2][0] == 'x') {
    // draw a winning line
    ucg.setColor(color_X[0], color_X[1], color_X[2]);
    ucg.drawLine(95, 225, 305, 15);

    change_score_X();
    return true;
  }
  return false;
}

bool check_O_win() {
  // check if O won
  // check lines
  for (int i = 0; i < 3; i++)
    if (gameMap[i][0] == 'o' && gameMap[i][1] == 'o' && gameMap[i][2] == 'o') {
      // draw a winning line
      ucg.setColor(color_O[0], color_O[1], color_O[2]);
      ucg.drawHLine(95, 50 + (i * 70), 210);

      change_score_O();
      return true;
    }
  // check columns
  for (int j = 0; j < 3; j++)
    if (gameMap[0][j] == 'o' && gameMap[1][j] == 'o' && gameMap[2][j] == 'o') {
      // draw a winning line
      ucg.setColor(color_O[0], color_O[1], color_O[2]);
      ucg.drawVLine(130 + (j * 70), 15, 210);

      change_score_O();
      return true;
    }
  // check diagonals
  if (gameMap[0][0] == 'o' && gameMap[1][1] == 'o' && gameMap[2][2] == 'o') {
    // draw a winning line
    ucg.setColor(color_O[0], color_O[1], color_O[2]);
    ucg.drawLine(95, 15, 305, 225);

    change_score_O();
    return true;
  }
  if (gameMap[0][2] == 'o' && gameMap[1][1] == 'o' && gameMap[2][0] == 'o') {
    // draw a winning line
    ucg.setColor(color_O[0], color_O[1], color_O[2]);
    ucg.drawLine(95, 225, 305, 15);

    change_score_O();
    return true;
  }
  return false;
}

void make_X_move(uint8_t i, uint8_t j) {
  // draw X
  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
  ucg.setFont(ucg_font_logisoso58_tr);
  ucg.setColor(color_X[0], color_X[1], color_X[2]);
  ucg.setPrintPos(x_coord[j], y_coord[i]);
  ucg.print("x");

  no_moves++;
  is_X_turn = !is_X_turn;
  redToMove = !redToMove;
  gameMap[i][j] = 'x';
  if (check_X_win()) {
    drawEndGameBox(X_WIN_RESULT);
  } else if (no_moves == 9) {
    change_score_draw();
    drawEndGameBox(DRAW_RESULT);
  } else if (gameMode > 0) {
    is_bot_turn = !is_bot_turn;
  }
}

void make_O_move(uint8_t i, uint8_t j) {
  // draw O
  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
  ucg.setFont(ucg_font_logisoso58_tr);
  ucg.setColor(color_O[0], color_O[1], color_O[2]);
  ucg.setPrintPos(x_coord[j], y_coord[i]);
  ucg.print("o");

  no_moves++;
  is_X_turn = !is_X_turn;
  redToMove = !redToMove;
  gameMap[i][j] = 'o';
  if (check_O_win()) {
    drawEndGameBox(O_WIN_RESULT);
  } else if (gameMode > 0) {
    is_bot_turn = !is_bot_turn;
  }
}


/*            BOT DIFFICULTIES FUNCTIONS              */

bool findWinningOpportunity(char symbol, uint8_t &i, uint8_t &j) {
  i = j = -1;
  // check lines
  for (uint8_t k = 0; k < 3; k++) {
    if (gameMap[k][0] == symbol && gameMap[k][1] == symbol && gameMap[k][2] == ' ') {
      i = k;
      j = 2;
      return true;
    }
    if (gameMap[k][0] == symbol && gameMap[k][1] == ' ' && gameMap[k][2] == symbol) {
      i = k;
      j = 1;
      return true;
    }
    if (gameMap[k][0] == ' ' && gameMap[k][1] == symbol && gameMap[k][2] == symbol) {
      i = k;
      j = 0;
      return true;
    }
  }

  // check columns
  for (uint8_t k = 0; k < 3; k++) {
    if (gameMap[0][k] == symbol && gameMap[1][k] == symbol && gameMap[2][k] == ' ') {
      i = 2;
      j = k;
      return true;
    }
    if (gameMap[0][k] == symbol && gameMap[1][k] == ' ' && gameMap[2][k] == symbol) {
      i = 1;
      j = k;
      return true;
    }
    if (gameMap[0][k] == ' ' && gameMap[1][k] == symbol && gameMap[2][k] == symbol) {
      i = 0;
      j = k;
      return true;
    }
  }

  // check first diagonal
  if (gameMap[0][0] == symbol && gameMap[1][1] == symbol && gameMap[2][2] == ' ') {
    i = 2;
    j = 2;
    return true;
  }
  if (gameMap[0][0] == symbol && gameMap[1][1] == ' ' && gameMap[2][2] == symbol) {
    i = 1;
    j = 1;
    return true;
  }
  if (gameMap[0][0] == ' ' && gameMap[1][1] == symbol && gameMap[2][2] == symbol) {
    i = 0;
    j = 0;
    return true;
  }

  // check second diagonal
  if (gameMap[0][2] == symbol && gameMap[1][1] == symbol && gameMap[2][0] == ' ') {
    i = 2;
    j = 0;
    return true;
  }
  if (gameMap[0][2] == symbol && gameMap[1][1] == ' ' && gameMap[2][0] == symbol) {
    i = 1;
    j = 1;
    return true;
  }
  if (gameMap[0][2] == ' ' && gameMap[1][1] == symbol && gameMap[2][0] == symbol) {
    i = 0;
    j = 2;
    return true;
  }
  return false;
}

void makeEasyMove() {
  // find possible moves
  uint8_t no_possible_moves = 0, i, j;
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      if (gameMap[i][j] == ' ')
        no_possible_moves++;

  uint8_t choice = random(no_possible_moves);
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      if (gameMap[i][j] == ' ') {
        if (choice == 0) {
          if (is_X_turn) {
            make_X_move(i, j);
          } else {
            make_O_move(i, j);
          }
          return;
        }
        choice--;
      }
}

void makeMediumMove() {
  uint8_t i, j;
  bool can_win, can_lose;
  if (is_X_turn) {
    can_win = findWinningOpportunity('x', i, j);
    if (can_win) {
      make_X_move(i, j);
      return;
    }
    can_lose = findWinningOpportunity('o', i, j);
    if (can_lose)
      make_X_move(i, j);
    else
      makeEasyMove();
  } else {
    can_win = findWinningOpportunity('o', i, j);
    if (can_win) {
      make_O_move(i, j);
      return;
    }
    can_lose = findWinningOpportunity('x', i, j);
    if (can_lose)
      make_O_move(i, j);
    else
      makeEasyMove();
  }
}

bool checkLineWinOpportunity(char symbol, uint8_t a, uint8_t b, uint8_t &threat_b) {
  uint8_t symbol_app = 0;
  uint8_t space_app = 0;
  uint8_t space_id;
  for (uint8_t i = 0; i < 3; i++) {
    if (i != b && gameMap[a][i] == symbol)
      symbol_app++;
    if (i != b && gameMap[a][i] == ' ') {
      space_app++;
      space_id = i;
    }
  }
  if (symbol_app == 1 && space_app == 1) {
    threat_b = space_id;
    return true;
  }
  return false;
}

bool checkColumnWinOpportunity(char symbol, uint8_t a, uint8_t b, uint8_t &threat_a) {
  uint8_t symbol_app = 0;
  uint8_t space_app = 0;
  uint8_t space_id;
  for (uint8_t i = 0; i < 3; i++) {
    if (i != a && gameMap[i][b] == symbol)
      symbol_app++;
    if (i != a && gameMap[i][b] == ' ') {
      space_app++;
      space_id = i;
    }
  }
  if (symbol_app == 1 && space_app == 1) {
    threat_a = space_id;
    return true;
  }
  return false;
}

bool checkFirstDiagonalOpportunity(char symbol, uint8_t a, uint8_t b, uint8_t &threat_a, uint8_t &threat_b) {
  if (a != b)
    return false;
  uint8_t symbol_app = 0;
  uint8_t space_app = 0;
  uint8_t space_id;
  for (uint8_t i = 0; i < 3; i++) {
    if (i != a && gameMap[i][i] == symbol)
      symbol_app++;
    if (i != a && gameMap[i][i] == ' ') {
      space_app++;
      space_id = i;
    }
  }
  if (symbol_app == 1 && space_app == 1) {
    threat_a = space_id;
    threat_b = space_id;
    return true;
  }
  return false;
}

bool checkSecondDiagonalOpportunity(char symbol, uint8_t &a, uint8_t &b, uint8_t &threat_a, uint8_t &threat_b) {
  if (a + b != 2)
    return false;
  uint8_t symbol_app = 0;
  uint8_t space_app = 0;
  uint8_t space_id;
  for (uint8_t i = 0; i < 3; i++) {
    if (i != a && gameMap[i][2 - i] == symbol)
      symbol_app++;
    if (i != a && gameMap[i][2 - i] == ' ') {
      space_app++;
      space_id = i;
    }
  }
  if (symbol_app == 1 && space_app == 1) {
    threat_a = space_id;
    threat_b = 2 - space_id;
    return true;
  }
  return false;
}

uint8_t checkDiagonalWinOpportunities(char symbol, uint8_t a, uint8_t b) {
  uint8_t no_opportunities = 0;
  uint8_t unused_1, unused_2;
  if (checkFirstDiagonalOpportunity(symbol, a, b, unused_1, unused_2))
    no_opportunities++;
  if (checkSecondDiagonalOpportunity(symbol, a, b, unused_1, unused_2))
    no_opportunities++;
  return no_opportunities;
}

bool isForkMove(char symbol, uint8_t i, uint8_t j) {
  // check if a fork would be created
  uint8_t no_win_opportunities = checkDiagonalWinOpportunities(symbol, i, j);
  if (no_win_opportunities == 2) {
    return true;
  }
  uint8_t unused;
  if (checkLineWinOpportunity(symbol, i, j, unused)) {
    no_win_opportunities++;
    if (no_win_opportunities == 2) {
      return true;
    }
  }
  if (checkColumnWinOpportunity(symbol, i, j, unused)) {
    no_win_opportunities++;
    if (no_win_opportunities == 2) {
      return true;
    }
  }
  return false;
}

bool findForkOpportunity(char symbol, uint8_t &a, uint8_t &b) {
  // check all the free squares for a fork opportunity
  for (uint8_t i = 0; i < 3; i++)
    for (uint8_t j = 0; j < 3; j++) {
      if (gameMap[i][j] == ' ') {
        if (isForkMove(symbol, i, j)) {
          a = i;
          b = j;
          return true;
        }
      }
    }
  return false;
}

bool findForkBlock(char symbol, uint8_t &a, uint8_t &b) {
  uint8_t threat_i, threat_j;
  char counter_symbol = (symbol == 'x') ? 'o' : 'x';
  // try to block opponent's fork
  for (uint8_t i = 0; i < 3; i++)
    for (uint8_t j = 0; j < 3; j++) {
      if (gameMap[i][j] == ' ') {
        if (checkLineWinOpportunity(symbol, i, j, threat_j) && !isForkMove(counter_symbol, i, threat_j)) {
          a = i;
          b = j;
          return true;
        }
        if (checkColumnWinOpportunity(symbol, i, j, threat_i) && !isForkMove(counter_symbol, threat_i, j)) {
          a = i;
          b = j;
          return true;
        }
        if (checkFirstDiagonalOpportunity(symbol, i, j, threat_i, threat_j) && !isForkMove(counter_symbol, threat_i, threat_j)) {
          a = i;
          b = j;
          return true;
        }
        if (checkSecondDiagonalOpportunity(symbol, i, j, threat_i, threat_j) && !isForkMove(counter_symbol, threat_i, threat_j)) {
          a = i;
          b = j;
          return true;
        }
      }
    }
  return false;
}

void makeHardMove() {
  uint8_t i, j;
  bool can_win, can_lose, can_fork, player_can_fork;
  if (is_X_turn) {
    // BOT IS X
    // check if the bot can win or if the bot can lose in one move
    can_win = findWinningOpportunity('x', i, j);
    if (can_win) {
      make_X_move(i, j);
      return;
    }
    can_lose = findWinningOpportunity('o', i, j);
    if (can_lose) {
      make_X_move(i, j);
      return;
    }
    // if the bot can't win and the player can't win, try to create a fork
    can_fork = findForkOpportunity('x', i, j);
    if (can_fork) {
      make_X_move(i, j);
      return;
    }
    // if the player can make a fork, try to block it
    player_can_fork = findForkOpportunity('o', i, j);
    if (player_can_fork) {
      // block the fork
      if (findForkBlock('x', i, j)) {
        make_X_move(i, j);
      } else {
        // opponent is winning, no block possible, make random move
        makeEasyMove();
      }
      return;
    }
    // try to play the center
    if (gameMap[1][1] == ' ') {
      make_X_move(1, 1);
      return;
    }
    // try to play an opposite corner
    if (gameMap[0][0] == 'o' && gameMap[2][2] == ' ') {
      make_X_move(2, 2);
      return;
    }
    if (gameMap[2][2] == 'o' && gameMap[0][0] == ' ') {
      make_X_move(0, 0);
      return;
    }
    if (gameMap[0][2] == 'o' && gameMap[2][0] == ' ') {
      make_X_move(2, 0);
      return;
    }
    if (gameMap[2][0] == 'o' && gameMap[0][2] == ' ') {
      make_X_move(0, 2);
      return;
    }
    // try to play any corner
    if (gameMap[0][0] == ' ') {
      make_X_move(0, 0);
      return;
    }
    if (gameMap[0][2] == ' ') {
      make_X_move(0, 2);
      return;
    }
    if (gameMap[2][0] == ' ') {
      make_X_move(2, 0);
      return;
    }
    if (gameMap[2][2] == ' ') {
      make_X_move(2, 2);
      return;
    }
    makeEasyMove();


  } else {
    // BOT IS O
    // check if the bot can win or if the bot can lose in one move
    can_win = findWinningOpportunity('o', i, j);
    if (can_win) {
      make_O_move(i, j);
      return;
    }
    can_lose = findWinningOpportunity('x', i, j);
    if (can_lose) {
      make_O_move(i, j);
      return;
    }
    // if the bot can't win and the player can't win, try to create a fork
    can_fork = findForkOpportunity('o', i, j);
    if (can_fork) {
      make_O_move(i, j);
      return;
    }
    // if the player can make a fork, try to block it
    player_can_fork = findForkOpportunity('x', i, j);
    if (player_can_fork) {
      // block the fork
      if (findForkBlock('o', i, j)) {
        make_O_move(i, j);
      } else {
        // opponent is winning, no block possible, make random move
        makeEasyMove();
      }
      return;
    }
    // try to play the center
    if (gameMap[1][1] == ' ') {
      make_O_move(1, 1);
      return;
    }
    // try to play an opposite corner
    if (gameMap[0][0] == 'x' && gameMap[2][2] == ' ') {
      make_O_move(2, 2);
      return;
    }
    if (gameMap[2][2] == 'x' && gameMap[0][0] == ' ') {
      make_O_move(0, 0);
      return;
    }
    if (gameMap[0][2] == 'x' && gameMap[2][0] == ' ') {
      make_O_move(2, 0);
      return;
    }
    if (gameMap[2][0] == 'x' && gameMap[0][2] == ' ') {
      make_O_move(0, 2);
      return;
    }
    // try to play any corner
    if (gameMap[0][0] == ' ') {
      make_O_move(0, 0);
      return;
    }
    if (gameMap[0][2] == ' ') {
      make_O_move(0, 2);
      return;
    }
    if (gameMap[2][0] == ' ') {
      make_O_move(2, 0);
      return;
    }
    if (gameMap[2][2] == ' ') {
      make_O_move(2, 2);
      return;
    }
    makeEasyMove();
  }
}


/*                INTERRUPTIONS FUNCTIONS               */

volatile long ts = 0;

ISR(INT0_vect){
  // RED_SELECT_BUTTON INTERRUPTION
  cli();
  if (millis() - ts > 300) {
    ts = millis();
    if (buzzer_playing)
      buzzer_playing = false;
    if (isInStartingScreen || (isInGameScreen && redToMove && (gameMode == 0 || !is_bot_red)) || isInEndGameScreen)
      selection_button = true;
    else if (isInColorChoiceScreen)
      color_selected = 1;
  }
  sei();
}
ISR(INT1_vect){
  // RED_RIGHT_BUTTON INTERRUPTION
  cli();
  if (millis() - ts > 300) {
    ts = millis();
    if (buzzer_playing)
      buzzer_playing = false;
    if (isInStartingScreen) {
      if (startingSelection == 0) {
        startingSelection = 1;
        // draw new selection frame and clear the previous one
        ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
        ucg.drawRFrame(37, 122, 116, 51, 10);

        ucg.setColor(0, 255, 0);
        ucg.drawRFrame(167, 67, 116, 51, 10);
      }
    } else if (isInGameScreen && redToMove && (gameMode == 0 || !is_bot_red)) {
      if (gameSelection_x < 2) {
        // draw new selection frame and clear the previous one
        ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
        ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);

        gameSelection_x++;
        ucg.setColor(0, 255, 0);
        ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
      }
    } else if (isInEndGameScreen && endGameSelection == 0) {
      // draw new selection frame and clear the previous one
      ucg.setColor(255, 255, 255);
      ucg.drawRFrame(37, 107, 116, 66, 5);

      ucg.setColor(0, 255, 0);
      ucg.drawRFrame(167, 107, 116, 66, 5);
      endGameSelection = 1;
    }
  }
  sei();
}
ISR(INT2_vect){
  // RED_UP_BUTTON INTERRUPTION
  cli();
  if (millis() - ts > 300) {
    ts = millis();
    if (buzzer_playing)
      buzzer_playing = false;
    if (isInStartingScreen) {
      if (startingSelection == 2) {
        startingSelection = 1;
        // draw new selection frame and clear the previous one
        ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
        ucg.drawRFrame(167, 122, 116, 51, 10);

        ucg.setColor(0, 255, 0);
        ucg.drawRFrame(167, 67, 116, 51, 10);

      } else if (startingSelection == 3) {
        startingSelection = 2;
        // draw new selection frame and clear the previous one
        ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
        ucg.drawRFrame(167, 177, 116, 51, 10);

        ucg.setColor(0, 255, 0);
        ucg.drawRFrame(167, 122, 116, 51, 10);
      }
    } else if (isInGameScreen && redToMove && (gameMode == 0 || !is_bot_red)) {
      if (gameSelection_y > 0) {
        // draw new selection frame and clear the previous one
        ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
        ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);

        gameSelection_y--;
        ucg.setColor(0, 255, 0);
        ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
      }
    }
  }
  sei();
}
ISR(INT3_vect){
  // RED_LEFT_BUTTON INTERRUPTION
  cli();
  if (millis() - ts > 300) {
    ts = millis();
    if (buzzer_playing)
      buzzer_playing = false;
    if (isInStartingScreen) {
      if (startingSelection == 1) {
        startingSelection = 0;
        // draw new selection frame and clear the previous one
        ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
        ucg.drawRFrame(167, 67, 116, 51, 10);

        ucg.setColor(0, 255, 0);
        ucg.drawRFrame(37, 122, 116, 51, 10);
        
      } else if (startingSelection == 2) {
        startingSelection = 0;
        // draw new selection frame and clear the previous one
        ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
        ucg.drawRFrame(167, 122, 116, 51, 10);

        ucg.setColor(0, 255, 0);
        ucg.drawRFrame(37, 122, 116, 51, 10);

      } else if (startingSelection == 3) {
        startingSelection = 0;
        // draw new selection frame and clear the previous one
        ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
        ucg.drawRFrame(167, 177, 116, 51, 10);

        ucg.setColor(0, 255, 0);
        ucg.drawRFrame(37, 122, 116, 51, 10);
      }
    } else if (isInGameScreen && redToMove && (gameMode == 0 || !is_bot_red)) {
      if (gameSelection_x > 0) {
        // draw new selection frame and clear the previous one
        ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
        ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);

        gameSelection_x--;
        ucg.setColor(0, 255, 0);
        ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
      }
    } else if (isInEndGameScreen && endGameSelection == 1) {
      // draw new selection frame and clear the previous one
      ucg.setColor(255, 255, 255);
      ucg.drawRFrame(167, 107, 116, 66, 5);

      ucg.setColor(0, 255, 0);
      ucg.drawRFrame(37, 107, 116, 66, 5);
      endGameSelection = 0;
    }
  }
  sei();
}
ISR(INT4_vect){
  // RED_DOWN_BUTTON INTERRUPTION
  cli();
  if (millis() - ts > 300) {
    ts = millis();
    if (buzzer_playing)
      buzzer_playing = false;
    if (isInStartingScreen) {
      if (startingSelection == 1) {
        startingSelection = 2;
        // draw new selection frame and clear the previous one
        ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
        ucg.drawRFrame(167, 67, 116, 51, 10);

        ucg.setColor(0, 255, 0);
        ucg.drawRFrame(167, 122, 116, 51, 10);
        
      } else if (startingSelection == 2) {
        startingSelection = 3;
        // draw new selection frame and clear the previous one
        ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
        ucg.drawRFrame(167, 122, 116, 51, 10);

        ucg.setColor(0, 255, 0);
        ucg.drawRFrame(167, 177, 116, 51, 10);

      }
    } else if (isInGameScreen && redToMove && (gameMode == 0 || !is_bot_red)) {
      if (gameSelection_y < 2) {
        // draw new selection frame and clear the previous one
        ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
        ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);

        gameSelection_y++;
        ucg.setColor(0, 255, 0);
        ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
      }
    }
  }
  sei();
}

ISR(PCINT0_vect){
  cli();
  if ((PINB & (1 << PB4)) == 0){
    if (digitalRead(BLUE_SELECT_BUTTON) == LOW) {
      if (millis() - ts > 300) {
        // BLUE_SELECT_BUTTON INTERRUPTION
        ts = millis();
        if (buzzer_playing)
          buzzer_playing = false;
        if (isInStartingScreen || (isInGameScreen && !redToMove && (gameMode == 0 || is_bot_red)) || isInEndGameScreen)
          selection_button = true;
        else if (isInColorChoiceScreen)
          color_selected = 2;
      }
    }
  } else if ((PINB & (1 << PB5)) == 0){
    if (digitalRead(BLUE_RIGHT_BUTTON) == LOW) {
      if (millis() - ts > 300) {
        ts = millis();
        if (buzzer_playing)
          buzzer_playing = false;
        // BLUE_RIGHT_BUTTON INTERRUPTION
        if (isInStartingScreen) {
          if (startingSelection == 0) {
            startingSelection = 1;
            // draw new selection frame and clear the previous one
            ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
            ucg.drawRFrame(37, 122, 116, 51, 10);
    
            ucg.setColor(0, 255, 0);
            ucg.drawRFrame(167, 67, 116, 51, 10);
          }
        } else if (isInGameScreen && !redToMove && (gameMode == 0 || is_bot_red)) {
          if (gameSelection_x < 2) {
            // draw new selection frame and clear the previous one
            ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
            ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
    
            gameSelection_x++;
            ucg.setColor(0, 255, 0);
            ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
          }
        }  else if (isInEndGameScreen && endGameSelection == 0) {
          // draw new selection frame and clear the previous one
          ucg.setColor(255, 255, 255);
          ucg.drawRFrame(37, 107, 116, 66, 5);
    
          ucg.setColor(0, 255, 0);
          ucg.drawRFrame(167, 107, 116, 66, 5);
          endGameSelection = 1;
        }
      }
    }
  } else if ((PINB & (1 << PB6)) == 0){
    if (digitalRead(BLUE_UP_BUTTON) == LOW) {
      if (millis() - ts > 300) {
        // BLUE_UP_BUTTON INTERRUPTION
        ts = millis();
        if (buzzer_playing)
          buzzer_playing = false;
        if (isInStartingScreen) {
          if (startingSelection == 2) {
            startingSelection = 1;
            // draw new selection frame and clear the previous one
            ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
            ucg.drawRFrame(167, 122, 116, 51, 10);
    
            ucg.setColor(0, 255, 0);
            ucg.drawRFrame(167, 67, 116, 51, 10);
    
          } else if (startingSelection == 3) {
            startingSelection = 2;
            // draw new selection frame and clear the previous one
            ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
            ucg.drawRFrame(167, 177, 116, 51, 10);
    
            ucg.setColor(0, 255, 0);
            ucg.drawRFrame(167, 122, 116, 51, 10);
          }
        } else if (isInGameScreen && !redToMove && (gameMode == 0 || is_bot_red)) {
          if (gameSelection_y > 0) {
            // draw new selection frame and clear the previous one
            ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
            ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
    
            gameSelection_y--;
            ucg.setColor(0, 255, 0);
            ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
          }
        }
      }
    }
  } else if ((PINB & (1 << PB7)) == 0){
    if (digitalRead(BLUE_LEFT_BUTTON) == LOW) {
      if (millis() - ts > 300) {
        // BLUE_LEFT_BUTTON INTERRUPTION
        ts = millis();
        if (buzzer_playing)
          buzzer_playing = false;
        if (isInStartingScreen) {
          if (startingSelection == 1) {
            startingSelection = 0;
            // draw new selection frame and clear the previous one
            ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
            ucg.drawRFrame(167, 67, 116, 51, 10);
    
            ucg.setColor(0, 255, 0);
            ucg.drawRFrame(37, 122, 116, 51, 10);
            
          } else if (startingSelection == 2) {
            startingSelection = 0;
            // draw new selection frame and clear the previous one
            ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
            ucg.drawRFrame(167, 122, 116, 51, 10);
    
            ucg.setColor(0, 255, 0);
            ucg.drawRFrame(37, 122, 116, 51, 10);
    
          } else if (startingSelection == 3) {
            startingSelection = 0;
            // draw new selection frame and clear the previous one
            ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
            ucg.drawRFrame(167, 177, 116, 51, 10);
    
            ucg.setColor(0, 255, 0);
            ucg.drawRFrame(37, 122, 116, 51, 10);
          }
        } else if (isInGameScreen && !redToMove && (gameMode == 0 || is_bot_red)) {
          if (gameSelection_x > 0) {
            // draw new selection frame and clear the previous one
            ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
            ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
    
            gameSelection_x--;
            ucg.setColor(0, 255, 0);
            ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
          }
        } else if (isInEndGameScreen && endGameSelection == 1) {
          // draw new selection frame and clear the previous one
          ucg.setColor(255, 255, 255);
          ucg.drawRFrame(167, 107, 116, 66, 5);
    
          ucg.setColor(0, 255, 0);
          ucg.drawRFrame(37, 107, 116, 66, 5);
          endGameSelection = 0;
        }
      }
    }
  }
  sei();
}

ISR(PCINT1_vect){
  cli();
  if ((PINJ & (1 << PJ0)) == 0){
    if (digitalRead(BLUE_DOWN_BUTTON) == LOW) {
      if (millis() - ts > 300) {
        // BLUE_DOWN_BUTTON INTERRUPTION
        ts = millis();
        if (buzzer_playing)
          buzzer_playing = false;
        if (isInStartingScreen) {
          if (startingSelection == 1) {
            startingSelection = 2;
            // draw new selection frame and clear the previous one
            ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
            ucg.drawRFrame(167, 67, 116, 51, 10);
    
            ucg.setColor(0, 255, 0);
            ucg.drawRFrame(167, 122, 116, 51, 10);
            
          } else if (startingSelection == 2) {
            startingSelection = 3;
            // draw new selection frame and clear the previous one
            ucg.setColor(color_startingScreen[0], color_startingScreen[1], color_startingScreen[2]);
            ucg.drawRFrame(167, 122, 116, 51, 10);
    
            ucg.setColor(0, 255, 0);
            ucg.drawRFrame(167, 177, 116, 51, 10);
    
          }
        } else if (isInGameScreen && !redToMove && (gameMode == 0 || is_bot_red)) {
          if (gameSelection_y < 2) {
            // draw new selection frame and clear the previous one
            ucg.setColor(color_gameScreen[0], color_gameScreen[1], color_gameScreen[2]);
            ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
    
            gameSelection_y++;
            ucg.setColor(0, 255, 0);
            ucg.drawFrame(98 + 70 * gameSelection_x, 18 + 70 * gameSelection_y, 64, 64);
          }
        }
      }
    }
  }
  sei();
}

void setupButtons() {
  pinMode(BLUE_SELECT_BUTTON, INPUT_PULLUP);
  pinMode(BLUE_RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(BLUE_DOWN_BUTTON, INPUT_PULLUP);
  pinMode(BLUE_UP_BUTTON, INPUT_PULLUP);
  pinMode(BLUE_LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RED_SELECT_BUTTON, INPUT_PULLUP);
  pinMode(RED_RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(RED_DOWN_BUTTON, INPUT_PULLUP);
  pinMode(RED_UP_BUTTON, INPUT_PULLUP);
  pinMode(RED_LEFT_BUTTON, INPUT_PULLUP);

  // configure PCINT interruptions
  PCICR |= (1 << PCIE0);
  PCICR |= (1 << PCIE1);
  PCMSK0 |= (1 << PCINT4);
  PCMSK0 |= (1 << PCINT5);
  PCMSK0 |= (1 << PCINT6);
  PCMSK0 |= (1 << PCINT7);
  PCMSK1 |= (1 << PCINT9);

  // configure INT interruptions
  EIMSK |= (1 << INT0);
  EIMSK |= (1 << INT1);
  EIMSK |= (1 << INT2);
  EIMSK |= (1 << INT3);
  EIMSK |= (1 << INT4);

  EICRA |= (1 << ISC01) | (1 << ISC00);
  EICRA |= (1 << ISC11) | (1 << ISC10);
  EICRA |= (1 << ISC21) | (1 << ISC20);
  EICRA |= (1 << ISC31) | (1 << ISC30);
  EICRB |= (1 << ISC41) | (1 << ISC40);
  sei();
}


/*                 BUZZER FUNCTIONS               */

void playBotMelody() {
  int tempo = 114;
  int notes = sizeof(bot_melody) / sizeof(bot_melody[0]) / 2;
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;

  buzzer_playing = true;
  digitalWrite(buzzer, LOW);
  for (int thisNote = 0; thisNote < notes * 2 && buzzer_playing; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = bot_melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, bot_melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(buzzer);
  }
  digitalWrite(buzzer, HIGH);
  buzzer_playing = false;
}

void playNokiaTone() {
  int tempo = 180;
  int notes = sizeof(nokia_melody) / sizeof(nokia_melody[0]) / 2;
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;

  buzzer_playing = true;
  digitalWrite(buzzer, LOW);
  for (int thisNote = 0; thisNote < notes * 2 && buzzer_playing; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = nokia_melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, nokia_melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(buzzer);
  }
  digitalWrite(buzzer, HIGH);
  buzzer_playing = false;
}

void playTakeOnMe() {
  int tempo = 140;
  int notes = sizeof(takeOnMe_melody) / sizeof(takeOnMe_melody[0]) / 2;
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;
  buzzer_playing = true;
  digitalWrite(buzzer, LOW);
  for (int thisNote = 0; thisNote < notes * 2 && buzzer_playing; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = takeOnMe_melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, takeOnMe_melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(buzzer);
  }
  digitalWrite(buzzer, HIGH);
  buzzer_playing = false;
}

void playPeacefulMelody() {
  int tempo = 140;
  int notes = sizeof(peaceful_melody) / sizeof(peaceful_melody[0]) / 2;
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;
  buzzer_playing = true;
  digitalWrite(buzzer, LOW);
  for (int thisNote = 0; thisNote < notes * 2 && buzzer_playing; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = peaceful_melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, peaceful_melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(buzzer);
  }
  digitalWrite(buzzer, HIGH);
  buzzer_playing = false;
}

void playWrongMoveTone() {
  digitalWrite(buzzer, LOW);
  tone(buzzer, 8, 150);
  delay(150);
  noTone(buzzer);
  digitalWrite(buzzer, HIGH);

  digitalWrite(buzzer, LOW);
  tone(buzzer, 8, 250);
  delay(250);
  noTone(buzzer);
  digitalWrite(buzzer, HIGH);
}
