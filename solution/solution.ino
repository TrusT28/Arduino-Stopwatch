
#include "funshield.h"
#ifndef slowDigitalWrite
#define slowDigitalWrite(a,b) digitalWrite(a,b)
#define constDigitalWrite(a,b) digitalWrite(a,b)
#endif

// ***SETUP BUTTONS*** \\

const int t_debounce = 10000; //[us]
const long t_slow = 1000000;   /* this amounts to 1 sec */
const long t_fast = 300000; /* 300ms */

enum ButtonState {UP, DEBOUNCING, DOWN};

struct Button {
  long deadline;
  ButtonState state;
  unsigned char pin;
};

void button_init(Button& b, int which) {
  b.pin = which;
  b.state = UP;
  pinMode(which, INPUT);
}

long duration(long now, long then) {
  return ((unsigned long)now) - then;
}

int get_pulse(Button& b) {
  if (digitalRead(b.pin) == OFF) {
    b.state = UP;
    return 0;
  }
  long now = micros();  /* the button has been pressed, which brought us here */
  switch (b.state) {
    case UP:
      b.deadline = now + t_debounce;
      b.state = DEBOUNCING;
      return 0;
    case DEBOUNCING:
      if (duration(now, b.deadline) < 0) return 0;
      b.state = DOWN;
      b.deadline += t_slow;
      return 1;
    case DOWN:
      if (duration(now, b.deadline) < 0) return 0;
      b.deadline += t_fast;
      return 1;
  }
  return 0; // This is never reached. Just to fix warning: control reaches end of non-void function [-Wreturn-type] bug
}

// *** SETUP SCREEN *** \\

struct Disp {
  unsigned char place[4]; /* place[0] is the rightmost digit, place[3] the leftmost one */
  long deadline;          /* place[] hold data in a translated form */
  unsigned char phase;    /* index of place until which we display digits, 0-3 refer to place[], 4==disp off */
};

void init(Disp& d);   /* to suppress Arduino IDE errors */
unsigned char redraw(Disp& d, long now);
void translate(Disp& d, unsigned long t);

const long t_disp_on = 60;   /* [us] duration of a given LED being on */
const long t_disp_off = 160; /* [us] duration of the display being off */

inline void shift_out8(unsigned char data)
{
  for (signed char i = 7; i >= 0; i--, data >>= 1) {
    digitalWrite(data_pin, data & 1); /* set up the data bit */
    constDigitalWrite(clock_pin, HIGH); /* upon clock rising edge, the 74HC595 will shift the data bit in */
    constDigitalWrite(clock_pin, LOW);
  }
}

void shift_out(unsigned int bit_pattern)
{
  shift_out8(bit_pattern);
  shift_out8(bit_pattern >> 8);
  constDigitalWrite(latch_pin, HIGH); /* upon latch_pin rising edge, both 74HC595s will instantly change its outputs to */
  constDigitalWrite(latch_pin, LOW); /* its internal value that we've just shifted in */
}

void disp_init()
{
  pinMode(latch_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  digitalWrite(clock_pin, LOW);
  digitalWrite(latch_pin, LOW);
  shift_out(0);  /* turn all the display LEDs off */
}

void disp_7seg(unsigned char column, unsigned char glowing_leds_bitmask)
{
  shift_out((1 << (8 + 4 + column)) | (0xFF ^ glowing_leds_bitmask));
}

void init(Disp& d)
{
  d.phase = 1;
  d.deadline = micros() + t_disp_off;
  disp_init();
  for (int i = 0; i < 4; i++) d.place[i] = 0;
}

// font is array of numbers for screen. 0th element is zero, 1st element is one and so on until nine
const unsigned char font[] = {0b11111100, 0b01100000, 0b11011010, 0b11110010, 0b01100110, 0b10110110, 0b10111110, 0b11100000, 0b11111110, 0b11110110};

/* Returns 1 upon the start of the blank phase, 0 otherwise
*/

unsigned char redraw(Disp& d, long now)
{
  if (duration(now, d.deadline) >= 0) {
    int i = d.phase;
    if (i < 4) {
      for (int j = 0; j <= i; j++) {
        disp_7seg(j, d.place[j]);
      }
      for(int j=i+1; j<4; j++){
        disp_7seg(j, 0b00000000);
      }
      d.deadline += t_disp_on;
      return 0;
    } else {
      d.deadline += t_disp_off;
      shift_out(0);
      d.phase = 0;
      return 1;
    }
  }
  return 0;
}

void translate(Disp& d, unsigned long t)
{
  unsigned long ms = ((t / 1000) % 1000) / 100; //Milliseconds
  //disp_7seg(0, font[ms]);
  d.place[0] = font[ms];
  unsigned long sec = (t / 1000000); //Seconds
  for (int i = 1; i < 4; i++) {
    unsigned long digit = sec % 10;
    sec /= 10;
    unsigned char value = font[digit];
    if (i == 1)
      value = value | 0b00000001;
    d.place[i] = value;
    if (sec == 0) { // Stop showing digits
      d.phase = i;
      break;
    }
  }
}

// *** SETUP HELPERS *** \\


int state = 0;
int switch_state = 0;
bool updated = false;
long now;
long prev;
long timer;
const long milli = 1000;
bool on = false;
bool lap = false;

Disp display;
Button button_plus, button_minus, button_switch;

// *** Start Code *** \\

void setup()
{
//  Serial.begin(9600);
 // while (!Serial);

  button_init(button_plus, button1_pin);
  button_init(button_minus, button2_pin);
  button_init(button_switch, button3_pin);
  init(display);
  translate(display, 0);
  prev = micros();
}

void loop()
{
  now = micros();
  
  redraw(display, now);
  long elapsed = duration(now, prev);
  if (on && elapsed >= milli) { //1 millisecond passed
    timer += elapsed;
    unsigned long ms = ((timer / 1000) % 1000) / 100; //Milliseconds
    unsigned long sec = (timer / 1000000);  //Seconds
 //   Serial.print(sec);
 //   Serial.print(":");
 //   Serial.println(ms);
 
 //   if not button2
    if(!lap){
      translate(display,  timer);
    }
    prev = now;
  }

  if (updated) {
    if (state > 0 && !lap) {
      on = !on;
      if (on)
        prev = micros();
    }
   else if (state < 0 && on){
      lap=!lap;
   }
    if (switch_state > 0 && !on){
      prev=micros();
      timer=0;
      translate(display,  timer);
    }
      
    state = 0;
    switch_state = 0;
    updated = false;
  }

  int newState = state;
  newState += get_pulse(button_plus);
  newState -= get_pulse(button_minus);
  switch_state += get_pulse(button_switch);
  if (newState != state || switch_state > 0) {
    updated = true;
    state = newState;
  }
}
