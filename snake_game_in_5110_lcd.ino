#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

// #include <Adafruit_PCD8544.h>
#include <string.h>
#include <stdlib.h>
#define DEBUG 0
#define SCREEN 0
#define USING_VOLUME_LIB 0
#define USING_EEPROM 1
#define CLEAR_EEPROM 0

#if USING_VOLUME_LIB == 0
  #define DELAY(x) delay(x)
  #define DELAY_MICROSECONDS(x) delayMicroseconds(x)
  #define MILLIS(x) millis(x)
#else
  #include "Volume.h"
  Volume vol;
  #define DELAY(x) vol.delay(x)
  #define DELAY_MICROSECONDS(x) vol.delayMicroseconds(x)
  #define MILLIS(x) vol.millis(x)
#endif

#if SCREEN == 0
  #include <Adafruit_PCD8544.h>
#elif SCREEN == 1
  #include <Adafruit_SSD1306.h>
#elif SCREEN == 2
  #include <Adafruit_SH110X.h>
#endif

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

// declare ic 74hc165n to arduino
#define PL_PIN 9
#define CE_PIN 12
#define QH_PIN 11
#define CP_PIN 10

// declare passive buzzer
#define BUZZER_PIN 5

// declare Potentiometer (POT)
#define POT A0

// 0: lcd 5510
// 1: oled 1360
// 2: oled 1106
#if SCREEN == 0
  #define SCLK_PIN 2
  #define DIN_PIN 4
  #define DC_PIN 6
  #define CS_PIN 7
  #define RST_PIN 8
  #define LED_PIN 3
  #define HIDE WHITE
  #define SHOW BLACK
  #define SCREEN_WIDTH 84 // LCD display width, in pixels
  #define SCREEN_HEIGHT 48 // LCD display height, in pixels
  Adafruit_PCD8544 display = Adafruit_PCD8544(SCLK_PIN, DIN_PIN, DC_PIN, CS_PIN, RST_PIN);
  // define game map rows and colms
  #define ROWS 12
  #define COLMS 21
  // define node snake
  #define NODE_SNAKE_WIDTH 4
  #define NODE_SNAKE_HEIGHT 3
#elif SCREEN == 1
  #define HIDE SSD1306_BLACK
  #define SHOW SSD1306_WHITE
  #define OLED_RESET -1   //   QT-PY / XIAO
  #define SCREEN_ADDRESS 0x3C
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  // define game map rows and colms
  #define ROWS 11
  #define COLMS 20
  // define node snake
  #define NODE_SNAKE_WIDTH 6
  #define NODE_SNAKE_HEIGHT 5
#elif SCREEN == 2
  #define HIDE SH110X_BLACK
  #define SHOW SH110X_WHITE
  #define OLED_RESET -1   //   QT-PY / XIAO
  #define SCREEN_ADDRESS 0x3c
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  // define game map rows and colms
  #define ROWS 11
  #define COLMS 20
  // define node snake
  #define NODE_SNAKE_WIDTH 6
  #define NODE_SNAKE_HEIGHT 5
#endif

//
#define SAVE_MAGIC 0xABCD
#define NUM_MAGIC_1_ADDR 0
#define NUM_MAGIC_2_ADDR 2
#define NUM_MAGIC_3_ADDR 4
#define NUM_MAGIC_4_ADDR 6
#define BRIGHT_LEV_ADDR 8
#define GAME_MODE_ADDR 9
#define SCORES_ADDR 10
#define GAME_STATE_ADDR 298

// declare game menu
#define GAME_SNAKE_MENU 0
#define IN_GAME_MENU 1
#define LEVEL_MENU 2
#define GAME_TYPES_MENU 3
#define HIGH_SCORES_MENU 4
#define BRIGHTNESS_MENU 5

#define MENU_INDEXES_LENGTH 5

// init game map matrix
// another: 0
// button "TOP": 1
// button "DOWN": 2
// button "LEFT": 3
// button "RIGHT": 4
// button "OK": 5
// button "BACK": 6
// wall: 127
// food: 191
// gameover: 0
// eaten: 1
// continue: 2
#define TOP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define OK 5
#define BACK 6
#define FOOD 191
#define WALL 127
#define WILL_BE_GAMEOVER 0
#define WILL_BE_EATEN 1
#define CONTINUE 2

#define TIME_DELAY_LEVEL_1 1000
#define TIME_DELAY_LEVEL_2 850
#define TIME_DELAY_LEVEL_3 650
#define TIME_DELAY_LEVEL_4 550
#define TIME_DELAY_LEVEL_5 350
#define TIME_DELAY_LEVEL_6 250
#define TIME_DELAY_LEVEL_7 200
#define TIME_DELAY_LEVEL_8 150

#define POINTS_LEVEL_1 1
#define POINTS_LEVEL_2 2
#define POINTS_LEVEL_3 3
#define POINTS_LEVEL_4 4
#define POINTS_LEVEL_5 5
#define POINTS_LEVEL_6 6
#define POINTS_LEVEL_7 7
#define POINTS_LEVEL_8 8

// bit 0 - bit 2: future snake direct
// bit 3 - bit 5: current snake direct
// bit 6 - bit 7: 00 or 01
//  000: no infor
//  001: top
//  010: down
//  011: left
//  100: right
// bit 0 - bit 5: 111111
// bit 6 - bit 7: wall, normal food
//  01: wall
//  10: normal food
uint8_t game_map[ROWS][COLMS];

// [0] -> [4]: 
//  bit 0 - bit 3: curr
//  bit 4 - bit 7: prev
//  1111 = -1, 0000 = 0 -> 1110 = 14
// menu_indexes[0]: row selected index 0, 1 or 2
// menu_indexes[1]: title index
// menu_indexes[2]: first index in first row
// menu_indexes[3]: second index in second row
// menu_indexes[4]: third index in third row
byte menu_indexes[MENU_INDEXES_LENGTH] = {0, 0b11111111, 0b11111111, 0b11111111, 0b11111111};

const char menu_0[] PROGMEM = "CONTINUE";
const char menu_1[] PROGMEM = "NEW GAME";
const char menu_2[] PROGMEM = "LEVEL";
const char menu_3[] PROGMEM = "GAME TYPES";
const char menu_4[] PROGMEM = "HIGH SCORES";
const char menu_5[] PROGMEM = "BRIGHTNESS";
const char menu_6[] PROGMEM = "EXIT GAME";
const char menu_7[] PROGMEM = "SNAKE GAME";
const char menu_8[] PROGMEM = "SCORE: ";
const char menu_9[] PROGMEM = "1. CLASSIC";
const char menu_10[] PROGMEM = "2. INFINITY";
const char menu_11[] PROGMEM = "3. TUNNEL";
const char menu_12[] PROGMEM = "4. MILL";
const char menu_13[] PROGMEM = "5. RAILS";
const char menu_14[] PROGMEM = "6. APARTMENT";
const char *const menu_table[] PROGMEM = {menu_0, menu_1, menu_2, menu_3, menu_4, menu_5, menu_6, menu_7, menu_8, menu_9, menu_10, menu_11, menu_12, menu_13, menu_14};

const uint8_t level_hide_bitmap[] PROGMEM= {
  0b10101000,
  0b01010000,
  0b01110000,
  0b01010000,
  0b10101000
};

#if SCREEN == 0
  const uint8_t food_bitmap[] PROGMEM= {
    0b01000000,
    0b10100000,
    0b01000000
  };
#endif

const unsigned char game_start_menu_bitmap [] PROGMEM = {
	// 'Snake - Game start menu screen', 84x48px
	0xff, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xf0, 0xff, 0xe1, 0xff, 0xf1, 0xf3, 
	0xe3, 0xfc, 0x0f, 0xff, 0xff, 0xf0, 0xff, 0xc1, 0xf9, 0xe1, 0xe1, 0xc3, 0xe6, 0x07, 0xff, 0xff, 
	0xf0, 0xff, 0x80, 0xc1, 0xe1, 0xc1, 0x83, 0xc2, 0x07, 0xff, 0xff, 0xf0, 0xff, 0x00, 0xc1, 0xc1, 
	0x81, 0xc3, 0x82, 0x3f, 0xff, 0xff, 0xf0, 0xfe, 0x01, 0xc1, 0xc3, 0x81, 0xc3, 0x84, 0x7f, 0xff, 
	0xff, 0xf0, 0xfe, 0x07, 0xc0, 0xc3, 0x00, 0xf3, 0x09, 0xff, 0xff, 0xff, 0xf0, 0xfc, 0x1f, 0xc0, 
	0xc3, 0x18, 0x90, 0x09, 0x9f, 0xff, 0xff, 0xf0, 0xfc, 0x3f, 0xc0, 0x43, 0x18, 0x08, 0x11, 0x1f, 
	0xff, 0xff, 0xf0, 0xfc, 0x7f, 0xe0, 0x43, 0x1c, 0x08, 0x30, 0x1f, 0xff, 0xff, 0xf0, 0xf8, 0x70, 
	0x62, 0x02, 0x18, 0x18, 0x78, 0x3f, 0xff, 0xff, 0xf0, 0xf8, 0x00, 0x22, 0x02, 0x00, 0x70, 0x38, 
	0x67, 0xff, 0xff, 0xf0, 0xf8, 0x00, 0x23, 0x06, 0x00, 0x60, 0x1c, 0x47, 0xff, 0xff, 0xf0, 0xf8, 
	0x38, 0x23, 0x04, 0x0c, 0x21, 0x0c, 0x07, 0xff, 0xff, 0xf0, 0xff, 0xf0, 0x23, 0x8c, 0x3c, 0x21, 
	0x8e, 0x0f, 0xff, 0xff, 0xf0, 0xff, 0xc0, 0x43, 0xf8, 0x3f, 0xf1, 0xce, 0x3f, 0xff, 0xff, 0xf0, 
	0xff, 0x80, 0xcf, 0xf8, 0x7f, 0x19, 0xfe, 0xff, 0xff, 0xff, 0xf0, 0xfc, 0x01, 0xff, 0xff, 0xfe, 
	0x6f, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xfc, 0x07, 0xff, 0xff, 0x99, 0x90, 0x7f, 0xff, 0xff, 0xff, 
	0xf0, 0xfe, 0x0f, 0xff, 0xff, 0x67, 0x4f, 0xbf, 0xff, 0xff, 0xff, 0xf0, 0xfe, 0x3f, 0xff, 0xff, 
	0x1e, 0xcf, 0xbf, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xfc, 0x02, 0x5c, 0x1f, 0xdf, 0xff, 0xff, 
	0xff, 0xf0, 0xff, 0xff, 0xe2, 0xa8, 0x3b, 0xf0, 0xcf, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0x15, 
	0x53, 0xff, 0xfd, 0xe7, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xfc, 0xaa, 0xa0, 0x07, 0xf8, 0x01, 0xff, 
	0xff, 0xff, 0xf0, 0xff, 0xf9, 0x54, 0x17, 0xfb, 0x00, 0xaa, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xf2, 
	0xa0, 0xf1, 0xfc, 0x3a, 0x55, 0x7f, 0xff, 0xff, 0xf0, 0xff, 0xe5, 0x4f, 0xfa, 0x02, 0x7b, 0xa0, 
	0x07, 0xff, 0xff, 0xf0, 0xff, 0xea, 0x9f, 0xfa, 0xca, 0xf3, 0x9f, 0xf9, 0xe7, 0xff, 0xf0, 0xff, 
	0xd4, 0x3f, 0xfc, 0xcc, 0xe7, 0x7f, 0xfe, 0xd9, 0xff, 0xf0, 0xff, 0xc2, 0x3f, 0xff, 0x9b, 0xce, 
	0xf0, 0x0f, 0x0e, 0xff, 0xf0, 0xfe, 0x05, 0x43, 0xff, 0x57, 0x9d, 0xef, 0xf7, 0xa7, 0x7f, 0xf0, 
	0xfc, 0xa8, 0x21, 0xff, 0xef, 0x3b, 0xdf, 0xfb, 0xa3, 0x7f, 0xf0, 0xfd, 0x53, 0x95, 0xff, 0xde, 
	0x77, 0xbc, 0x13, 0x83, 0x9f, 0xf0, 0xfc, 0x09, 0x00, 0xff, 0xbc, 0xf7, 0x70, 0xc7, 0x81, 0xaf, 
	0xf0, 0xf9, 0x44, 0x15, 0x3f, 0x79, 0xee, 0xe7, 0x00, 0x51, 0xcf, 0xf0, 0xfa, 0xa2, 0xaa, 0x9e, 
	0xf3, 0x2e, 0xeb, 0xff, 0xe8, 0xdf, 0xf0, 0xfc, 0x05, 0x50, 0x0d, 0xe6, 0x2f, 0x65, 0xff, 0xd0, 
	0xdf, 0xf0, 0xff, 0xf0, 0x2a, 0xab, 0xcc, 0x17, 0xb4, 0x7e, 0x20, 0xdf, 0xf0, 0xff, 0xfc, 0x05, 
	0x4b, 0xd8, 0x1b, 0xdb, 0x81, 0xd0, 0xdf, 0xf0, 0xff, 0x07, 0xa0, 0x1b, 0xd0, 0x0d, 0xed, 0xff, 
	0xe0, 0xdf, 0xf0, 0xfc, 0x01, 0xa0, 0x57, 0x9c, 0x06, 0xb6, 0x7e, 0x14, 0xdf, 0xf0, 0xf8, 0x00, 
	0x9d, 0x37, 0xa7, 0xff, 0x72, 0x81, 0xa6, 0xdf, 0xf0, 0xf8, 0x00, 0xfa, 0xf7, 0xf8, 0x00, 0xfa, 
	0xff, 0xc8, 0x3f, 0xf0, 0xfc, 0x00, 0x1f, 0xf9, 0xff, 0xff, 0xf2, 0x3e, 0x3f, 0xef, 0xf0, 0xfe, 
	0x00, 0x00, 0x0c, 0xff, 0xff, 0xf7, 0x81, 0xe0, 0x1f, 0xf0, 0xff, 0x80, 0x00, 0x07, 0x1f, 0xff, 
	0x0c, 0xff, 0x00, 0x7f, 0xf0, 0xff, 0xff, 0x00, 0x01, 0xe0, 0x00, 0xf8, 0x00, 0x3f, 0xff, 0xf0
};

