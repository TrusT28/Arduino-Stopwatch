
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
  unsigned char phase;  /* index of next place to be displayed 0-3 refer to place[], 4==disp off */
};

void init(Disp& d);   /* to suppress Arduino IDE errors */
unsigned char redraw(Disp& d, long now);
void set_number_to_display(Disp& d, unsigned long t);

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
    if(i<4){
      disp_7seg(i,d.place[i]);
      d.deadline+=t_disp_on;
      i++;
      d.phase=i;
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

void set_number_to_display(Disp& d, unsigned long t)
{
  // First set only milliseconds at rightmost digit
  unsigned long ms = ((t / 1000) % 1000) / 100; //Milliseconds
  d.place[0] = font[ms];
  
  // Now set up seconds for the rest of the digits
  unsigned long sec = (t / 1000000); //Seconds
  
  int limit=4; // We do not show digits further than at index 'limit'
  for (int i = 1; i < 4; i++) {
    if(i>limit){
      d.place[i]=0b00000000; // Set display at this digits to show nothing
    }
    else {
    unsigned long digit = sec % 10;
    sec /= 10;
    unsigned char value = font[digit];
    // Set up the dot to distiguish between seconds and milliseconds
    if (i == 1)
      value = value | 0b00000001;
    d.place[i] = value;
    
    if (sec == 0)
       // No more digits in seconds to set. Stop showing digits
       limit=i;
    }
  }
}

// *** SETUP HELPERS *** \\

enum states {Stopped, Running, Lapped};

enum states state;
long now;
long prev;
long timer;
const long milli = 1000;
Disp display;
Button button_A, button_B, button_C;

// *** Start Code *** \\

void setup()
{
//  Serial.begin(9600);
//  while (!Serial);

  button_init(button_A, button1_pin);
  button_init(button_B, button2_pin);
  button_init(button_C, button3_pin);
  init(display);
  set_number_to_display(display, 0);
  prev = micros();
  state = Stopped;
}

void loop()
{
  now = micros();
  
  if(redraw(display, now)){
    switch(state){
    case Stopped:
        if(get_pulse(button_A)) {
          state=Running;
          prev = micros();
        }
        else if(get_pulse(button_C)) {
          prev=micros();
          timer=0;
          set_number_to_display(display,  timer);
        }
        break;
    case Running:
       if(get_pulse(button_A)){
        state=Stopped;
       }
       if(get_pulse(button_B)) {
        state=Lapped;
      }
      break;
    case Lapped:
      if(get_pulse(button_B)){
        state=Running;
      }
      break;
   }
  }
  long elapsed = duration(now, prev);
  if ((state!=Stopped) && elapsed >= milli) { //1 millisecond passed, update timer
    timer += elapsed;
    if(state!=Lapped){
      set_number_to_display(display,  timer);
    }
    prev = now;
  }
}
