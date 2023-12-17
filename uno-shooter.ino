// Uno Shooter

#include "LedControl.h"

const uint8_t pin_button_left = 2;
const uint8_t pin_button_right = 3;
const uint8_t pin_button_fire = 4;

const uint8_t pin_7s_DS = 5;
const uint8_t pin_7s_STCP = 6;
const uint8_t pin_7s_SHCP = 7;

const uint8_t pin_buzzer = 9;

LedControl matrix = LedControl(12, 11, 10, 1);

const uint8_t digits[10]{1, 79, 18, 6, 76, 36, 32, 15, 0, 4};

const uint8_t drop_interval = 5;
volatile uint8_t time_left;

volatile bool flag_left = false;
volatile bool flag_right = false;
volatile bool flag_fire = false;
volatile bool flag_drop = false;
volatile bool flag_sec = false;

bool stop = true;
uint8_t score = 0;

uint8_t player_row = 7, player_col = 0;
uint8_t enemy_row = 0, enemy_col = 0;

void setup() {
  pinMode(pin_7s_DS, OUTPUT);
  pinMode(pin_7s_STCP, OUTPUT);
  pinMode(pin_7s_SHCP, OUTPUT);

  pinMode(pin_button_left, INPUT_PULLUP);
  pinMode(pin_button_right, INPUT_PULLUP);
  pinMode(pin_button_fire, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pin_button_left), handle_left, FALLING);
  attachInterrupt(digitalPinToInterrupt(pin_button_right), handle_right,
                  FALLING);

  PCICR |= B00000100;
  PCMSK2 |= B00010000;

  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << OCIE1A);

  matrix.shutdown(0, false);
  matrix.setIntensity(0, 0);

  randomSeed(analogRead(0));
}

void loop() {
  if (stop) {
    if (!flag_fire) {
      return;
    }
    start_game();
  }

  if (flag_sec) {
    digitalWrite(pin_7s_STCP, LOW);
    shiftOut(pin_7s_DS, pin_7s_SHCP, LSBFIRST, digits[time_left]);
    digitalWrite(pin_7s_STCP, HIGH);
    flag_sec = false;
  }

  if (player_row == enemy_row) {
    stop = true;
    score_animation();
    flag_fire = false;
    return;
  }

  if (flag_fire) {
    tone(pin_buzzer, 2794, 40);
    bullet_animation();
    flag_fire = false;
  }

  update_enemy();
  update_player();
}

void start_game() {
  matrix.clearDisplay(0);

  enemy_row = 0;
  enemy_col = random(0, 8);
  matrix.setLed(0, enemy_row, enemy_col, true);

  player_col = random(0, 8);
  matrix.setLed(0, player_row, player_col, true);

  stop = false;
  flag_left = false;
  flag_right = false;
  flag_fire = false;
  flag_drop = false;
  time_left = drop_interval;
  TCNT1 = 15624;
}

void score_animation() {
  matrix.clearDisplay(0);
  for (uint8_t i = 0; i < 8; ++i) {
    for (uint8_t j = 0; j < 8; j++) {
      if (score == 0) {
        return;
      }
      matrix.setLed(0, i, j, true);
      --score;
      delay(100);
    }
  }
}

void bullet_animation() {
  for (int8_t row = 6; row >= 0; --row) {
    matrix.setLed(0, row, player_col, true);
    delay(10);
    matrix.setLed(0, row, player_col, false);

    if (row == enemy_row) {
      if (player_col == enemy_col) {
        tone(pin_buzzer, 3322, 80);
        ++score;
        new_enemy();
        break;
      }
    }
  }
}

void update_enemy() {
  if (flag_drop) {
    matrix.setLed(0, enemy_row, enemy_col, false);
    ++enemy_row;
    enemy_col = random(0, 2) ? ++enemy_col : --enemy_col;
    enemy_col = enemy_col % 8;
    flag_drop = false;
  }

  matrix.setLed(0, enemy_row, enemy_col, true);
}

void update_player() {
  matrix.setLed(0, player_row, player_col, false);

  if (flag_left) {
    player_col = --player_col % 8;
    flag_left = false;
  }

  if (flag_right) {
    player_col = ++player_col % 8;
    flag_right = false;
  }

  matrix.setLed(0, player_row, player_col, true);
}

void new_enemy() {
  uint8_t new_enemy_col;
  do {
    new_enemy_col = random(0, 8);
  } while (enemy_col == new_enemy_col);
  enemy_col = new_enemy_col;
  matrix.setLed(0, enemy_row, enemy_col, true);
}

void handle_button(volatile bool &flag, volatile unsigned long &last_time) {
  unsigned long time = millis();
  if (time - last_time > 75UL) {
    flag = true;
  }
  last_time = time;
}

void handle_left() {
  volatile static unsigned long last_time = 0;
  handle_button(flag_left, last_time);
}

void handle_right() {
  volatile static unsigned long last_time = 0;
  handle_button(flag_right, last_time);
}

ISR(PCINT2_vect) {
  volatile static unsigned long last_time = 0;
  if (digitalRead(pin_button_fire) == LOW) {
    handle_button(flag_fire, last_time);
  }
}

ISR(TIMER1_COMPA_vect) {
  flag_sec = true;
  if (--time_left == 0) {
    time_left = drop_interval;
    flag_drop = true;
  }
}