int8_t menu_idx = 1;
// bit 0 - bit 1: start game menu - game menu - gameplay
//  00: start game menu
//  01: game menu
//  10: gameplay
// bit 2: is_menu_init = 0;
// bit 3: is_game_init = 0;
// bit 4: 
// bit 5: 
// bit 6: is_gameplay_rendered = 0
// bit 7: is_first_game_init = 0;
uint8_t game_flag = 0b00000000;

// bit 0 - bit 2: level;
//  000 -> 111: level 1 -> level 8
// bit 3 - bit 5: game type;
// 000: CLASSIC    011: MILL
// 001: INFINITY   100: RAILS
// 010: TUNNEL     101: APARTMENT
// default: level: 4 - game type: classic
// bit 6: is_game_over = 0;
// bit 7: is_food_eaten = 0;
uint8_t game_mode_flag = 0b00000100;

// brightness level: 0 -> 8
uint8_t brightness_level = 4;

// init head position in game_map matrix
uint8_t head_snake_row_x = 0;
uint8_t head_snake_colm_y = 0;
uint8_t prev_head_snake_row_x = 0;
uint8_t prev_head_snake_colm_y = 0;

// init tail position in game_map matrix
uint8_t tail_snake_row_x = 0;
uint8_t tail_snake_colm_y = 0;
uint8_t prev_tail_snake_row_x = 0;
uint8_t prev_tail_snake_colm_y = 0;
uint8_t prev_tail_snake_direct = 0;

// init food position in game_map matrix
uint8_t food_row_x = 0;
uint8_t food_colm_y = 0;
uint8_t prev_food_row_x = 0;
uint8_t prev_food_colm_y = 0;

// init scores
uint16_t curr_score = 0;
uint16_t scores[3] = {0, 0, 0};

// init pot value
uint16_t pot_value = 0;

// init button state
uint8_t btn_state = 0;

unsigned long curr_time_gameplay;
unsigned long time_render_init_game;
unsigned long time_init_menu;

// declare address EPPROM
// store: scores, is_game_init, is_food_eaten, is_game_over, game_mode_flag, brightness_level and game state
// addr 0 (2 bytes): number magic 1 = 0xABCD (check brightness_level is saved)
// addr 2 (2 bytes): number magic 2 = 0xABCD (check game_mode_flag is saved)
// addr 4 (2 bytes): number magic 3 = 0xABCD (check scores is saved)
// addr 6 (2 bytes): number magic 4 = 0xABCD (check game state is saved)
// addr 8 (1 byte): brightness_level
// addr 9 (1 byte): game_mode_flag
// addr 10 (288 bytes) : scores
// addr 298: game_state
struct GameState{
  int8_t game_flag; //is_game_init; bit 0, is_food_eaten: bit 1, is_game_over: bit 2
  int16_t curr_score;
  int8_t head_snake_row_x;
  int8_t head_snake_colm_y;
  int8_t prev_head_snake_row_x;
  int8_t prev_head_snake_colm_y;
  int8_t tail_snake_row_x;
  int8_t tail_snake_colm_y;
  int8_t prev_tail_snake_row_x;
  int8_t prev_tail_snake_colm_y;
  int8_t prev_tail_snake_direct;
  int8_t food_row_x;
  int8_t food_colm_y;
  int8_t prev_food_row_x;
  int8_t prev_food_colm_y;
  int8_t game_map[ROWS][COLMS];
};

// get (x, y) in screen from (x, y) in game map matrix
uint16_t getXPosScreen(uint8_t row_x, uint8_t colm_y);
uint16_t getYPosScreen(uint8_t row_x, uint8_t colm_y);
// handle buttons
uint8_t handleButtons(void);
// eeprom
void writeSaveGame(void);
void loadSaveGame(void);
void loadScores(void);
void writeScores(void);
void writeGameModeFlag(void);
void writeBrightnessLevel(void);
// init
void initMenu(void);
void initGame(void);
void initWall(void);
void initSnake(void);
void initFood(void);
// handle
void gamePlay(uint8_t button_value);
uint8_t checkFutureSnake(uint8_t row_x, uint8_t colm_y, uint8_t direct);
void controlMenu(uint8_t button_value);
void controlGameStartMenu(uint8_t button_value);
void gameOver(void);
// render
void renderGameplay(void);
void renderMenuInit(void);
void renderMenu(void);
void renderGameStartMenu(void);
// sound
void createFoodEatenSound(void);
void createGameOverSound(void);

void setup() {
  Serial.begin(9600);
  memset(game_map, 0, sizeof(game_map));
  randomSeed(analogRead(1));
  // Setup 74HC165n
  pinMode(PL_PIN, OUTPUT);
  pinMode(CE_PIN, OUTPUT);
  pinMode(CP_PIN, OUTPUT);
  pinMode(QH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  analogWrite(LED_PIN, map(brightness_level, 0, 8, 255, 0));

  #if CLEAR_EEPROM == 1
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.write(i, 0); // Đặt lại từng ô nhớ về 0
    }
  #else
    // load game state from EPPROM to SRAM
    loadSaveGame();
    loadScores();
  #endif
  // init done
  #if SCREEN == 0
    display.begin();
  #elif SCREEN == 1
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }
  #elif SCREEN == 2
    display.begin();
  #endif

  // #if USING_EEPROM == 1
  //   uint16_t num_magic_4 = 0;
  //   EEPROM.put(NUM_MAGIC_4_ADDR, num_magic_4);
  // #endif

  display.clearDisplay();
  // initGame();
  // initMenu();
}

void loop() {
  analogWrite(LED_PIN, map(brightness_level, 0, 8, 255, 0));
  uint8_t val = handleButtons();
  // debugln(val);
  // pot_value = analogRead(POT);
  // debugln(brightness_level);
  switch(game_flag & 0b00000011){
    case 0:
      controlGameStartMenu(val);
      DELAY(300);
      break;
    case 1:
      display.invertDisplay(false);
      if(((game_flag >> 2) & 0b00000001) == 0){
        initMenu();
      }
      controlMenu(val);
      DELAY(300);
      break;
    case 2:
      display.invertDisplay(false);
      if(((game_flag >> 7) & 1) == 0){ // is_first_game_init == 0
        initGame();
        game_flag |= 0b10000000; // is_first_game_init = 1;
        // debugln(F("Da init game"));
      }
      if(((game_mode_flag >> 6) & 1) == 0){ // is_game_over == 0
        game_flag |= 0b00001000; // is_game_init = 1;
        // debugln(F("Truoc thuc hien ham gamePlay"));
        gamePlay(val);
        // debugln(F("Da thuc hien ham gamePlay"));
        // printMatrix();
      }
      else{
        gameOver();
        DELAY(1000);
        display.clearDisplay();
        initGame();
      }
      break;
  }
}

#if USING_EEPROM == 1
  void writeSaveGame(void){
    uint16_t num_magic_4 = 0xABCD;
    EEPROM.put(NUM_MAGIC_4_ADDR, num_magic_4);
    GameState game_state;
    game_state.game_flag = ((game_flag >> 3) & 0b00000001) | (((game_mode_flag >> 6) & 0b00000011) << 1);
    game_state.curr_score = curr_score;
    game_state.head_snake_row_x = head_snake_row_x;
    game_state.head_snake_colm_y = head_snake_colm_y;
    game_state.prev_head_snake_row_x = prev_head_snake_row_x;
    game_state.prev_head_snake_colm_y = prev_head_snake_colm_y;
    game_state.tail_snake_row_x = tail_snake_row_x;
    game_state.tail_snake_colm_y = tail_snake_colm_y;
    game_state.prev_tail_snake_row_x = prev_tail_snake_row_x;
    game_state.prev_tail_snake_colm_y = prev_tail_snake_colm_y;
    game_state.prev_tail_snake_direct = prev_tail_snake_direct;
    game_state.food_row_x = food_row_x;
    game_state.food_colm_y = food_colm_y;
    game_state.prev_food_row_x = prev_food_row_x;
    game_state.prev_food_colm_y = prev_food_colm_y;
    for(int8_t i = 0; i < ROWS; i++){
      for(int8_t j = 0; j < COLMS; j++){
        game_state.game_map[i][j] = game_map[i][j];
      }
    }
    EEPROM.put(GAME_STATE_ADDR, game_state);
  }
  void loadSaveGame(void){
    uint16_t num_magics[4];
    EEPROM.get(NUM_MAGIC_1_ADDR, num_magics);
    if(num_magics[0] == SAVE_MAGIC){
      EEPROM.get(BRIGHT_LEV_ADDR, brightness_level);
    }
    if(num_magics[1] == SAVE_MAGIC){
      uint8_t load_game_mode_flag = 0b00000100;
      EEPROM.get(GAME_MODE_ADDR, load_game_mode_flag);
      game_mode_flag = (game_mode_flag & 0b11000000) | (load_game_mode_flag & 0b00111111); 
    }
    if(num_magics[3] == SAVE_MAGIC){
      GameState load_game_state;
      EEPROM.get(GAME_STATE_ADDR, load_game_state);
      game_flag = (game_flag & 0b11110111) | ((load_game_state.game_flag & 0b00000001) << 3); //load is_game_init_value
      if(((game_flag >> 3) & 0b00000001) == 1){ // is_game_init == 1
        game_flag |= 0b10000000; // is_first_game_init = 1
      }
      game_mode_flag = (game_mode_flag & 0b00111111) | (((load_game_state.game_flag >> 1) & 0b00000011) << 6); //load is_game_over and is_food_eaten
      curr_score = load_game_state.curr_score;
      head_snake_row_x = load_game_state.head_snake_row_x;
      head_snake_colm_y = load_game_state.head_snake_colm_y;
      prev_head_snake_row_x = load_game_state.prev_head_snake_row_x;
      prev_head_snake_colm_y = load_game_state.prev_head_snake_colm_y;
      tail_snake_row_x = load_game_state.tail_snake_row_x;
      tail_snake_colm_y = load_game_state.tail_snake_colm_y;
      prev_tail_snake_row_x = load_game_state.prev_tail_snake_row_x;
      prev_tail_snake_colm_y = load_game_state.prev_tail_snake_colm_y;
      prev_tail_snake_direct = load_game_state.prev_tail_snake_direct;
      food_row_x = load_game_state.food_row_x;
      food_colm_y = load_game_state.food_colm_y;
      prev_food_row_x = load_game_state.prev_food_row_x;
      prev_food_colm_y = load_game_state.prev_food_colm_y;
      for(int8_t i = 0; i < ROWS; i++){
        for(int8_t j = 0; j < COLMS; j++){
          game_map[i][j] = load_game_state.game_map[i][j];
        }
      }
    }
  }
  void loadScores(void){
    uint16_t num_magic_3;
    EEPROM.get(NUM_MAGIC_3_ADDR, num_magic_3);
    if(num_magic_3 != SAVE_MAGIC)
      return;
    int addr = SCORES_ADDR;
    int level = (game_mode_flag) & 0b00000111;
    int game_type = (game_mode_flag >> 3) & 0b00000111;
    uint16_t load_scores[3];
    addr += ((game_type * 8 + level) * 6);
    EEPROM.get(addr, load_scores);
    for(int i = 0; i < 3; i++){
      scores[i] = load_scores[i];
    }
  }
  void writeScores(void){
    uint16_t num_magic_3;
    EEPROM.get(NUM_MAGIC_3_ADDR, num_magic_3);
    if(num_magic_3 != SAVE_MAGIC){
      num_magic_3 = 0xABCD;
      uint16_t full_scores[144] = {0};
      EEPROM.put(NUM_MAGIC_3_ADDR, num_magic_3);
      EEPROM.put(SCORES_ADDR, full_scores);
    }
    else{
      int addr = SCORES_ADDR;
      int level = (game_mode_flag) & 0b00000111;
      int game_type = (game_mode_flag >> 3) & 0b00000111;
      addr += ((game_type * 8 + level) * 6);
      EEPROM.put(addr, scores);
    }
  }
  void writeGameModeFlag(void){
    uint16_t num_magic_2 = 0xABCD;
    EEPROM.put(NUM_MAGIC_2_ADDR, num_magic_2);
    EEPROM.put(GAME_MODE_ADDR, (game_mode_flag & 0b00111111));
  }
  void writeBrightnessLevel(void){
    uint16_t num_magic_1 = 0xABCD;
    EEPROM.put(NUM_MAGIC_1_ADDR, num_magic_1);
    EEPROM.put(BRIGHT_LEV_ADDR, brightness_level);
  }
#else
  void writeSaveGame(void){
    return;
  }
  void loadSaveGame(void){
    return;
  }
  void loadScores(void){
    return;
  }
  void writeScores(void){
    return;
  }
  void writeGameModeFlag(void){
    return;
  }
  void writeBrightnessLevel(void){
    return;
  }
#endif

void printPos(char message[], int x, int y) {
  debug(message);
  debug(": (");
  debug(x);
  debug(", ");
  debug(y);
  debugln(")");
}

void printMatrix(void) {
  for (uint8_t i = 0; i < ROWS; i++) {
    for (uint8_t j = 0; j < COLMS; j++) {
      if(game_map[i][j] == WALL){
        debug('w');
      }
      else if(game_map[i][j] >0 && game_map[i][j] <=253){
        debug(game_map[i][j]);
      }
      else if(game_map[i][j] == FOOD){
        debug('f');
      }
      else {
        debug("0");
      }
      debug(" ");
    }
    debugln("");
  }
}

void initMenu(void){
  game_flag |= 0b00000100; // is_menu_init = 1
  if(((game_flag >> 3) & 1) == 1){
    menu_idx = 0;
    menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00001000;
  }
  else{
    menu_idx = 1;
    menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000111;
  }
  menu_indexes[0] &= 0b11110000;
  menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
  menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
  menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111);
  // debug("Chay vao day");
  renderMenuInit();
}
void initGame(void) {
  game_mode_flag &= 0b00111111; // is_game_over = 0 and is_food_eaten = 0
  game_flag |= 0b01000000; // is_gameplay_rendered = 1
  initWall();
  initSnake();
  initFood();
  renderGameInit();
}
void initWall(void) {
  uint8_t game_type = (game_mode_flag >>3) & 0b00000111;
  if(game_type == 1){ // game type = INFINITY
    return;
  }
  for (uint8_t i = 0; i < ROWS; i++) {
    for (uint8_t j = 0; j < COLMS; j++) {
      switch (game_type){
        case 0: // game type = CLASSIC
          if (i == 0 || i == ROWS - 1 || j == 0 || j == COLMS - 1) {
            game_map[i][j] = WALL;
          }
          break;
        case 2: // game type = TUNNEL
          #if SCREEN == 0
            if(((i == 1 || i == ROWS - 2) && (j == 1 || j == 2 || j == COLMS - 2 || j == COLMS - 3)) ||         // draw:       __
              ((i == 2 || i == ROWS - 3) && (j == 1 || j == COLMS - 2)) ||                                      //            |
              ((i == 4 || i == 7) && (j >= 5 && j <= 15)))                                                      // draw:       ___
                game_map[i][j] = WALL;
          #endif
          break;
        case 3: // game type = MILL
          #if SCREEN == 0
            if(((i >= 1 && i <= 5) && j == 6) ||                                                               // draw:   | in left
               (i == 8 && (j >= 1 && j <= 6)) ||                                                                // draw: __ in left
               ((i >= 6 && i <= ROWS - 2) && j == 14) ||                                                        // draw:               __ in right
               ((i == 3) && (j >= 14 && j <= COLMS - 2)))                                                       // draw:              | in right
                game_map[i][j] = WALL;
          #endif
          break;
        case 4: // game type = RAILS
          #if SCREEN == 0
            if(((i == 1 || i == ROWS - 2) && (j >= 1 && j <= COLMS - 2)) ||                                                                     // draw:       _________
              ((i == 2 || i == ROWS - 3) && (j == 1 || j == COLMS - 2)) ||                                      //            |         |
              ((i == 4 || i == 7) && (j >= 5 && j <= 15)))                                                      // draw:       ___
                game_map[i][j] = WALL;
          #endif
          break;
        case 5: // game type = APARTMENT
          if((i == 1 && ((j >= 1 && j <= 2) || (j >= 5 && j <= 17))) || 
             (i == 5 && ((j >= 1 && j <= 9) || (j >= 12 && j <= COLMS - 2))) || 
             (i == 8 && (j >= 1 && j <= COLMS - 2)) || 
             ((i >= 1 && i <= 2) && j == 1) || 
             ((i >= 2 && i <= 4) && j == 9) || 
             ((i >= 9 && i <= ROWS - 2) && j == 12))
                game_map[i][j] = WALL;
          break;
        // default:
        //   return;
      }
    }
  }
}
void initSnake(void) {
  uint8_t game_type = (game_mode_flag >>3) & 0b00000111;
  switch (game_type){
    case 0:
    case 1:
      do {
        head_snake_row_x = (uint8_t)random(ROWS / 2 + 1, ROWS);
        head_snake_colm_y = (uint8_t)random(COLMS / 2);
      } while (game_map[head_snake_row_x][head_snake_colm_y] == WALL || game_map[head_snake_row_x][head_snake_colm_y] == FOOD || 
               head_snake_row_x == 0 || head_snake_row_x == ROWS - 1 || head_snake_colm_y == 1 || head_snake_colm_y == 0);
      break;
    case 2:
    case 4:
    case 5:
      head_snake_row_x = 6;
      do {
        head_snake_colm_y = (uint8_t)random(COLMS / 2);
      } while (game_map[head_snake_row_x][head_snake_colm_y] == WALL || game_map[head_snake_row_x][head_snake_colm_y] == FOOD || head_snake_colm_y == 1 || head_snake_colm_y == 0);
      break;
    case 3:
      head_snake_row_x = 7;
      do {
        head_snake_colm_y = (uint8_t)random(COLMS / 2);
      } while (game_map[head_snake_row_x][head_snake_colm_y] == WALL || game_map[head_snake_row_x][head_snake_colm_y] == FOOD || head_snake_colm_y == 1 || head_snake_colm_y == 0);
      break;
  }
  tail_snake_row_x = head_snake_row_x;
  tail_snake_colm_y = head_snake_colm_y - 1;  

  // create snake in matrix
  for (uint8_t i = tail_snake_colm_y; i <= head_snake_colm_y; i++) {
    game_map[tail_snake_row_x][i] = (game_map[tail_snake_row_x][i] & 0) | 0b00100100; // curr = right, future = right
    if(i == head_snake_colm_y){
      game_map[tail_snake_row_x][i] &= 0b11111000;
    }
  }
}
void initFood(void) {
  do {
    food_row_x = (uint8_t)random(ROWS);
    food_colm_y = (uint8_t)random(COLMS);
  } while (game_map[food_row_x][food_colm_y] == WALL ||
          food_row_x == 1 || food_row_x == 0 || food_row_x == ROWS - 1 || food_row_x == ROWS - 2 ||
          food_colm_y == 1 || food_colm_y == 0 || food_colm_y == COLMS - 1 || food_colm_y == COLMS - 2 ||
         ((game_map[food_row_x][food_colm_y] & 0b00000111) >= 0 && (game_map[food_row_x][food_colm_y] & 0b00000111) <= RIGHT &&
         ((game_map[food_row_x][food_colm_y] >> 3) & 0b00000111) >= TOP && ((game_map[food_row_x][food_colm_y] >> 3) & 0b00000111) <= RIGHT &&
         (((game_map[food_row_x][food_colm_y] >> 6) & 0b00000011) == 0 || ((game_map[food_row_x][food_colm_y] >> 6) & 0b00000011) == 1)));
  // Create food in matrix
  game_map[food_row_x][food_colm_y] = FOOD;
}

void renderGameInit(void){
  uint8_t game_type = (game_mode_flag >>3) & 0b00000111;
  #if SCREEN == 0
    display.drawLine(2, 1 + 2, 2, SCREEN_HEIGHT - 2 - 2, SHOW);
    display.drawLine(SCREEN_WIDTH - 2 - 2, 1 + 2, SCREEN_WIDTH - 2 - 2, SCREEN_HEIGHT - 2 - 2, SHOW);
    display.drawLine(2, 2, SCREEN_WIDTH - 2 - 2, 2, SHOW);
    display.drawLine(2, SCREEN_HEIGHT - 1 - 3, SCREEN_WIDTH - 2 - 2, SCREEN_HEIGHT - 1 - 3, SHOW);
  #else 
    display.drawLine(4, 1, 4, SCREEN_HEIGHT - 2, SHOW);
    display.drawLine(SCREEN_WIDTH - 2 - 6, 1, SCREEN_WIDTH - 2 - 6, SCREEN_HEIGHT - 2, SHOW);
    display.drawLine(4, 0, SCREEN_WIDTH - 2 - 6, 0, SHOW);
    display.drawLine(4, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 2 - 6, SCREEN_HEIGHT - 1, SHOW);
  #endif
  for (uint8_t i = 0; i < ROWS; i++) {
      for (uint8_t j = 0; j < COLMS; j++) {
        if (game_map[i][j] == 0) continue;
        uint16_t pos_x = getXPosScreen(j, j);
        uint16_t pos_y = getYPosScreen(i, j);
        if (game_map[i][j] == WALL && game_type != 0 && game_type != 1) {
          #if SCREEN == 0
            pos_x -= 1;
            pos_y -= 1;
            display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH - 1, NODE_SNAKE_HEIGHT, SHOW);
            uint16_t pos_x_1 = pos_x;
            uint16_t pos_y_1 = pos_y;
            if(game_map[i + 1][j] == WALL && i + 1 < ROWS){
              pos_y_1 += 3;
              display.drawLine(pos_x_1, pos_y_1, pos_x_1 + 2, pos_y_1, SHOW); 
            }
            pos_x_1 = pos_x;
            pos_y_1 = pos_y;
            if(game_map[i - 1][j] == WALL && i - 1 >= 0){
              pos_y_1 -= 1;
              display.drawLine(pos_x_1, pos_y_1, pos_x_1 + 2, pos_y_1, SHOW);
            }
            pos_x_1 = pos_x;
            pos_y_1 = pos_y;
            if(game_map[i][j + 1] == WALL && j + 1 < COLMS){
              pos_x_1 += 3;
              display.drawLine(pos_x_1, pos_y_1, pos_x_1, pos_y_1 + 2, SHOW);
            }
            if(game_map[i][j - 1] == WALL && j - 1 >= 0){
              pos_x_1 -= 1;
              display.drawLine(pos_x_1, pos_y_1, pos_x_1, pos_y_1 + 2, SHOW);
            }
          #endif
        } else {
        if (game_map[i][j] == FOOD) {
          #if SCREEN == 0
            pos_x -= 1;
            pos_y -= 1;
            display.drawBitmap(pos_x, pos_y, food_bitmap, 3, 3, SHOW);
          #else
            display.drawPixel(pos_x, pos_y, SHOW);
            display.drawPixel(pos_x, pos_y - 2, SHOW);
            display.drawPixel(pos_x, pos_y + 2, SHOW);
            display.drawPixel(pos_x - 2, pos_y, SHOW);
            display.drawPixel(pos_x + 2, pos_y, SHOW);
            display.drawPixel(pos_x - 1, pos_y - 1, SHOW);
            display.drawPixel(pos_x + 1, pos_y + 1, SHOW);
            display.drawPixel(pos_x - 1, pos_y + 1, SHOW);
            display.drawPixel(pos_x + 1, pos_y - 1, SHOW);
          #endif
        } 
        if((game_map[i][j] & 0b00000111) >= 0 && (game_map[i][j] & 0b00000111) <= RIGHT &&
          ((game_map[i][j] >> 3) & 0b00000111) >= TOP && ((game_map[i][j] >> 3) & 0b00000111) <= RIGHT &&
          (((game_map[i][j] >> 6) & 0b00000011) == 0 || ((game_map[i][j] >> 6) & 0b00000011) == 1)) { 
          uint8_t direct = (game_map[i][j] >> 3) & 0b00000111;
          switch (direct) {
            case TOP:
            #if SCREEN == 0
              pos_x -= 1;
              pos_y -= 1;
            #else
              pos_x -= 2;
              pos_y -= 2;
            #endif
            if(i == ROWS - 2){
              display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH - 1, SHOW);
            }
            else if (i < ROWS - 2){
              display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH, SHOW);
            }
            break;
          case DOWN:
            if(i == 1){
              #if SCREEN == 0
                pos_x -= 1;
                pos_y -= 1;
              #else
                pos_x -= 2;
                pos_y -= 2;
              #endif
              display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH - 1, SHOW); 
            }
            else if (i > 1){
              #if SCREEN == 0
                pos_x -= 1;
                pos_y -= 2;
              #else
                pos_x -= 2;
                pos_y -= 3;
              #endif
              display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH, SHOW); 
            }
            break;
          case LEFT:
            #if SCREEN == 0
              pos_x -= 1;
              pos_y -= 1;
            #else
              pos_x -= 2;
              pos_y -= 2;
            #endif
            if(j == COLMS - 2){
              display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH - 1, NODE_SNAKE_HEIGHT, SHOW);
            }
            else if(j < COLMS - 2){
              display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH, NODE_SNAKE_HEIGHT, SHOW);
            }
            break;
          case RIGHT:
            if(j > 1){
              #if SCREEN == 0
                pos_x -= 2;
                pos_y -= 1;
              #else
                pos_x -= 3;
                pos_y -= 2;
              #endif
              display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH, NODE_SNAKE_HEIGHT, SHOW);
            }
            else if(j == 1){
              #if SCREEN == 0
                pos_x -= 1;
                pos_y -= 1;
              #else
                pos_x -= 2;
                pos_y -= 2;
              #endif
              display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH - 1, NODE_SNAKE_HEIGHT, SHOW);
            }
            break;
          }
        }
      }
      } 
  }
  display.display();
  time_render_init_game = MILLIS();
}
void renderGameplay(void) {
  if(((game_flag >> 3) & 1) == 1){ // is_game_init == 1
    uint16_t pos_x = getXPosScreen(prev_tail_snake_row_x, prev_tail_snake_colm_y);
    uint16_t pos_y = getYPosScreen(prev_tail_snake_row_x, prev_tail_snake_colm_y);
    // Delete tail snake
    if (prev_tail_snake_row_x != 0 && prev_tail_snake_colm_y != 0) {
      uint8_t direct = prev_tail_snake_direct;
        switch (direct) {
          case TOP:
            #if SCREEN == 0
              pos_x -= 1;
              pos_y -= 1;
            #else
              pos_x -= 2;
              pos_y -= 2;
            #endif
            display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH, HIDE);
            break;
          case DOWN:
            #if SCREEN == 0
              pos_x -= 1;
              pos_y -= 2;
            #else
              pos_x -= 2;
              pos_y -= 3;
            #endif
            display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH, HIDE);
            break;
          case LEFT:
            #if SCREEN == 0
              pos_x -= 1;
              pos_y -= 1;
            #else
              pos_x -= 2;
              pos_y -= 2;
            #endif
            display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH, NODE_SNAKE_HEIGHT, HIDE);
            break;
          case RIGHT:
              #if SCREEN == 0
                pos_x -= 2;
                pos_y -= 1;
              #else
                pos_x -= 3;
                pos_y -= 2;
              #endif
              display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH, NODE_SNAKE_HEIGHT, HIDE);
            break;
        }
    }
    pos_x = getXPosScreen(prev_food_row_x, prev_food_colm_y);
    pos_y = getYPosScreen(prev_food_row_x, prev_food_colm_y);
    if (prev_food_row_x != 0 && prev_food_colm_y != 0) {
      // Remove old food
      #if SCREEN == 0
        pos_x -= 1;
        pos_y -= 1;
        display.drawBitmap(pos_x, pos_y, food_bitmap, 3, 3, HIDE);
      #else
        display.drawPixel(pos_x, pos_y, HIDE);
        display.drawPixel(pos_x, pos_y - 2, HIDE);
        display.drawPixel(pos_x, pos_y + 2, HIDE);
        display.drawPixel(pos_x - 2, pos_y, HIDE);
        display.drawPixel(pos_x + 2, pos_y, HIDE);
        display.drawPixel(pos_x - 1, pos_y - 1, HIDE);
        display.drawPixel(pos_x + 1, pos_y + 1, HIDE);
        display.drawPixel(pos_x - 1, pos_y + 1, HIDE);
        display.drawPixel(pos_x + 1, pos_y - 1, HIDE);
      #endif
      // Draw new food
      prev_food_row_x = 0;
      prev_food_colm_y = 0;
      pos_x = getXPosScreen(food_row_x, food_colm_y);
      pos_y = getYPosScreen(food_row_x, food_colm_y);
      #if SCREEN == 0
        pos_x -= 1;
        pos_y -= 1;
        display.drawBitmap(pos_x, pos_y, food_bitmap, 3, 3, SHOW);
        // display.drawPixel(pos_x, pos_y - 1, SHOW);
        // display.drawPixel(pos_x - 1, pos_y, SHOW);
        // display.drawPixel(pos_x, pos_y + 1, SHOW);
        // display.drawPixel(pos_x + 1, pos_y, SHOW);
      #else
        display.drawPixel(pos_x, pos_y, SHOW);
        display.drawPixel(pos_x, pos_y - 2, SHOW);
        display.drawPixel(pos_x, pos_y + 2, SHOW);
        display.drawPixel(pos_x - 2, pos_y, SHOW);
        display.drawPixel(pos_x + 2, pos_y, SHOW);
        display.drawPixel(pos_x - 1, pos_y - 1, SHOW);
        display.drawPixel(pos_x + 1, pos_y + 1, SHOW);
        display.drawPixel(pos_x - 1, pos_y + 1, SHOW);
        display.drawPixel(pos_x + 1, pos_y - 1, SHOW);
      #endif
    }
    // Draw new head snake
    pos_x = getXPosScreen(head_snake_row_x, head_snake_colm_y);
    pos_y = getYPosScreen(head_snake_row_x, head_snake_colm_y);
      uint8_t direct = (game_map[head_snake_row_x][head_snake_colm_y] >> 3) & 0b00000111;
        switch (direct) {
          case TOP:
            #if SCREEN == 0
              pos_x -= 1;
              pos_y -= 1;
            #else
              pos_x -= 2;
              pos_y -= 2;
            #endif
            if(head_snake_row_x == ROWS - 2){
              display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH - 1, SHOW);
            }
            else if (head_snake_row_x < ROWS - 2){
              display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH, SHOW);
            }
            break;
          case DOWN:
            if(head_snake_row_x == 1){
              #if SCREEN == 0
                pos_x -= 1;
                pos_y -= 1;
              #else
                pos_x -= 2;
                pos_y -= 2;
              #endif
              display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH - 1, SHOW); 
            }
            else if (head_snake_row_x > 1){
              #if SCREEN == 0
                pos_x -= 1;
                pos_y -= 2;
              #else
                pos_x -= 2;
                pos_y -= 3;
              #endif
              display.fillRect(pos_x, pos_y, NODE_SNAKE_HEIGHT, NODE_SNAKE_WIDTH, SHOW); 
            }
            break;
          case LEFT:
            #if SCREEN == 0
              pos_x -= 1;
              pos_y -= 1;
            #else
              pos_x -= 2;
              pos_y -= 2;
            #endif
            if(head_snake_colm_y == COLMS - 2){
              display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH - 1, NODE_SNAKE_HEIGHT, SHOW);
            }
            else if (head_snake_colm_y < COLMS - 2){
              display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH, NODE_SNAKE_HEIGHT, SHOW);
            }
            break;
          case RIGHT:
            if(head_snake_colm_y > 1){
              #if SCREEN == 0
                pos_x -= 2;
                pos_y -= 1;
              #else
                pos_x -= 3;
                pos_y -= 2;
              #endif
              display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH, NODE_SNAKE_HEIGHT, SHOW);
            }
            else if(head_snake_colm_y == 1){
              #if SCREEN == 0
                pos_x -= 1;
                pos_y -= 1;
              #else
                pos_x -= 2;
                pos_y -= 2;
              #endif
              display.fillRect(pos_x, pos_y, NODE_SNAKE_WIDTH - 1, NODE_SNAKE_HEIGHT, SHOW);
            }
            break;
        }
  }
  display.display();
}
void renderMenuInit(void) {
  char buffer[15];
  int8_t selected_index = ((menu_indexes[0] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[0] & 0b00001111);
  int8_t title_idx = ((menu_indexes[1] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[1] & 0b00001111);
  int8_t fi_idx_fi_row = ((menu_indexes[2] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[2] & 0b00001111);
  int8_t se_idx_se_row = ((menu_indexes[3] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[3] & 0b00001111);
  int8_t thr_idx_thr_row = ((menu_indexes[4] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[4] & 0b00001111);
  display.setTextSize(1);
  display.setTextColor(SHOW);
  strcpy_P(buffer, (char *)pgm_read_ptr(&(menu_table[title_idx])));
  if (title_idx == 8) {
    display.setCursor(0 + 6, 0 + 2);
    display.print(buffer);
    display.print(curr_score);
  } else {
    int16_t title_length = strlen(buffer);
    int16_t pos_x = (14 - title_length) * 3;
    display.setCursor(pos_x, 0 + 2);
    display.print(buffer);
  }

  display.drawLine(0 + 5, 11 + 2, 83 - 5, 13, SHOW);
    for (int8_t i = 0; i < 3; i++) {
      int8_t curr_index;
      int16_t cursor_pos_y;
      if (i == 0) {
        curr_index = fi_idx_fi_row;
        cursor_pos_y = 15;
      } else if (i == 1) {
        curr_index = se_idx_se_row;
        cursor_pos_y = 26;
      } else {
        curr_index = thr_idx_thr_row;
        cursor_pos_y = 37;
      }
      strcpy_P(buffer, (char *)pgm_read_ptr(&(menu_table[curr_index])));
      if (selected_index == i) {
        display.fillRect(0, cursor_pos_y, 84, 11, SHOW);
        display.setTextColor(HIDE);
      } else {
        display.setTextColor(SHOW);
      }
      display.setCursor(6, cursor_pos_y + 2);
      if (title_idx != 4) {
        if (fi_idx_fi_row != -1)
          display.print(buffer);
      } else {
        if (curr_index != -1) {
          display.print(curr_index + 1);
          display.print(". ");
          display.print(scores[curr_index]);
        }
      }
    }
  display.display();
  time_init_menu = MILLIS();
}
void renderMenu(void) {
  char buffer[15];
  int8_t prev_selected_index = (((menu_indexes[0] >> 4) & 0b00001111) == 0b00001111) ? -1 : (int8_t)((menu_indexes[0] >> 4) & 0b00001111);
  int8_t prev_title_idx = (((menu_indexes[1] >> 4) & 0b00001111) == 0b00001111) ? -1 : (int8_t)((menu_indexes[1] >> 4) & 0b00001111);
  int8_t prev_fi_idx_fi_row = (((menu_indexes[2] >> 4) & 0b00001111) == 0b00001111) ? -1 : (int8_t)((menu_indexes[2] >> 4) & 0b00001111);
  int8_t prev_se_idx_se_row = (((menu_indexes[3] >> 4) & 0b00001111) == 0b00001111) ? -1 : (int8_t)((menu_indexes[3] >> 4) & 0b00001111);
  int8_t prev_thr_idx_thr_row = (((menu_indexes[4] >> 4) & 0b00001111) == 0b00001111) ? -1 : (int8_t)((menu_indexes[4] >> 4) & 0b00001111);
  int8_t selected_index = ((menu_indexes[0] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[0] & 0b00001111);
  int8_t title_idx = ((menu_indexes[1] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[1] & 0b00001111);
  int8_t fi_idx_fi_row = ((menu_indexes[2] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[2] & 0b00001111);
  int8_t se_idx_se_row = ((menu_indexes[3] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[3] & 0b00001111);
  int8_t thr_idx_thr_row = ((menu_indexes[4] & 0b00001111) == 0b00001111) ? -1 : (int8_t)(menu_indexes[4] & 0b00001111);
  // Serial.println(F(""));
  // Serial.print(F("prev_selected_index: "));
  // Serial.println(prev_selected_index);
  // Serial.print(F("prev_title_idx: "));
  // Serial.println(prev_title_idx);
  // Serial.print(F("prev_fi_idx_fi_row: "));
  // Serial.println(prev_fi_idx_fi_row);
  // Serial.print(F("prev_se_idx_se_row: "));
  // Serial.println(prev_se_idx_se_row);
  // Serial.print(F("prev_thr_idx_thr_row: "));
  // Serial.println(prev_thr_idx_thr_row);
  // Serial.print(F("selected_index: "));
  // Serial.println(selected_index);
  // Serial.print(F("title_idx: "));
  // Serial.println(title_idx);
  // Serial.print(F("fi_idx_fi_row: "));
  // Serial.println(fi_idx_fi_row);
  // Serial.print(F("se_idx_se_row: "));
  // Serial.println(se_idx_se_row);
  // Serial.print(F("thr_idx_thr_row: "));
  // Serial.println(thr_idx_thr_row);
  if (((game_flag >> 2) & 0b00000001) == 1) {  // is_menu_init == 1
    display.setTextSize(1);
    if (prev_title_idx != title_idx) {
      strcpy_P(buffer, (char *)pgm_read_ptr(&(menu_table[title_idx])));
      display.fillRect(0, 0, 84, 9, HIDE);
      display.setTextColor(SHOW);
      if (title_idx == 8) {
        display.setCursor(0 + 6, 0 + 2);
        display.print(buffer);
        display.print(curr_score);
      } else {
        int16_t title_length = strlen(buffer);
        int16_t pos_x = (14 - title_length) * 3;
        display.setCursor(pos_x, 0 + 2);
        display.print(buffer);
      }
    }
    if (((prev_title_idx == 2 || prev_title_idx == 5) && (title_idx != 2 && title_idx != 5)) || ((prev_title_idx != 2 && prev_title_idx != 5) && (title_idx == 2 || title_idx == 5))) {
      display.fillRect(0, 15, 84, 33, HIDE);
    }
    if (title_idx != 2 && title_idx != 5) {
      switch (selected_index) {
        case 0:
          switch (prev_selected_index) {
            case 1:
              display.fillRect(0, 26, 84, 11, HIDE);
              break;
            case 2:
              display.fillRect(0, 37, 84, 11, HIDE);
              break;
          }
          display.fillRect(0, 15, 84, 11, SHOW);
          break;
        case 1:
          switch (prev_selected_index) {
            case 0:
              display.fillRect(0, 15, 84, 11, HIDE);
              break;
            case 2:
              display.fillRect(0, 37, 84, 11, HIDE);
              break;
          }
          display.fillRect(0, 26, 84, 11, SHOW);
          break;
        case 2:
          switch (prev_selected_index) {
            case 0:
              display.fillRect(0, 15, 84, 11, HIDE);
              break;
            case 1:
              display.fillRect(0, 26, 84, 11, HIDE);
              break;
          }
          display.fillRect(0, 37, 84, 11, SHOW);
          break;
      }
      for (int8_t i = 0; i < 3; i++) {
        int8_t curr_index, prev_index;
        int16_t cursor_pos_y;
        if (i == 0) {
          curr_index = fi_idx_fi_row;
          prev_index = prev_fi_idx_fi_row;
          cursor_pos_y = 15;
        } else if (i == 1) {
          curr_index = se_idx_se_row;
          prev_index = prev_se_idx_se_row;
          cursor_pos_y = 26;
        } else {
          curr_index = thr_idx_thr_row;
          prev_index = prev_thr_idx_thr_row;
          cursor_pos_y = 37;
        }
        display.setCursor(6, cursor_pos_y + 2);
        if (prev_index != curr_index || (title_idx != prev_title_idx && prev_title_idx == 4)) {
          if (selected_index == i) {
            display.fillRect(0, cursor_pos_y, 84, 11, SHOW);
            display.setTextColor(HIDE);
          } else {
            display.fillRect(0, cursor_pos_y, 84, 11, HIDE);
            display.setTextColor(SHOW);
          }
        } else {
          if (selected_index != i)
            display.setTextColor(SHOW);
          else
            display.setTextColor(HIDE);
        }
        strcpy_P(buffer, (char *)pgm_read_ptr(&(menu_table[curr_index])));
        if (title_idx != 4) {
          // Serial.println(F("Title index != 4"));
          if (curr_index != -1)
            display.print(buffer);
        } else {
          // Serial.println(F("Title index == 4"));
          if (curr_index != -1) {
            display.print(curr_index + 1);
            display.print(". ");
            display.print(scores[curr_index]);
          }
        }
        display.setCursor(0, 0);
      }
    }
    else{
      uint16_t pos_x = 22;
      uint16_t pos_y = 29;
      display.fillRect(pos_x, pos_y, 40, 5, HIDE);
      for (int i = 1; i <= selected_index; i++) {
        #if SCREEN == 0
          pos_x = 22;
          pos_y = 29;
        #endif
        pos_x += ((i - 1) * 5);
        display.fillRect(pos_x, pos_y, 5, 5, SHOW);
      }
      for (int i = selected_index + 1; i <= 8; i++) {
        #if SCREEN == 0
          pos_x = 22;
          pos_y = 29;
        #endif
        pos_x += ((i - 1) * 5);
        display.drawBitmap(pos_x, pos_y, level_hide_bitmap, 5, 5, SHOW);
      }
      // Serial.print(F("selected_index: "));
      // Serial.println(selected_index);
    }
  }
  display.display();
}
void renderGameStartMenu(void) {
  display.drawBitmap(0, 0, game_start_menu_bitmap, 84, 48, SHOW);
  display.invertDisplay(true);
  display.display();
}
uint16_t getXPosScreen(uint8_t row_x, uint8_t colm_y) {
  if (colm_y == 0) {
    #if SCREEN == 0
      return 2;
    #else
      return 4;
    #endif
  }
  #if SCREEN == 0
    uint16_t res = (uint16_t)(4 * colm_y - 1 + 2 + 0 * row_x);
  #else
    uint16_t res = (uint16_t)(6 * colm_y - 2 + 4 + 0 * row_x);
  #endif
  return res;
}
uint16_t getYPosScreen(uint8_t row_x, uint8_t colm_y) {
  if (row_x == 0) {
    return 0;
  }
  #if SCREEN == 0
    uint16_t res = (uint16_t)(4 * row_x - 1 + 2 + 0 * colm_y);
  #else
    uint16_t res = (uint16_t)(6 * row_x - 2 + 0 * colm_y);
  #endif
  return res;
}

uint8_t handleButtons(void) {
  // Write pulse to load pin
  digitalWrite(PL_PIN, LOW);
  DELAY_MICROSECONDS(5);
  digitalWrite(PL_PIN, HIGH);
  DELAY_MICROSECONDS(5);

  // Get data from 74HC165
  digitalWrite(CP_PIN, HIGH);
  digitalWrite(CE_PIN, LOW);
  byte incoming = shiftIn(QH_PIN, CP_PIN, MSBFIRST);
  digitalWrite(CE_PIN, HIGH);

  uint8_t cnt = 0;
  uint8_t res = 0;
  if(incoming == 0b11111111) return 0;
  for(uint8_t i = 0; i < 6; i++){
    if(((incoming >> i) & 1) == 0){
      res = i + 1;
      cnt++;
      if(cnt > 1) return 0;
    }
  }
  return res;
}

// Read input from button
// If value read from button > 0
//      If button pressed not equal 'OK', , snake move from button
//      Else game paused
// Else snake move straight
void gamePlay(uint8_t button_value) {
  //gameplay
  game_flag = (game_flag & 0b11111100)  | 0b00000010;
  game_flag &= 0b11111011; // is_menu_init = 0
  // check if button is OK or BACK, return no value in function and change gameflag value to game menu or game start menu
    switch (button_value) {
      case OK:
        if(MILLIS() - time_render_init_game >= 600){
          //return to menu
          game_flag = (game_flag & 0b11111100) | 0b00000001;
          game_flag = game_flag & 0b11111011; //is_menu_init = 0
          display.clearDisplay();
          // save state game to EPPROM
          writeSaveGame();
          return;
        }
        else
          button_value = 0;
        break;
      case BACK:
        if(MILLIS() - time_render_init_game >= 600){
          //return to game start menu
          game_flag &= 0b11111100;
          display.clearDisplay();
          // save state game to EPPROM
          writeSaveGame();
          return;
        }
        else
          button_value = 0;
        break;
    }
  uint8_t level = (game_mode_flag & 0b00000111);
  uint8_t game_type = (game_mode_flag >> 3) & 0b00000111;
  unsigned long time_delay = (unsigned long)timeDelay(level);
  if (((game_flag >> 6) & 1) == 1) {  // is_gameplay_rendered == 1
    curr_time_gameplay = MILLIS();
    uint8_t head_direct = (game_map[head_snake_row_x][head_snake_colm_y] >> 3) & 0b00000111;
    uint8_t head_direct_future = 0;

    btn_state = button_value;
    debugln(btn_state);
    if (btn_state == 0) {
      head_direct_future = head_direct;
    } else {
      switch (btn_state) {
        case TOP:
          if (head_direct == DOWN) head_direct_future = head_direct;
          else head_direct_future = btn_state;
          break;
        case DOWN:
          if (head_direct == TOP) head_direct_future = head_direct;
          else head_direct_future = btn_state;
          break;
        case LEFT:
          if (head_direct == RIGHT) head_direct_future = head_direct;
          else head_direct_future = btn_state;
          break;
        case RIGHT:
          if (head_direct == LEFT) head_direct_future = head_direct;
          else head_direct_future = btn_state;
          break;
      }
    }
    // debugln(head_direct_future);
    uint8_t snake_future = checkFutureSnake(head_snake_row_x, head_snake_colm_y, head_direct_future);

    if (snake_future == WILL_BE_GAMEOVER) {
      game_mode_flag |= 0b01000000;  // is_game_over = 1
      btn_state = 0;
      return;
    } else {
      // head snake move in matrix
      prev_head_snake_row_x = head_snake_row_x;
      prev_head_snake_colm_y = head_snake_colm_y;
      if (head_direct_future == TOP) {
        // declare future value: top to prev head snake
        game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = (game_map[prev_head_snake_row_x][prev_head_snake_colm_y] | 0b00000001) & 0b11111001;
        if(game_type != 0 && prev_head_snake_row_x == 1){
          game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = (game_map[prev_head_snake_row_x][prev_head_snake_colm_y] & 0b00111111) | 0b01000000;
          head_snake_row_x = ROWS - 2;
        }
        else{
          game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = game_map[prev_head_snake_row_x][prev_head_snake_colm_y] & 0b00111111;
          head_snake_row_x = head_snake_row_x - 1;
        }
        if(game_map[head_snake_row_x][head_snake_colm_y] == FOOD){
          game_map[head_snake_row_x][head_snake_colm_y] = 0;
        }
        // declare curr value: top to new head snake
        game_map[head_snake_row_x][head_snake_colm_y] = (game_map[head_snake_row_x][head_snake_colm_y] | 0b00001000) & 0b11001111;
      } else if (head_direct_future == DOWN) {
        // declare future value: down to prev head snake
        game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = (game_map[prev_head_snake_row_x][prev_head_snake_colm_y] | 0b00000010) & 0b11111010;
        if(game_type != 0 && prev_head_snake_row_x == ROWS - 2){
          game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = (game_map[prev_head_snake_row_x][prev_head_snake_colm_y] & 0b00111111) | 0b01000000;
          head_snake_row_x = 1;
        }
        else{
          game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = game_map[prev_head_snake_row_x][prev_head_snake_colm_y] & 0b00111111;
          head_snake_row_x = head_snake_row_x + 1;
        }
        if(game_map[head_snake_row_x][head_snake_colm_y] == FOOD){
          game_map[head_snake_row_x][head_snake_colm_y] = 0;
        }
        // declare curr value: down to new head snake
        game_map[head_snake_row_x][head_snake_colm_y] = (game_map[head_snake_row_x][head_snake_colm_y] | 0b00010000) & 0b11010111;
      } else if (head_direct_future == LEFT) {
        // declare future value: left to prev head snake
        game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = (game_map[prev_head_snake_row_x][prev_head_snake_colm_y] | 0b00000011) & 0b11111011;
        if(game_type != 0 && prev_head_snake_colm_y == 1){
          game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = (game_map[prev_head_snake_row_x][prev_head_snake_colm_y] & 0b00111111) | 0b01000000;
          head_snake_colm_y = COLMS - 2;
        }
        else{
          game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = game_map[prev_head_snake_row_x][prev_head_snake_colm_y] & 0b00111111;
          head_snake_colm_y = head_snake_colm_y - 1;
        }
        if(game_map[head_snake_row_x][head_snake_colm_y] == FOOD){
          game_map[head_snake_row_x][head_snake_colm_y] = 0;
        }
        // declare curr value: left to new head snake
        game_map[head_snake_row_x][head_snake_colm_y] = (game_map[head_snake_row_x][head_snake_colm_y] | 0b00011000) & 0b11011111;
      } else if (head_direct_future == RIGHT) {
        // declare future value: right to prev head snake
        game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = (game_map[prev_head_snake_row_x][prev_head_snake_colm_y] | 0b00000100) & 0b11111100;
        if(game_type != 0 && prev_head_snake_colm_y == COLMS - 2){
          game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = (game_map[prev_head_snake_row_x][prev_head_snake_colm_y] & 0b00111111) | 0b01000000;
          head_snake_colm_y = 1;
        }
        else {
          game_map[prev_head_snake_row_x][prev_head_snake_colm_y] = game_map[prev_head_snake_row_x][prev_head_snake_colm_y] & 0b00111111;
          head_snake_colm_y = head_snake_colm_y + 1;
        }
        if(game_map[head_snake_row_x][head_snake_colm_y] == FOOD){
          game_map[head_snake_row_x][head_snake_colm_y] = 0;
        }
        // declare curr value: right to new head snake
        game_map[head_snake_row_x][head_snake_colm_y] = (game_map[head_snake_row_x][head_snake_colm_y] | 0b00100000) & 0b11100111;
      }
      if (((game_mode_flag >> 7) & 1) == 0) {  //is_food_eaten == 0
        // tail snake move in matrix
        prev_tail_snake_row_x = tail_snake_row_x;
        prev_tail_snake_colm_y = tail_snake_colm_y;
        prev_tail_snake_direct = (game_map[prev_tail_snake_row_x][prev_tail_snake_colm_y] >> 3) & 0b00000111;
        uint8_t new_tail_snake_direct = game_map[prev_tail_snake_row_x][prev_tail_snake_colm_y] & 0b00000111;
        switch (new_tail_snake_direct) {
          case TOP:
            if(((game_map[prev_tail_snake_row_x][prev_tail_snake_colm_y] >> 6) & 0b00000011) == 1){
              tail_snake_row_x = ROWS - 2;
            }
            else{
              tail_snake_row_x -= 1;
            }
            break;
          case DOWN:
            if(((game_map[prev_tail_snake_row_x][prev_tail_snake_colm_y] >> 6) & 0b00000011) == 1){
              tail_snake_row_x = 1;
            }
            else{
              tail_snake_row_x += 1;
            }
            break;
          case LEFT:
            if(((game_map[prev_tail_snake_row_x][prev_tail_snake_colm_y] >> 6) & 0b00000011) == 1){
              tail_snake_colm_y = COLMS - 2;
            }
            else{
              tail_snake_colm_y -= 1;
            }
            break;
          case RIGHT:
            if(((game_map[prev_tail_snake_row_x][prev_tail_snake_colm_y] >> 6) & 0b00000011) == 1){
              tail_snake_colm_y = 1;
            }
            else{
              tail_snake_colm_y += 1;
            }
            break;
        }
        game_map[prev_tail_snake_row_x][prev_tail_snake_colm_y] = 0;
      }
      game_mode_flag &= 0b01111111;  // is_food_eaten = 0
      // debug("-------is_food_eaten before: ");
      // debugln(is_food_eaten);
      // debug("-------snake future before: ");
      // debugln(snake_future);
      if (snake_future != CONTINUE) {
        prev_food_row_x = food_row_x;
        prev_food_colm_y = food_colm_y;
        initFood();
        // debugln("-------create init food in gameplay()");
        // Serial.println(((game_map[head_snake_row_x][head_snake_colm_y] >> 3) & 0b00000111), BIN);
        calcScore(level);
        game_mode_flag |= 0b10000000;  // is_food_eaten = 1
      }
      // debug("-------is_food_eaten after: ");
      // debugln(is_food_eaten);
      // debug("-------snake future before: ");
      // debugln(snake_future);
    }
  }
  if (MILLIS() - curr_time_gameplay >= time_delay) {
    renderGameplay();
    if(((game_mode_flag >> 7) & 1) == 1){  //is_food_eaten == 1
      createFoodEatenSound();
    }
    curr_time_gameplay = MILLIS();
    game_flag |= 0b01000000;  //is_gameplay_rendered = 1
    return;
  }
  // if 'if' prove not process
  game_flag &= 0b10111111;  //is_gameplay_rendered = 0
}
void calcScore(uint8_t level){
  if(curr_score >= 9999) {
    curr_score = 9999;
    return;
  }
  switch(level){
    case 0:
      curr_score += POINTS_LEVEL_1;
      return;
      break;
    case 1:
      curr_score += POINTS_LEVEL_2;
      return;
      break;
    case 2:
      curr_score += POINTS_LEVEL_3;
      return;
      break;
    case 3:
      curr_score += POINTS_LEVEL_4;
      return;
      break;
    case 4:
      curr_score += POINTS_LEVEL_5;
      return;
      break;
    case 5:
      curr_score += POINTS_LEVEL_6;
      return;
      break;
    case 6:
      curr_score += POINTS_LEVEL_7;
      return;
      break;
    case 7:
      curr_score += POINTS_LEVEL_8;
      return;
      break;
  }
}
uint16_t timeDelay(uint8_t level){
  switch(level){
    case 0:
      return TIME_DELAY_LEVEL_1;
      break;
    case 1:
      return TIME_DELAY_LEVEL_2;
      break;
    case 2:
      return TIME_DELAY_LEVEL_3;
      break;
    case 3:
      return TIME_DELAY_LEVEL_4;
      break;
    case 4:
      return TIME_DELAY_LEVEL_5;
      break;
    case 5:
      return TIME_DELAY_LEVEL_6;
      break;
    case 6:
      return TIME_DELAY_LEVEL_7;
      break;
    case 7:
      return TIME_DELAY_LEVEL_8;
      break;
  }
}
uint8_t checkFutureSnake(uint8_t row_x, uint8_t colm_y, uint8_t direct) {
  if (direct != TOP && direct != DOWN && direct != LEFT && direct != RIGHT) return;
  uint8_t game_type = (game_mode_flag >> 3) & 0b00000111;
  uint8_t new_row_x, new_colm_y;
  switch (direct) {
    case TOP:
      if(row_x == 1 && game_type != 0){
        new_row_x = ROWS - 2;
      }
      else{
        new_row_x = row_x - 1;
      }
      new_colm_y = colm_y;
      break;
    case DOWN:
      if(row_x == ROWS - 2 && game_type != 0){
        new_row_x = 1;
      }
      else{
        new_row_x = row_x + 1;
      }
      new_colm_y = colm_y;
      break;
    case LEFT:
      if(colm_y == 1 && game_type != 0){
        new_colm_y = COLMS - 2;
      }
      else{
        new_colm_y = colm_y - 1;
      }
      new_row_x = row_x;
      break;
    case RIGHT:
      if(colm_y == COLMS - 2 && game_type != 0){
        new_colm_y = 1;
      }
      else{
        new_colm_y = colm_y + 1;
      }
      new_row_x = row_x;
      break;
  }
  if(((game_map[new_row_x][new_colm_y] & 0b00000111) >= 0 && (game_map[new_row_x][new_colm_y] & 0b00000111) <= RIGHT &&
          ((game_map[new_row_x][new_colm_y] >> 3) & 0b00000111) >= TOP && ((game_map[new_row_x][new_colm_y] >> 3) & 0b00000111) <= RIGHT &&
          (((game_map[new_row_x][new_colm_y] >> 6) & 0b00000011) == 0 || ((game_map[new_row_x][new_colm_y] >> 6) & 0b00000011) == 1)) || game_map[new_row_x][new_colm_y] == WALL) {
      // debugln("----GAME OVER");
      return WILL_BE_GAMEOVER;
    } else if (game_map[new_row_x][new_colm_y] == FOOD) {
      // debugln("----WILL_BE_EATEN");
      return WILL_BE_EATEN;
    }
    else {
      // debugln("----CONTINUE");
      return CONTINUE;
  } 
}

int8_t convertSubMenuIdxToMenuIdx(int8_t sub_menu_idx, int8_t menu_type){
  int8_t menu_idx = 0;
  if(menu_type == GAME_SNAKE_MENU){
    menu_idx = sub_menu_idx + 1;
  }
  else if(menu_type == IN_GAME_MENU || menu_type == HIGH_SCORES_MENU){
    menu_idx = sub_menu_idx;
  }
  else if(menu_type == GAME_TYPES_MENU){
    menu_idx = sub_menu_idx + 9;
  }
  return menu_idx;
}
void controlMenu(uint8_t button_value) {
  char buffer[15];
  int8_t sub_menu_length = 0;
  int8_t sub_menu_idx = 0;
  int8_t menu_type = GAME_SNAKE_MENU;
  // menu high scores
  if ((menu_indexes[1] & 0b00001111) == 0b00000100) {
    menu_type = HIGH_SCORES_MENU;
    sub_menu_length = 3;
    sub_menu_idx = menu_idx;
  }
  else if((menu_indexes[1] & 0b00001111) == 0b00000010){
    menu_type = LEVEL_MENU;
  }
  else if((menu_indexes[1] & 0b00001111) == 0b00000101){
    menu_type = BRIGHTNESS_MENU;
  }
  else {
    // menu game snake
    if (menu_idx >= 0 && menu_idx <= 6) {
      // menu when game is paused
      if (((game_flag >> 3) & 1) == 1) {  // is_game_init == 1
        menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00001000;    // title index = 8
        sub_menu_length = 7;
        sub_menu_idx = menu_idx;
        menu_type = IN_GAME_MENU;
      }
      // menu when game is exited or isn't played
      else {
        // title index = 7
        menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000111;
        sub_menu_length = 5;
        sub_menu_idx = menu_idx - 1;
      }
    }
    // menu game types
    else if (menu_idx >= 9 && menu_idx <= 14) {
      // title index = 3
      menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000011;
      sub_menu_length = 6;
      sub_menu_idx = menu_idx - 9;
      menu_type = GAME_TYPES_MENU;
    }
  }
  // copy value to varible prev
  menu_indexes[0] &= 0b00001111;
  menu_indexes[0] |= ((menu_indexes[0] << 4) & 0b11110000);
  menu_indexes[1] &= 0b00001111;
  menu_indexes[1] |= ((menu_indexes[1] << 4) & 0b11110000);
  menu_indexes[2] &= 0b00001111;
  menu_indexes[2] |= ((menu_indexes[2] << 4) & 0b11110000);
  menu_indexes[3] &= 0b00001111;
  menu_indexes[3] |= ((menu_indexes[3] << 4) & 0b11110000);
  menu_indexes[4] &= 0b00001111;
  menu_indexes[4] |= ((menu_indexes[4] << 4) & 0b11110000);
  switch (button_value) {
    case TOP:
      if ((menu_indexes[0] & 0b00001111) == 1) {
        if (sub_menu_idx > 0 && sub_menu_idx < sub_menu_length - 1) {
          sub_menu_idx--;
          menu_idx = convertSubMenuIdxToMenuIdx(sub_menu_idx, menu_type);
          menu_indexes[0] = menu_indexes[0] & 0b11110000;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
          (sub_menu_idx + 2 < sub_menu_length) ? menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111) : menu_indexes[4] |= 0b00001111;  // = -1
        }
      } else if ((menu_indexes[0] & 0b00001111) == 2) {
        if (sub_menu_idx >= 2) {
          sub_menu_idx--;
          menu_idx = convertSubMenuIdxToMenuIdx(sub_menu_idx, menu_type);
          menu_indexes[0] = (menu_indexes[0] & 0b11110000) | 0b00000001;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | ((menu_idx - 1) & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
        }
      } else if ((menu_indexes[0] & 0b00001111) == 0) {
        if (sub_menu_idx > 0) {
          sub_menu_idx--;
          menu_idx = convertSubMenuIdxToMenuIdx(sub_menu_idx, menu_type);
          menu_indexes[0] = menu_indexes[0] & 0b11110000;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
          (sub_menu_idx + 2 < sub_menu_length) ? menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111) : menu_indexes[4] |= 0b00001111;  // = -1
        } else {
          sub_menu_idx = sub_menu_length - 1;
          menu_idx = convertSubMenuIdxToMenuIdx(sub_menu_idx, menu_type);
          if (sub_menu_length >= 3) {
            menu_indexes[0] = (menu_indexes[0] & 0b11110000) | 0b00000010;
            menu_indexes[2] = (menu_indexes[2] & 0b11110000) | ((menu_idx - 2) & 0b00001111);
            menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx - 1) & 0b00001111);
            menu_indexes[4] = (menu_indexes[4] & 0b11110000) | (menu_idx & 0b00001111);
          } else {
            if (sub_menu_length == 1) {
              menu_indexes[0] = menu_indexes[0] & 0b11110000;
              menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
              menu_indexes[3] |= 0b00001111;  // = -1
              menu_indexes[4] |= 0b00001111;  // = -1
            } else if (sub_menu_length == 2) {
              menu_indexes[0] = (menu_indexes[0] & 0b11110000) | 0b00000001;
              menu_indexes[2] = (menu_indexes[2] & 0b11110000) | ((menu_idx - 1) & 0b00001111);
              menu_indexes[3] = (menu_indexes[3] & 0b11110000) | (menu_idx & 0b00001111);
              menu_indexes[4] |= 0b00001111;  // = -1
            }
          }
        }
      }
      break;
    case DOWN:
      if ((menu_indexes[0] & 0b00001111) == 1) {
        if (sub_menu_idx > 0 && sub_menu_idx < sub_menu_length - 1 && sub_menu_length >= 3) {
          sub_menu_idx++;
          menu_idx = convertSubMenuIdxToMenuIdx(sub_menu_idx, menu_type);
          // Serial.println(menu_idx);
          menu_indexes[0] = (menu_indexes[0] & 0b11110000) | 0b00000010;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | ((menu_idx - 2) & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx - 1) & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | (menu_idx & 0b00001111);
          // Serial.println(menu_indexes[4] & 0b00001111);
        } else if (sub_menu_idx == 1 && sub_menu_length == 2) {
          sub_menu_idx = 0;
          menu_idx = convertSubMenuIdxToMenuIdx(sub_menu_idx, menu_type);
          menu_indexes[0] &= 0b11110000;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
          menu_indexes[4] |= 0b00001111;  // = -1
        }
      } else if ((menu_indexes[0] & 0b00001111) == 0) {
        if (sub_menu_idx >= 0 && sub_menu_idx <= sub_menu_length - 3) {
          sub_menu_idx++;
          menu_idx = convertSubMenuIdxToMenuIdx(sub_menu_idx, menu_type);
          menu_indexes[0] = (menu_indexes[0] & 0b11110000) | 0b00000001;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | ((menu_idx - 1) & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
        }
      } else if ((menu_indexes[0] & 0b00001111) == 2) {
        if (sub_menu_idx >= 2 && sub_menu_idx < sub_menu_length - 1 && sub_menu_length > 3) {
          sub_menu_idx++;
          menu_idx = convertSubMenuIdxToMenuIdx(sub_menu_idx, menu_type);
          menu_indexes[0] = (menu_indexes[0] & 0b11110000) | 0b00000010;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | ((menu_idx - 2) & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx - 1) & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | (menu_idx & 0b00001111);
        } else if (sub_menu_idx >= 2 && sub_menu_idx == sub_menu_length - 1 && sub_menu_length >= 3) {
          sub_menu_idx = 0;
          menu_idx = convertSubMenuIdxToMenuIdx(sub_menu_idx, menu_type);
          menu_indexes[0] &= 0b11110000;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111);
        }
      }
      break;
    case LEFT:
      if(menu_type == BRIGHTNESS_MENU || menu_type == LEVEL_MENU){
        if(menu_type == BRIGHTNESS_MENU){
          if((menu_indexes[0] & 0b00001111) > 0){
            menu_indexes[0]--;
          }
        }
        else{
          if((menu_indexes[0] & 0b00001111) > 1){
            menu_indexes[0]--;
          }
        }
      }
      break;
    case RIGHT:
      if(menu_type == BRIGHTNESS_MENU || menu_type == LEVEL_MENU){
        if((menu_indexes[0] & 0b00001111) < 8){
            menu_indexes[0]++;
        }
      }
      break;
    case OK:
      if(menu_type == BRIGHTNESS_MENU || menu_type == LEVEL_MENU){
        if(menu_type == BRIGHTNESS_MENU){
          brightness_level = menu_indexes[0] & 0b00001111;
          #if SCREEN == 0
            analogWrite(LED_PIN, map(brightness_level, 0, 8, 255, 0));
          #endif
          if(((game_flag >> 3) & 1) == 1){
            menu_idx = 0;
            menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00001000;
          }
          else{
            menu_idx = 1;
            menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000111;
          }
          // save brightness level to eeprom
          writeBrightnessLevel();
        }
        else{
          game_mode_flag = (game_mode_flag & 0b11111000) | ((menu_indexes[0] & 0b00001111) - 1);
          resetGameSpecifications();
          menu_idx = 1;
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000111;
          // load scores from eeprom
          loadScores();
          // save game level to eeprom
          writeGameModeFlag();
          // prevent load game state from eeprom when restart
          #if USING_EEPROM == 1
            uint16_t num_magic_4 = 0;
            EEPROM.put(NUM_MAGIC_4_ADDR, num_magic_4);
          #endif
        }
        game_flag |= 0b00000100; // is_menu_init = 1
        menu_indexes[0] &= 0b11110000;
        menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
        menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
        menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111);
      }
      else{
        strcpy_P(buffer, (char *)pgm_read_ptr(&(menu_table[menu_idx])));
        if (strcmp(buffer, "CONTINUE") == 0) {
            if(MILLIS() - time_init_menu >= 600){
              // continue game
              game_flag = (game_flag & 0b11111100) | 0b00000010;
              display.clearDisplay();
              renderGameInit();
              return;
            }
        } else if (strcmp(buffer, "NEW GAME") == 0) {
          // create new game
          resetGameSpecifications();
          menu_idx = 0;
          menu_indexes[0] &= 0b11110000;
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00001000;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111);
          game_flag = (game_flag & 0b11111100) | 0b00000010;
          game_flag &= 0b01111111;  // is_first_game_init = 0;
          #if USING_EEPROM == 1
            uint16_t num_magic_4 = 0;
            EEPROM.put(NUM_MAGIC_4_ADDR, num_magic_4);
          #endif
          display.clearDisplay();
          return;
        } else if (strcmp(buffer, "1. CLASSIC") == 0 || strcmp(buffer, "2. INFINITY") == 0 || strcmp(buffer, "3. TUNNEL") == 0 || strcmp(buffer, "4. MILL") == 0 || strcmp(buffer, "5. RAILS") == 0 || strcmp(buffer, "6. APARTMENT") == 0) {
          if (strcmp(buffer, "1. CLASSIC") == 0) {
            game_mode_flag &= 0b11000111;
          } else if (strcmp(buffer, "2. INFINITY") == 0) {
            game_mode_flag = (game_mode_flag & 0b11000111) | 0b00001000;
          } else if (strcmp(buffer, "3. TUNNEL") == 0) {
            game_mode_flag = (game_mode_flag & 0b11000111) | 0b00010000;
          } else if (strcmp(buffer, "4. MILL") == 0) {
            game_mode_flag = (game_mode_flag & 0b11000111) | 0b00011000;
          } else if (strcmp(buffer, "5. RAILS") == 0) {
            game_mode_flag = (game_mode_flag & 0b11000111) | 0b00100000;
          } else if (strcmp(buffer, "6. APARTMENT") == 0) {
            game_mode_flag = (game_mode_flag & 0b11000111) | 0b00101000;
          }
          resetGameSpecifications();
          // load scores from eeprom
          loadScores();
          // save game level to eeprom
          writeGameModeFlag();
          // prevent load game state from eeprom when restart
          #if USING_EEPROM == 1
            uint16_t num_magic_4 = 0;
            EEPROM.put(NUM_MAGIC_4_ADDR, num_magic_4);
          #endif
          menu_idx = 1;
          menu_indexes[0] &= 0b11110000;
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000111;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111);
        } else if (strcmp(buffer, "LEVEL") == 0) {
          menu_indexes[0] = (menu_indexes[0] & 0b11110000) | ((game_mode_flag & 0b00000111) + 1);
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000010;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | 0b00001111;  // = -1
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | 0b00001111;  // = -1
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | 0b00001111;  // = -1
          writeGameModeFlag();
          #if USING_EEPROM == 1
            uint16_t num_magic_4 = 0;
            EEPROM.put(NUM_MAGIC_4_ADDR, num_magic_4);
          #endif
        } else if (strcmp(buffer, "BRIGHTNESS") == 0) {
          menu_indexes[0] = (menu_indexes[0] & 0b11110000) | (brightness_level & 0b00001111);
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000101;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | 0b00001111;  // = -1
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | 0b00001111;  // = -1
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | 0b00001111;  // = -1
        } else if (strcmp(buffer, "GAME TYPES") == 0) {
          menu_idx = 9;
          menu_indexes[0] &= 0b11110000;
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000011;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111);
        } else if (strcmp(buffer, "HIGH SCORES") == 0) {
          menu_idx = 0;
          menu_indexes[0] &= 0b11110000;
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000100;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111);
        } else if (strcmp(buffer, "EXIT GAME") == 0) {
          game_flag &= 0b01111011;  // is_first_game_init = 0 & is_menu_init = 0;
          menu_idx = 1;
          menu_indexes[0] &= 0b11110000;
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000111;
          menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
          menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
          menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111);
          display.clearDisplay();
          gameOver();
          DELAY(1000);
          display.clearDisplay();
          // prevent load game state from eeprom when restart
          #if USING_EEPROM == 1
            uint16_t num_magic_4 = 0;
            EEPROM.put(NUM_MAGIC_4_ADDR, num_magic_4);
          #endif
          return;
        }
      }
      break;
    case BACK:
      if (menu_type == GAME_SNAKE_MENU || menu_type == IN_GAME_MENU) {
        // back to game start menu
        game_flag &= 0b11111100;
        display.clearDisplay();
        return;
      } else if (menu_type == HIGH_SCORES_MENU || menu_type == LEVEL_MENU || menu_type == BRIGHTNESS_MENU) {
        if (((game_flag >> 3) & 1) == 1) {  // is_game_init == 1
          menu_idx = 0;
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00001000;
        } else {
          menu_idx = 1;
          menu_indexes[1] = (menu_indexes[1] & 0b11110000) | 0b00000111;
        }
        menu_indexes[0] &= 0b11110000;
        menu_indexes[2] = (menu_indexes[2] & 0b11110000) | (menu_idx & 0b00001111);
        menu_indexes[3] = (menu_indexes[3] & 0b11110000) | ((menu_idx + 1) & 0b00001111);
        menu_indexes[4] = (menu_indexes[4] & 0b11110000) | ((menu_idx + 2) & 0b00001111);
      }
      break;
  }
  renderMenu();
}

void controlGameStartMenu(uint8_t button_value) {
  if(button_value == OK){
    game_flag = (game_flag & 0b11111000) | 0b00000001;
    display.clearDisplay();
    return; 
  }
  renderGameStartMenu();
}

void createFoodEatenSound(void){
  // #if USING_VOLUME_LIB == 0
  //   tone(BUZZER_PIN, 882, 25);
  //   noTone(BUZZER_PIN);
  // #else
  //   vol.tone(882, map(pot_value, 0, 1023, 0, 255));
  //   DELAY(25);
  //   vol.noTone();
  // #endif
  digitalWrite(BUZZER_PIN, HIGH);
  DELAY(25);
  digitalWrite(BUZZER_PIN, LOW);
}
void createGameOverSound(void){
  // #if USING_VOLUME_LIB == 0
  //   tone(BUZZER_PIN, 441, 120);
  //   noTone(BUZZER_PIN);
  // #else
  //   uint8_t volume = map(pot_value, 0, 1023, 0, 255);
  //   vol.tone(441, volume);
  //   DELAY(120);
  //   vol.noTone();
  // #endif
  digitalWrite(BUZZER_PIN, HIGH);
  DELAY(120);
  digitalWrite(BUZZER_PIN, LOW);
}

void resetGameSpecifications(void){
  memset(game_map, 0, sizeof(game_map));

  // init head position in game_map matrix
  head_snake_row_x = 0;
  head_snake_colm_y = 0;
  prev_head_snake_row_x = 0;
  prev_head_snake_colm_y = 0;

  // init tail position in game_map matrix
  tail_snake_row_x = 0;
  tail_snake_colm_y = 0;
  prev_tail_snake_row_x = 0;
  prev_tail_snake_colm_y = 0;
  prev_tail_snake_direct = 0;

  // init food position in game_map matrix
  food_row_x = 0;
  food_colm_y = 0;
  prev_food_row_x = 0;
  prev_food_colm_y = 0;
  
  game_flag &= 0b11110111; // is_game_init = 0
  curr_score = 0;
}
void addCurrScoreToScoresArray(void){
  uint16_t sorted_scores[4];
  for(int i = 0; i < 3; i++){
    sorted_scores[i] = scores[i];
  }
  sorted_scores[3] = curr_score;
  // insertion sort
  for(int i = 1; i < 4; i++){
    uint16_t key = sorted_scores[i];
    int j = i - 1;
    while(j >= 0 && key > sorted_scores[j]){
      sorted_scores[j + 1] = sorted_scores[j];
      j-=1;
    }
    sorted_scores[j + 1] = key;
  }
  for(int i = 0; i < 3; i++){
    scores[i] = sorted_scores[i];
  }
}
void gameOver(void) {
  createGameOverSound();
  display.clearDisplay();
  display.setCursor(12, 15);
  display.setTextColor(SHOW);
  display.print("GAME OVER");
  display.setCursor(12, 25);
  display.print("SCORE: ");
  display.print(curr_score);
  display.display();
  addCurrScoreToScoresArray();
  resetGameSpecifications();
  // prevent load game state from eeprom when restart
  #if USING_EEPROM == 1
    uint16_t num_magic_4 = 0;
    EEPROM.put(NUM_MAGIC_4_ADDR, num_magic_4);
  #endif
  writeScores();
}