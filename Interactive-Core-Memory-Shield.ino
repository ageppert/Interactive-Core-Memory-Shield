/*
BUGS:


TROUBLESHOOT:
Sometimes snake head seems to be stuck with nowhere to go even though there are open spaces and stylus is detected there.
System seems unstable (connection) and the first core ends up setting itself.
Noticed the timing between the 29th and 30th bits wiggles (with #define ISOLATE_ONE_CORE enabled) when testing active stylus
2nd and 3rd row from the bottom start to fill in and end up setting all the LEDs on after a few seconds, sometimes. Seems like a loose conenction.

TO DO:
Crashing into the existing snake body should result in game over.
Test for trapped snake  head - because that's game over!
Eliminate the need for the button and use gestures only.
Reduce Global Variables use of all that RAM.
Startup, read and store core state, twinkle fade to off, scroll CORE MEMORY SHIELD, display starting core state
Wait for 20 seconds of no interaction, store core state, go to demo mode, screen saver, Matrix. Upon stylus sensing, return to saved screen.
Update drawing mode to have four colors to select from along left edge. BLANK, RED, GREEN, BLUE. Then draw in screen memory with stylus. 
Duration of stylus over a core increase intensity of chosen color.
*/

/*
  core_memory_shield_neopixel_andy by Andy Geppert 2018-2019

  This is a single Arduino configured to display the status of the bits in
  a core memory shield on an Adafruit NeoPixel Featherwing LED Matrix. The intent is to use a
  magnetic stylus on the core memory shield and visualize the cores magnetic state.
  TO DO: Future extension to allow control with an active stylus to allow setting and clearing of cores!

  Learning: how to transpose logical arrangement of bits to physical arrangement in the core layout.
  
  SEND HARDWARE AND SOFTWARE:
  Core memory shield running on Arduino Uno
  This program "core_memory_shield_andy" is being written in Arduino 1.8.5
    Arduino with USB up. Featherwing with reset button on the right.

  
  CONNECTIONS - ANALOG SIDE (LEFT) - Counting from the capacitor at the core end
  1st No Connection
  (yellow under board, 2nd from capacitor)  3.3V taps from Arduino to Core Shield Board 
  (red, 3rd from capacitor) 5V to Featherwing (3rd pin from the left, on top row)
  (black, 4th from capacitor) GND to Featherwing (4th pin from the left, bottom row)
  5th No Connection - GND
  6th No Connection - VIN
  
  (blue, 7th from capacitor) D14 (A0) to Featherwing Digital Input DIN on left edge
  8th No Connection
  9th No Connection
  10th No Connection
  11th No Connection
  12th D19 (A5) (orange) far end to external stylus pulse coil

  CONNECTIONS - DIGITAL SIDE (RIGHT) - Counting correctly from the bottom of the Arduino
  1st D0 Serial TXD
  2nd D1 Serial RXD
  3rd D2 Core Shield Enable
  4th D3 Core Shield ADDR0
  5th D4 Core Shield ADDR1
  6th D5 Core Shield ADDR2
  7th D6 Core Shield ADDR3
  8th D7 Core Shield ADDR4

  9th D8 Core Shield WR
  10th D9 Core Shield RD
  11th D10 
  12th D11 
  13th D12 CLEAR BUTTON - pull low to clear cores.
  14th D13  
  15th GND Connect other end of button to ground.
  16th AREF 
  17th I2C/SCL (not brought to the top of the Core Shield)
  18th I2C/SDA (not brought to the top of the Core Shield) 

  CHANGE HISTORY FROM PRIOR VERSION:
  2018-10-19 Serial is 115200. Works with Arduino Uno but not Arduino Mega for some reason. Configure serial monitor to send no line ending. 
  2018-10-19 Adding a mode to constantly read and stream bytes out serial port for display on Rainbowduino. Press "o" to begin constant stream. Press "O" to stop it.
  2019-02-08 Changed serial to 9600 for testing. Test output stream counts up and works, with a delay to accommodate the slow receive side. Need to output the real bytes now!
  2019-02-14 Save as to new program. Added NeoPixel Library and test code. It's alive! Setup with NEO_GRB on NEO_GRBW.
  2019-02-17 Reading bits and updating LEDs.
  2019-02-18 Removed wait() and convert to polling for time to pass between tasks/updates. Triple write to cores to be sure they clear.
  2019-03-15 Add test mode to isolate and test a single core for stylus control
  2019-03-30 Added clear button between D12 and GND. Disabled "always clear at start up" to show the non-volatile nature of the cores.
  2019-03-31 Added fast digital write to be able to time the stylus pulse closer. Moved stylus pulse inside write_bit, adjusted timing manually to occur over the sense signal.
  2019-04-10 Added Snake Game file and placeholder functions.
  2019-04-11 Created core scan to report which bit changed, without updating the display.
  2019-04-13 Cleaned up state machine, but broke the stylus detection point functionality, rolled back to 04-12, got it working again. Very strange problem in state 3.
  2019-04-14 Add state 3 hand off to snake game with debug. Moved snake game back to single file to avoid dealing with data between files. Go global!
  2091-04-15 Move gesture mode to game to start game over. Basic full game play is done!

*/

// Rough-and-ready Arduino code for testing, calibrating and using core memory shield.
//
// Copyright 2011 Ben North and Oliver Nash.
//
// Modified by Jussi Kilpeläinen 2016-04-17 for his own core memory shield (http://jussikilpelainen.kapsi.fi/?p=213)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <Adafruit_NeoPixel.h>
#define PIN 14 // A0 or D14 are the same thing, different use modes
#define NUM_LEDS 32
#define BRIGHTNESS 15
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);


//#define ANDY_OUTPUT_CONSTANT_DATA_STREAM
#define ANDY_TEST_REAL_BYTES
//#define ANDY_READ_LOGICAL_DATA_WORD
#define ANDY_READ_PHYSICAL_DATA_WORD
//#define ANDY_STARTUP_MESSAGE
#define ANDY_INVERT_LED_STATE
//#define START_WITH_ALL_BITS_SET
//#define ISOLATE_ONE_CORE
  #define ISOLATE_CORE_BIT 31
#define STYLUS_ENABLE
#define STYLUS_PIN 19 // Will use FastDigitalPin library 
#include <FastDigitalPin.h> // Arduino Library Manager provides this from https://github.com/hippymulehead/FastDigitalPin
FastDigitalPin StylusPin(STYLUS_PIN);
#define CLEAR_BUTTON_INPUT_PIN 12
#define DEBUG_SNAKE_IN_TOP_LEVEL_STATE_MACHINE
#include "CharacterMap.h"

#ifdef ANDY_OUTPUT_CONSTANT_DATA_STREAM
static unsigned int constant_data_stream_mode_enabled  = true;
#endif
//

static unsigned int PhysicalCoreStartupPattern[4];


#define ADDRSIZE      5
#define WORDSIZE      (1 << ADDRSIZE)
#define ENABLE        B00000100 // PORTD
#define DRD           B00000010 // PORTB
#define DWR           B00000001 // PORTB

static unsigned int WRITE_ON_US  = 3;
  static unsigned int STYLUS_ON_US  = 1; // Andy added to add up to 3, but insert stylus pulse.
  static unsigned int STYLUS_OFF_US  = 2; // Andy added to add up to 3, but insert stylus pulse.
static unsigned int WRITE_OFF_US = 5; // May not be necessary.

static unsigned long errs = 0;
static unsigned int sense_test_addr = 0;
static int n_test_iters = 1000;
static char report_errors_p = 0;
static char trace_core_calls_p = 0;

const unsigned long NeopixelUpdatePeriod = 25 ; // ms (also reads the cores)
const unsigned long SerialPacketUpdatePeriod = 50 ; // ms
const unsigned long CoreChangeDetectUpdatePeriod = 30 ; // ms (effectively debounces the stylus movement)
const unsigned long GestureTimeout = 3000 ; // ms
const unsigned long SnakeGameUpdatePeriod = 30 ; // ms
const unsigned long ScrollUpdatePeriod = 300; // ms
volatile unsigned long NeopixelUpdateLastRunTime;
volatile unsigned long SerialPacketUpdateLastRunTime;
volatile unsigned long CoreChangeDetectUpdateLastRunTime;
volatile unsigned long SnakeGameUpdateLastRunTime;
volatile unsigned long ScrollUpdateLastRunTime;
volatile unsigned long nowTime;
volatile unsigned long StartReadTime;
volatile unsigned long EndReadTime;
volatile unsigned long ReadTime;
volatile unsigned long StartLoopTime;
volatile unsigned long EndLoopTime;
volatile unsigned long LoopTime;
volatile unsigned long GameOverTime;
volatile unsigned long GameOverTimeAutoReset = 2250 ; // ms restart the game automaticaly
volatile int pixel_number = 0;
volatile bool ClearCores = 0;
volatile unsigned int TopLevelStateMachine = 0;                    // start with game for testing
volatile unsigned int GameState = 0;
volatile unsigned int GestureDetectState = 0;
bool GameOver = false;
bool Winner = false;
volatile unsigned long CorePhysicalStateChanged = 0;
volatile unsigned long ButtonPressDuration_ms = 0;
volatile unsigned int SnakeHeadX; // lower left, remember the logical array position is backwards how it is written in source code.
volatile unsigned int SnakeHeadY; 
volatile signed int SnakeLength;
volatile signed int OldSnakeLength;
volatile unsigned int TestX = 0;
volatile unsigned int TestY = 0;
bool MovementDetected = false;

volatile int screen_memory[4][8] = { // 4 rows of bytes with 8 columns of bits
    { 0,-2,0,-2,0,0,-1, 0},
    { 0,-2,-2,0,0,0,-2,-1},
    { 0, 0,0,0,0,-1, 0,-2},
    { 0,-2,0,-2,0,0, 0,-1} // Correct ! <- This corner nearest sense wire solder pads.
  }; 

volatile int stylus_memory[4][8] = { // 4 rows of bytes with 8 columns of bits
    { 0,0,0,0,0,0,0,0},
    { 0,0,0,0,0,0,0,0},
    { 0,0,0,0,0,0,0,0},
    { 0,0,0,0,0,0,0,0} // Correct ! <- This corner nearest sense wire solder pads.
  };
  
const int brightness_position[32] = { // 4 rows of bytes with 8 columns of bits
    190,180, 90, 80, 80, 90,180,190,
    190,180, 90, 80, 80, 90,180,190,
    190,180, 90, 80, 80, 90,180,190,
    190,180, 90, 80, 80, 90,180,190      // Correct ! <- This corner nearest sense wire solder pads. This is LED #31.
  };

void write_bit(char n, const int v)
{
  if (trace_core_calls_p)
  {
    char buf[64];
    sprintf(buf, "x[0%02d] <- %d", n, v);
    Serial.println(buf);
  }

  // Assert 0 <= n <= 31, v == 0 or 1.
  noInterrupts();
  if(v == 0)
  {
    PORTB &= (~DWR);
  }
  else
  {
    PORTB |= DWR;
  }
  PORTD = ((n << 3) & (~ENABLE));
  PORTD |= ENABLE; // Enable separately to be safe.
  //delayMicroseconds(WRITE_ON_US);
    #ifdef STYLUS_ENABLE
    //delayMicroseconds(STYLUS_ON_US);
    StylusPin.digitalWrite(HIGH);
    StylusPin.digitalWrite(LOW);    // The pulse is 1 us, timed to occur over expected sense pulse.
    delayMicroseconds(STYLUS_OFF_US);
    #endif
  PORTD &= (~ENABLE);
  delayMicroseconds(WRITE_OFF_US);
  interrupts();
}

int exchg_bit(char n, const int v)
{
  write_bit(n, v);

  if(PINB & DRD)
  {
    // Switching occurred so core held opposite of
    // what we just set it to.

    if (trace_core_calls_p)
      Serial.println("___ -> CHANGE");

    return !v;
  }
  else
  {
    // No switching occurred so core held whatever
    // we just wrote to it.

    if (trace_core_calls_p)
      Serial.println("___ -> NO-CHANGE");

    return v;
  }
}

int read_bit(const int n)
{
  char buf[64];
  if (trace_core_calls_p)
  {
    sprintf(buf, "x[0%02d] -> ___", n);
    Serial.println(buf);
  }
  write_bit(n, 0);
  if(PINB & DRD)
  {
    // Switching occurred so core held 1.

    if (trace_core_calls_p)
      Serial.println("___ -> 1");

    write_bit(n, 1); // Put it back!
    return 1;
  }
  else
  {
    // No switching occurred so core held 0 (and still holds it).

    if (trace_core_calls_p)
      Serial.println("___ -> 0");

    return 0;
  }
}

void write_word(unsigned long v)
{
  int n;
  for(n = 0; n < WORDSIZE; n++)
  {
    write_bit(n, v & 1);
    v >>= 1;
  }
}

void write_physical_word(unsigned long v)
{
  const int physical_position[32] = {
  //{ 0, 3, 4, 7, 1, 2, 5, 6}
      6, 5, 2, 1, 7, 4, 3, 0,  
  //{ 9,10,13,14, 8,11,12,15},
     15,12,11, 8,14,13,10, 9,
  //{16,19,20,23,17,18,21,22},
     22,21,18,17,23,20,19,16,
  //{25,26,29,30,24,27,28,31},
     31,28,27,24,30,29,26,25 };
  int n;
  for(n = 0; n < WORDSIZE; n++)
  {
    write_bit(physical_position[n], v & 1);
    v >>= 1;
  }
}

unsigned long read_logical_word(void)
{
  unsigned long v = 0;
  int n;
  for(n = WORDSIZE-1; n >= 0; n--)
  {
    v <<= 1;
    v |= read_bit(n);
  }
  return v;
}

unsigned int read_word_physical_row(unsigned int row)            // Andy added function to read bits in order of physical placement on board, a byte at a time.
{
  /*  Create four bytes by polling bit status of physical positions. Row is 0, 1, 2, or 3. Column is 0-7 bits, MSB listed first.
   *  
   */
  unsigned int v = 0;
  const int physical_position[4][8] = { // 4 rows of bytes with 8 columns of bits

    {25,26,29,30,24,27,28,31},
    {16,19,20,23,17,18,21,22},
    { 9,10,13,14, 8,11,12,15},
    { 0, 3, 4, 7, 1, 2, 5, 6} // Correct ! <- This corner nearest sense wire solder pads.

/*    
    {31,23,15,7},  // This is the original listing from Jussi, but doesn't seem right. I'm pretty sure it's wrong.
    {29,21,13,5},
    {27,19,11,3},
    {25,17,9,1},
    {30,22,14,6},
    {28,20,12,4},
    {26,18,10,2},
    {24,16,8,0}

    {31,22,15,6},  // This is Andy's updated list by trial and error. #6 is nearest sense wire start/end. This transposed above and works with a magnet probe check!
    {28,21,12,5},
    {27,18,11,2},
    {24,17,8,1},
    {30,23,14,7},
    {29,20,13,4},
    {26,19,10,3},
    {25,16,9,0}
*/
  }; 
  int col;
  for(col = 0; col <= 7; col++)
  {
    v <<= 1;
    v |= read_bit(physical_position[row][col]);
  }
  return v; // byte
}                                                     // End of Andy's changes.

bool read_bit_physical_position(unsigned int position_number)            // Andy added function to read bits in order of physical placement on board, a bit at a time.
{
  bool bit_state = 0;
  const int physical_position[32] = { // 4 rows of bytes with 8 columns of bits
    25,26,29,30,24,27,28,31,
    16,19,20,23,17,18,21,22,
     9,10,13,14, 8,11,12,15,
     0, 3, 4, 7, 1, 2, 5, 6           // Correct ! <- This corner nearest sense wire solder pads.
  };
  #ifdef ISOLATE_ONE_CORE
  position_number = ISOLATE_CORE_BIT;
  #endif
  bit_state = read_bit(physical_position[position_number]);
  return bit_state;
}                                                     // End of Andy's changes.

unsigned long exchg_word(unsigned long v)
{
  unsigned long v_prev;
  int n;
  for (n = 0; n < WORDSIZE; ++n)
  {
    unsigned long b = exchg_bit(n, v & 1);
    v >>= 1;
    v_prev >>= 1;
    v_prev |= (b << 31);
  }
  return v_prev;
}

static void maybe_report_error(int i, int j, const char * lbl)
{
  char buf[64];

  if (!report_errors_p)
    return;

  sprintf(buf, "(%o, %o): %s", i, j, lbl);
  Serial.println(buf);
}

static void toggle_error_reporting()
{
  char buf[64];
  report_errors_p = !report_errors_p;
  sprintf(buf, "report-errors-p now %d", report_errors_p);
  Serial.println(buf);
}

static void toggle_tracing()
{
  char buf[64];
  trace_core_calls_p = !trace_core_calls_p;
  sprintf(buf, "trace-core-calls-p now %d", trace_core_calls_p);
  Serial.println(buf);
}

static void current_calibration()
{
  Serial.print("pulsing for current calibration...");
  while (Serial.available() == 0)
  {
    int i;
    for (i = 0; i < WORDSIZE; ++i)
    {
      write_bit(i, 0);
      write_bit(i, 1);
    }
    delayMicroseconds(100);
  }
  Serial.println(" stopped");
  Serial.read(); /* discard typed character */
}

static void timing_exchange_word()
{
  unsigned long val_0 = read_logical_word();
  unsigned long i;
  
  Serial.print("start...");
  for (i = 0; i < 12000; ++i)
  {
    exchg_word(val_0);
    exchg_word(val_0);
    exchg_word(val_0);
    exchg_word(val_0);
    exchg_word(val_0);
    exchg_word(val_0);
    exchg_word(val_0);
    exchg_word(val_0);
    exchg_word(val_0);
    exchg_word(val_0);
  }
  Serial.println("stop");
}

static void echo_comment()
{
  Serial.print("# ");
  char c;
  do {
    while (Serial.available() == 0);
    c = Serial.read();
    Serial.print(c);
  } while (c != '\r');
  Serial.println("");
}

static void corewise_set_test()
{
  char buf[64];
  for (int b = 0; b < 2; ++b)
  {
    errs = 0;
    for (int n = 0; n < n_test_iters; ++n)
      for (int i = 0; i < WORDSIZE; ++i)
      {
        write_bit(i, b);
        if (read_bit(i) != b) ++errs;
      }
    sprintf(buf, "set %d %lu %d", b, errs, n_test_iters);
    Serial.println(buf);
  }
}

static void corewise_interfere_test()
{
  char buf[64];
  for (int b0 = 0; b0 < 2; ++b0)
  {
    for (int b1 = 0; b1 < 2; ++b1)
    {
      errs = 0;
      for (int n = 0; n < n_test_iters; ++n)
      {
        for (int i = 0; i < WORDSIZE; ++i)
        {
          for (int j = 0; j < WORDSIZE; ++j)
          {
            if (j == i) continue;
            
            write_bit(i, b0);
            write_bit(j, b1);
            if (read_bit(i) != b0) {
              if (report_errors_p)
              {
                // Redundant test but sprintf() takes time.
                sprintf(buf, "%d %d", b0, b1);
                maybe_report_error(i, j, buf);
              }
              ++errs;
            }
          }
        }
      }
      sprintf(buf, "interfere %d %d %lu %d", b0, b1, errs, n_test_iters);
      Serial.println(buf);
    }
  }
}

static void corewise_tests()
{
  corewise_set_test();
  corewise_interfere_test();
}

static void R_test()
{
  unsigned long d = read_logical_word();
  Serial.print("Core data read: 0x");
  Serial.print(d, HEX);
  Serial.print(" = B");
  Serial.println(d, BIN);
  Serial.println("Physical Positions:");  // Andy added to display physical placement of bits where the USB port is up in the serial text output view.
  int thisRow;
  int n;
  for(n = 0; n <= 3; n++)
  {
    thisRow = read_word_physical_row(n);
      for (unsigned int mask = 0x80; mask; mask >>= 1)
      {
         Serial.print(" ");
         if (mask & thisRow)
         {
             Serial.print('1');
         }
         else 
         {
             Serial.print('0');
         }
      }
      Serial.println();
   }                                                    // End of Andy's changes.
}

static void s_test()
{
  char buf[64];
  write_bit(sense_test_addr, 0);
  write_bit(sense_test_addr, 0);
  write_bit(sense_test_addr, 1);
  write_bit(sense_test_addr, 1);
  sprintf(buf, "Pulsed core 0%02d", sense_test_addr);
  Serial.println(buf);
}

static void report_sense_addr()
{
  char buf[64];
  sprintf(buf, "Ready to pulse core 0%02d", sense_test_addr);
  Serial.println(buf);
}

static void a_test()
{
  sense_test_addr += 1;
  sense_test_addr %= 32;
  report_sense_addr();
}

static void A_test()
{
  sense_test_addr -= 1;
  sense_test_addr %= 32;
  report_sense_addr();
}

static void W_test()
{
  int i, x;
  unsigned long d = 0;
  Serial.print("Please enter 8 hexadecimal digits: ");
  for(i = 0; i < 8; i++)
  {
    d <<= 4;
    while(Serial.available() == 0) ;    // This is BLOCKING CODE if the whole packet isn't sent at once.
    x = Serial.read();
    if('0' <= x && x <= '9')
    {
      d += x - '0'; 
    }
    else if('A' <= x && x <= 'F')
    {
      d += x - 'A' + 10;
    }
    else if('a' <= x && x <= 'f')
    {
      d += x - 'a' + 10;
    }
    else
    {
      Serial.print("Assuming 0 for non-hexadecimal digit: ");
      Serial.println(x);
    }
  }
  write_word(d);
  Serial.print("\r\nCore data write: 0x");
  Serial.print(d, HEX);
  Serial.print(" = B");
  Serial.println(d, BIN);
}

static void X_test()
{
  char buf[64];
  int i, x;
  unsigned long d1, d = 0;
  Serial.print("Please enter 8 hexadecimal digits: ");
  for(i = 0; i < 8; i++)
  {
    d <<= 4;
    while(Serial.available() == 0) ;
    x = Serial.read();
    if('0' <= x && x <= '9')
    {
      d += x - '0'; 
    }
    else if('A' <= x && x <= 'F')
    {
      d += x - 'A' + 10;
    }
    else if ('a' <= x && x <= 'f')
    {
      d += x - 'a' + 10;
    }
    else
    {
      Serial.print("Assuming 0 for non-hexadecimal digit: ");
      Serial.println(x);
    }
  }
  d1 = exchg_word(d);
  Serial.print("\r\nCore data write: 0x");
  sprintf(buf, "%08lx; read 0x%08lx", d, d1);
  Serial.println(buf);
}

static void r_test()
{
  int i, x, a = 0;
  Serial.print("Please enter address of bit to read: ");
  for(i = 0; i < ADDRSIZE; i++)
  {
    a <<= 1;
    while(Serial.available() == 0) ;
    x = Serial.read();
    if(x != '0') // Assert x == '1'
    {
      a += 1;
    }
  }
  Serial.print("\r\nCore data read from address: ");
  Serial.print(a, BIN);
  Serial.print(" found: ");
  Serial.println(read_bit(a));
}

static void w_test()
{
  int i, x, a = 0;
  Serial.print("Please enter address of bit to write: ");
  for(i = 0; i < ADDRSIZE; i++)
  {
    a <<= 1;
    while(Serial.available() == 0) ;
    x = Serial.read();
    if(x != '0') // Assert x == '1'
    {
      a += 1;
    }
  }
  Serial.print("\r\nPlease enter bit to write: ");
  while(Serial.available() == 0) ;
  x = Serial.read();
  Serial.print("\r\nCore data write to address: ");
  Serial.print(a, BIN);
  Serial.print(" value: ");
  if(x == '0')
  {
    write_bit(a, 0);
    Serial.println("0");
  }
  else // Assert x == '1'
  {
    write_bit(a, 1);
    Serial.println("1");
  }
}

static void t_test()
{
  int i;
  unsigned long d;
  for(i = 0; i < WORDSIZE; i++)
  {
    write_bit(i, 0);
    d = read_bit(i);
    Serial.print("Address (");
    Serial.print(i >> 3);
    Serial.print(i & 7);
    Serial.print(") ");
    Serial.print(i, BIN);
    Serial.print("   wrote 0 read ");
    Serial.print(d);
    write_bit(i, 1);
    d = read_bit(i);
    Serial.print("   wrote 1 read ");
    Serial.println(d);
  }
}

static void T_test()
{
  unsigned long d;
  int i, j;
  Serial.print("log10(iters)? ");
  while(Serial.available() == 0) ;
  int n = Serial.read() - '0';
  if (n < 0 || n > 9)
      Serial.println("bad power-of-10 arg to T");
  else
  {
    unsigned long n_iters = 1, n_iters_done = 0;
    while (n--) n_iters *= 10;
    errs = 0;
    for (n_iters_done = 0; n_iters_done < n_iters; ++n_iters_done)
    {
      for(i = 0; i < WORDSIZE; i++)
      {
        for(j = 0; j < WORDSIZE; j++)
        {
          if(j == i) { continue; }

          write_bit(i, 0);
          d = read_bit(i);
          if(d != 0) { errs++; maybe_report_error(i, j, "0-0"); }
          d = read_bit(i);
          if(d != 0) { errs++; maybe_report_error(i, j, "0-1"); }

          write_bit(i, 0);
          write_bit(j, 0);
          d = read_bit(i);
          if(d != 0) { errs++; maybe_report_error(i, j, "000"); }
          d = read_bit(i);
          if(d != 0) { errs++; maybe_report_error(i, j, "001"); }

          write_bit(i, 0);
          write_bit(j, 1);
          d = read_bit(i);
          if(d != 0) { errs++; maybe_report_error(i, j, "010"); }
          d = read_bit(i);
          if(d != 0) { errs++; maybe_report_error(i, j, "011"); }

          write_bit(i, 1);
          d = read_bit(i);
          if(d != 1) { errs++; maybe_report_error(i, j, "1-0"); }
          d = read_bit(i);
          if(d != 1) { errs++; maybe_report_error(i, j, "1-1"); }

          write_bit(i, 1);
          write_bit(j, 0);
          d = read_bit(i);
          if(d != 1) { errs++; maybe_report_error(i, j, "100"); }
          d = read_bit(i);
          if(d != 1) { errs++; maybe_report_error(i, j, "101"); }

          write_bit(i, 1);
          write_bit(j, 1);
          d = read_bit(i);
          if(d != 1) { errs++; maybe_report_error(i, j, "110"); }
          d = read_bit(i);
          if(d != 1) { errs++; maybe_report_error(i, j, "111"); }
        }
      }
      if (n_iters_done % 100 == 0)
      {
        Serial.print(n_iters_done, DEC);
        Serial.print(" iters; ");
        Serial.print(errs, DEC);
        Serial.println(" errors");
      }
    }
    Serial.print(n_iters_done, DEC);
    Serial.print(" iters; ");
    Serial.print(errs, DEC);
    Serial.println(" errors");
  }
}

static void U_test()
{  
  unsigned long n_iters_done = 0;
  unsigned long prev_datum = 0UL;
  errs = 0;
  write_word(prev_datum);
  while (1)
  {
    if (Serial.available() > 0)
    {
      (void)Serial.read();
      Serial.println("stopping");
      break;
    }
    unsigned rnd_0 = random(0x10000);
    unsigned rnd_1 = random(0x10000);
    unsigned long rnd = (unsigned long)(rnd_1) << 16 | rnd_0;
    unsigned long read_datum = exchg_word(rnd);
//          sprintf(buf, "rnd %08lx; read %08lx", rnd, read_datum);
//          Serial.println(buf);
    if (read_datum != prev_datum)
      ++errs;
    prev_datum = rnd;
    if (n_iters_done % 20000 == 0)
    {
      char buf[64];
      sprintf(buf, "%lu iters-done; %lu errors", n_iters_done, errs);
      Serial.println(buf);
    }
    ++n_iters_done;
  }
}

#ifdef ANDY_OUTPUT_CONSTANT_DATA_STREAM
void constant_stream_start()
{
  static unsigned long test;
  //test = read_logical_word(); // need to read four bytes and send them instead
/*
  Serial.print(d, HEX);
  Serial.println();
*/
/*
  Serial.print("Abg1234x");
*/
  Serial.print("Abg");
  Serial.write(test);
  Serial.print("234x");
  test++;
  constant_data_stream_mode_enabled = true;
}
void constant_stream_stop()
{
  constant_data_stream_mode_enabled = false;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  SETUP
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup(void) 
{
  DDRD  = B11111100; // Port D bits 0-1 input, bits 2-7 output
  PORTD = (~ENABLE);
  DDRB  = DWR;
  //pinMode(STYLUS_PIN, OUTPUT); // Stock Arduino
  StylusPin.pinMode(OUTPUT); // FastDigitalPin library 
  Serial.begin(19200);
  randomSeed(0xdeadbeef);
  strip.setBrightness(BRIGHTNESS); //  intended to be called once, in setup(), to limit the current/brightness of the LEDs throughout the life of the sketch. It is not intended as an animation effect itself! The operation of this function is “lossy” — it modifies the current pixel data in RAM, not in the show() call — in order to meet NeoPixels’ strict timing requirements. 
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
#ifdef ANDY_STARTUP_MESSAGE
  Serial.println("Welcome! If you're new, try using the commands 'r', 'w', 't' and 'R', 'W', 'T' to get started.");
#endif
#ifdef START_WITH_ALL_BITS_SET
  write_word(0xffffffff);
  write_word(0xffffffff);
  write_word(0xffffffff);
#endif
  pinMode(CLEAR_BUTTON_INPUT_PIN, INPUT);
  digitalWrite(CLEAR_BUTTON_INPUT_PIN, HIGH); // Pull Up High
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void CheckForSerialCommand() {
  char c;
  if(Serial.available() > 0)
  {
    c = Serial.read();
    Serial.write(c);
    switch(c)
    {
#ifdef ANDY_OUTPUT_CONSTANT_DATA_STREAM  // TO DO: change this to work with the correct output code that sends real data instead of test data.
    case 'o':
      constant_stream_start();
      break;
    case 'O':
      constant_stream_stop();
      break;
#endif
    case 'c':
      current_calibration();
      break;
    case '#':
      echo_comment();
      break;
    case 'e':
      corewise_tests();
      break;
    case 'v':
      toggle_tracing();
      break;
    case 'f':
      toggle_error_reporting();
      break;
    case 'm':
      timing_exchange_word();
      break;
    case 'R':
      R_test();
      break;
    case 's':
      s_test();
      break;
    case 'a':
      a_test();
      break;  
    case 'A':
      A_test();
      break;
    case 'W':
      W_test();
      break;
    case 'X':
      X_test();
      break;
    case 'r':
      r_test();
      break;
    case 'w':
      w_test();
      break;
    case 't':
      t_test();
      break;
    case 'T':
      T_test();
      break;
    case 'U':
      U_test();
      break;
    case 'z':
      errs = 0;
      Serial.println("error-count = 0");
      break;
    default:
      Serial.print("Ignoring unknown command: ");
      Serial.print(c, HEX);
      Serial.print(" ");
      Serial.println(c);
    }
  }
}

void ReadCoresUpdateDisplay() {
    // Read 32 bits and write to NeoPixel
  uint32_t light = strip.Color(0, 0,150);
  uint32_t no_light = strip.Color(0, 0, 0);
  uint32_t color = 0; 
  if ((nowTime - NeopixelUpdateLastRunTime) >= NeopixelUpdatePeriod)
  {
    StartReadTime = millis();
    for(uint16_t i=0; i<strip.numPixels(); i++)
    {
      light = strip.Color(0, 0, brightness_position[( (strip.numPixels()-1) -i)]);
      if (read_bit_physical_position(i))
      {
        #ifdef ANDY_INVERT_LED_STATE
        color = no_light; // bit is set by Wffffffff, so turn off the LED because all we can do so far is clear them with a permanent magnet
        #else
        color = light; 
        #endif
      } 
      else
      {
        #ifdef ANDY_INVERT_LED_STATE
        color = light; 
        #else
        color = no_light; // bit is cleared by external magnet (because that's all a magnet does right now is cleear it)
        #endif
      } 
      strip.setPixelColor(( (strip.numPixels()-1) -i), color );
    }
    strip.show();
    
    EndReadTime = millis(); 
    ReadTime = EndReadTime - StartReadTime;  
    NeopixelUpdateLastRunTime = nowTime;
  }
}

void UpdateDisplayFromScreenArray() {
    // Read 32 bits and write to NeoPixel
  uint32_t red_light = strip.Color(150, 0, 0);
  uint32_t blue_light = strip.Color(0,150, 0);
  uint32_t green_light = strip.Color(0, 0,150);
  uint32_t yellow_light = strip.Color(255,215,0); // Dark Orange
  uint32_t no_light = strip.Color(0, 0, 0);
  uint32_t color = 0;
  int SinglePixel = 0;
  int i = 0;
  if ((nowTime - NeopixelUpdateLastRunTime) >= NeopixelUpdatePeriod)
  {
    StartReadTime = millis();
    for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
    {
      for (uint8_t y=0; y<=3; y++)
      {
        i = x + ( y * 8 );
        red_light = strip.Color(brightness_position[( (strip.numPixels()-1) -i)], 0, 0);
        blue_light = strip.Color(0, brightness_position[( (strip.numPixels()-1) -i)], 0);
        green_light = strip.Color(0, 0, brightness_position[( (strip.numPixels()-1) -i)]);
        SinglePixel = screen_memory[y][x];
        if (SinglePixel == 0) { color = no_light; }
        if (SinglePixel > 1) { color = green_light; } 
        if (SinglePixel ==1) { color = yellow_light; } 
        if (SinglePixel == -1) { color = red_light; } 
        if (SinglePixel == -2) { color = blue_light; } 
        strip.setPixelColor(( (strip.numPixels()-1) -i), color );
      }
    }
    strip.show();
    
    EndReadTime = millis(); 
    ReadTime = EndReadTime - StartReadTime;  
    NeopixelUpdateLastRunTime = nowTime;
  }
}

void CheckForCoreStateChange()
{
  // Which bits changed from the last call?
  volatile unsigned long CorePhysicalStateNow;
  volatile unsigned long CorePhysicalStateLast;
  volatile unsigned long LogicalChanged;
  volatile unsigned long PhysicalChanged;
  // bottom lower right of screen 0 bit. Counts right to left across bottom row, then repeats next row up. The bits are placed in physical order.
  const unsigned int logical_to_physical_position[] = { 6, 5, 2, 1, 7, 4, 3, 0, 15, 12, 11, 8, 14, 13, 10, 9, 22, 21, 18, 17, 23, 20, 19, 16, 31, 28, 27, 24, 30, 29, 26, 25 };
  //  {25,26,29,30,24,27,28,31},
  //  {16,19,20,23,17,18,21,22},
  //  { 9,10,13,14, 8,11,12,15},
  //  { 0, 3, 4, 7, 1, 2, 5, 6} // Correct ! <- This corner nearest sense wire solder pads. This corner is physical core 0 in the 32 bit long.
  write_word(0xffffffff); // To detect changes the cores must be written every time to see if something changes.
  if ((nowTime - CoreChangeDetectUpdateLastRunTime) >= CoreChangeDetectUpdatePeriod)
  {
    CorePhysicalStateNow = read_logical_word(); 
      // Test that each bit works
      //                        31    22      16       8       0
      //                        |      |       |       |       |
    //CorePhysicalStateLast = 0b00000000000000000000000000000000;
    //CorePhysicalStateNow  = 0b00000000000000000000000100000001;
      // End test
    if (CorePhysicalStateLast != CorePhysicalStateNow)
    {
      LogicalChanged = CorePhysicalStateLast ^ CorePhysicalStateNow; // Which bits changed? Bitwise XOR determines which bits are different between the numbers
    }
    else
    {
      LogicalChanged = 0;
    }
    CorePhysicalStateLast = CorePhysicalStateNow;
    CoreChangeDetectUpdateLastRunTime = nowTime;
  }
  // Convert logical to physical screen 32 bit long
  PhysicalChanged = 0;
  for(int n = 31; n >= 0; n--)
  {
    PhysicalChanged = PhysicalChanged << 1; // shift left to make room for the next bit
    // isolate logical bit of interest from LogicalChanged
    if (LogicalChanged & (1UL << logical_to_physical_position[n]) ) // is that bit is set
    {
      PhysicalChanged = PhysicalChanged + 1; // move 1 into PhysicalChanged
    }
    else 
    {
      PhysicalChanged = PhysicalChanged + 0; // move 0 into PhysicalChanged
    }
  }
  CorePhysicalStateChanged = PhysicalChanged; // directly update the global variable
  // Convert 32 bit long PhysicalChanged to array stylus_memory[y][x]
  int i = 0; // bit position in the 32 bit word
  for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
  {
    for (uint8_t y=0; y<=3; y++)
    {
      i = ( (7-x) + (3-y) * 8 );
      if ((PhysicalChanged & (1UL<<i)) > 0) { stylus_memory[y][x] = 1; }
      else { stylus_memory[y][x] = 0; }
    }
  }
  // return (PhysicalChanged); // Return the changed logical bit positions as 32 bit unsigned long.
}

void BinaryStrZeroPad(unsigned long Number){
  //ZeroPadding = nth bit, e.g for a 16 bit number nth bit = 15
  signed char i= 31 ; //ZeroPadding;
  while(i>=0)
  {
      if((Number & (1UL<<i)) > 0) Serial.write('1');
      else Serial.write('0');
      --i;
  }
}

void SendSerialPacketUpdate() {
  if ((nowTime - SerialPacketUpdateLastRunTime) >= SerialPacketUpdatePeriod)
  {
    #ifdef ANDY_OUTPUT_CONSTANT_DATA_STREAM
    if (constant_data_stream_mode_enabled)
    {
      constant_stream_start();
      Serial.println();
    }
    #endif
    #ifdef ANDY_TEST_REAL_BYTES
    byte arr[4];
    // read real data word
    #ifdef ANDY_READ_LOGICAL_DATA_WORD
    unsigned long value = read_logical_word(); // full_32_bit_word; // 0xdeadbeaf; // full_32_bit_word0xaaaaaaaa;
    // convert word to four bytes
    arr[3] = value & 0xFF;
    arr[2] = (value >> 8) & 0xFF;
    arr[1] = (value >> 16) & 0xFF;
    arr[0] = (value >> 24) & 0xFF;
    #endif
    #ifdef ANDY_READ_PHYSICAL_DATA_WORD
    // read physical bytes positions
    arr[0] = read_word_physical_row(0);
    arr[1] = read_word_physical_row(1);
    arr[2] = read_word_physical_row(2);
    arr[3] = read_word_physical_row(3);
    #endif
    #ifdef ANDY_INVERT_LED_STATE  
    arr[0] = ~arr[0];
    arr[1] = ~arr[1];
    arr[2] = ~arr[2];
    arr[3] = ~arr[3];
    #endif
    Serial.print(GameState,DEC);
    Serial.print(TopLevelStateMachine,DEC);
    // send packet prefix 3 bytes
    BinaryStrZeroPad(CorePhysicalStateChanged);
    Serial.print("Abg");
    // send four bytes
    Serial.write(arr[0]);
    Serial.write(arr[1]);
    Serial.write(arr[2]);
    Serial.write(arr[3]);
    // Send checksum
    Serial.print("x");  // TO DO: calculate a real checksum
    Serial.print("SL:");
    Serial.print(SnakeLength);
    /*
    Serial.print(" RT:");
    Serial.print(ReadTime); // How many ms it takes to complete a read of each of the 32 bits and send a neopixel update
    Serial.print("ms LT:");
    Serial.print(LoopTime); // How many ms it takes to complete a read of each of the 32 bits and send a neopixel update
    Serial.print("ms");
    */
    // Dump the screen array for debug
    Serial.print("SA:");
    for (uint8_t y=0; y<=3; y++)
    {
      for (uint8_t x=0; x<=7; x++)
      {
        Serial.print(screen_memory[y][x]); // prints from top left, 
      }
    }
    Serial.println();
    SerialPacketUpdateLastRunTime = nowTime;
    #endif
  }
}

void GetPhysicalCoreState() {
  PhysicalCoreStartupPattern[0] = read_word_physical_row(0);
  PhysicalCoreStartupPattern[1] = read_word_physical_row(1);
  PhysicalCoreStartupPattern[2] = read_word_physical_row(2);
  PhysicalCoreStartupPattern[3] = read_word_physical_row(3);
}

void ScrollCoreMemory() {
unsigned int ScreenReadCol = 0;
unsigned int ScreenWriteCol = 0;
unsigned int StringPosition = 0;
unsigned int CharacterColumn = 0;

// Is it time to scroll again?
  if ((nowTime - ScrollUpdateLastRunTime) >= ScrollUpdatePeriod)
  {
    ScrollUpdateLastRunTime = nowTime;
// Shift All Screen Content Left One Column
    for (uint8_t x=6; x=0; x--)
    {
      // Read Screen Column x
      // Write Screen Column x+1
    }
// Move in new character column by column
  // Read Character Column
  // Write Screen Column
  }
// Out of characters?

}

unsigned long Button1State(unsigned long clear_duration) // send a 1 or more to clear, 0 to use normally)
{
  volatile unsigned long lasttime;
  volatile unsigned long thistime;
  volatile unsigned long duration;
  thistime = millis();
  //if(clear_duration>0) { duration = clear_duration; }
  if(digitalRead(CLEAR_BUTTON_INPUT_PIN)!=true)  { duration = duration + (thistime - lasttime) ; }
  else { duration = 0; }
  lasttime = thistime;
  return duration;
}

char get_gesture(void) // TO DO Work on Gesture
{
  char gesture = 0;
  switch(GestureDetectState)
  {
  case 0:
    GestureDetectState = CorePhysicalStateChanged;
    gesture = 0;
    break;
  case 1:
    GestureDetectState = CorePhysicalStateChanged;
    break;
  case 2:
    GestureDetectState = CorePhysicalStateChanged;
    break;
  case 4:
    GestureDetectState = CorePhysicalStateChanged;
    break;
  case 8:
    GestureDetectState = CorePhysicalStateChanged;
    break;
  case 16:
    GestureDetectState = CorePhysicalStateChanged;
    break;
  case 32:
    GestureDetectState = CorePhysicalStateChanged;
    break;
  case 64:
    GestureDetectState = CorePhysicalStateChanged;
    break;
  case 128:
    gesture = "E";
    GestureDetectState = 0;
    break;
  default:
    break;
  }
  return gesture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  SNAKE GAME FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RandomStartMap()
{
  for (uint8_t x=0; x<=7; x++)
  {
    for (uint8_t y=0; y<=3; y++)
    {
      int RandomPixel = random(0, 6); // wider range than need to get some extra blank space
      if (RandomPixel == 1) { RandomPixel = RandomPixel * (-1); }
      if (RandomPixel == 2) { RandomPixel = RandomPixel * (-1); }
      if (RandomPixel > 2) { RandomPixel = 0; }
      screen_memory[y][x] = RandomPixel;
    }
  }
  SnakeHeadX = random(0, 7);
  SnakeHeadY = random(0, 3);
  screen_memory[SnakeHeadY][SnakeHeadX] = 1; // The snake starts here!
}

void IncreaseSnakeLength() 
{
  SnakeLength++;
}

void DecreaseSnakeLength() 
{
  SnakeLength = SnakeLength - 2;
  if (SnakeLength < 1) { GameOver = true; }
}

void RemoveSnakeTail() 
{
  // scan for positive numbers greater than SnakeLength and remove them from screen memory locations
  for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
  {
    for (uint8_t y=0; y<=3; y++)
    {
      if (screen_memory[y][x] > SnakeLength ) { screen_memory[y][x] = 0; } // 
    }
  }  
}

void AreYouAWinner() 
{
  // Assume a win
  Winner = true;
  // scan for -2 numbers, if any are found left, you're not a winner yet.
  for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
  {
    for (uint8_t y=0; y<=3; y++)
    {
      if (screen_memory[y][x] == ((signed int)-2) ) { Winner = false; }
    }
  }  
}

void SnakeGameLogic()
{
  // Y Keep track of previous state in the screen memory (already in screen memory array)
  // Y Keep track of updated state affected by stylus (already in CorePhysicalStateChanged when this routine runs)
  // Y Keep track of the head in X and Y position to make it easier to detect a stylus move
  // Y Translate the core change from 32 bit long to array 
  // Y Test, scan stylus memory, and if there is a stylus, update screen memory
    //  for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
    //  {
    //    for (uint8_t y=0; y<=3; y++)
    //    {
    //      if (stylus_memory[y][x] == 1) { screen_memory[y][x] = 1; }
    //    }
    //  }

  // Look for movement near the snake head since that is all that is valid. Check the four cardinal directions, as long as they are one screen.
  // Test right for movement and a blank pixel
    OldSnakeLength = SnakeLength;
    MovementDetected = false;
    TestX = SnakeHeadX + 1;
    TestY = SnakeHeadY;
    if ((TestX < 8) && (TestX >= 0) && (MovementDetected == false))
    {
      if (stylus_memory[TestY][TestX] == 1)
      {
        if (screen_memory[TestY][TestX] == 0) // blank pixel found so move in
        { 
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
        if (screen_memory[TestY][TestX] == 1) // crashed into snake means you die!
        {
          //GameOver = true;
        }
        if (screen_memory[TestY][TestX] == -2) // food grows snake 1, and moves in
        {
          IncreaseSnakeLength();
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
        if (screen_memory[TestY][TestX] == -1) // poison shortens snake 2, and moves in
        {
          DecreaseSnakeLength();
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
      }
    }
 
  // Test down for movement and a blank pixel
    TestX = SnakeHeadX;
    TestY = SnakeHeadY + 1;
    if ((TestY < 4) && (TestY >= 0) && (MovementDetected == false))
    { 
      if (stylus_memory[TestY][TestX] == 1)
      {
        if (screen_memory[TestY][TestX] == 0) // blank pixel found so move in
        { 
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
        if (screen_memory[TestY][TestX] == 1) // crashed into snake means you die!
        {
          //GameOver = true;
        }
        if (screen_memory[TestY][TestX] == -2) // food grows snake 1, and moves in
        {
          IncreaseSnakeLength();
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
        if (screen_memory[TestY][TestX] == -1) // poison shortens snake 2, and moves in
        {
          DecreaseSnakeLength();
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
      }
    }

  // Test left for movement and a blank pixel
    TestX = SnakeHeadX - 1;
    TestY = SnakeHeadY;
    if ((TestX < 8) && (TestX >= 0) && (MovementDetected == false))
    { 
      if (stylus_memory[TestY][TestX] == 1)
      {
        if (screen_memory[TestY][TestX] == 0) // blank pixel found so move in
        { 
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
        if (screen_memory[TestY][TestX] == 1) // crashed into snake means you die!
        {
          //GameOver = true;
        }
        if (screen_memory[TestY][TestX] == -2) // food grows snake 1, and moves in
        {
          IncreaseSnakeLength();
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
        if (screen_memory[TestY][TestX] == -1) // poison shortens snake 2, and moves in
        {
          DecreaseSnakeLength();
          SnakeHeadX = TestX;
          MovementDetected = true;
        }
      }
    }
 
  // Test UP for movement and a blank pixel
    TestX = SnakeHeadX;
    TestY = SnakeHeadY - 1;
    if ((TestY < 4) && (TestY >= 0) && (MovementDetected == false))
    { 
      if (stylus_memory[TestY][TestX] == 1)
      {
        if (screen_memory[TestY][TestX] == 0) // blank pixel found so move in
        { 
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
        if (screen_memory[TestY][TestX] == 1) // crashed into snake means you die!
        {
          //GameOver = true;
        }
        if (screen_memory[TestY][TestX] == -2) // food grows snake 1, and moves in
        {
          IncreaseSnakeLength();
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
        if (screen_memory[TestY][TestX] == -1) // poison shortens snake 2, and moves in
        {
          DecreaseSnakeLength();
          SnakeHeadY = TestY;
          MovementDetected = true;
        }
      }
    }
    
    if (MovementDetected == true)
    {
      if (SnakeLength > OldSnakeLength) // snake got longer, no need to remove anything.
      {
          // scan for positive numbers up to SnakeLength and add one to those screen memory locations
          for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<=3; y++)
            {
              if (screen_memory[y][x] > (signed int)0) { screen_memory[y][x]=screen_memory[y][x]+(signed int)1; }
            }
          }  
          // Nothing to erase
      }
      else if (SnakeLength < OldSnakeLength) // snake got shorter, remove any above snake length
      {
          for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<=3; y++)
            {
              if (screen_memory[y][x] >= (signed int)SnakeLength) { screen_memory[y][x]=0; }
            }
          }
          // scan for positive numbers up to SnakeLength and add one to those screen memory locations
          for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<=3; y++)
            {
              if (screen_memory[y][x] > 0) { screen_memory[y][x]=screen_memory[y][x]+(signed int)1; }
            }
          }
          //RemoveSnakeTail();
      }
      else if (SnakeLength == OldSnakeLength) // no change, just remove the old end, then increment existing body
      {
          for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<=3; y++)
            {
              if (screen_memory[y][x] >= (signed int)SnakeLength) { screen_memory[y][x]=0; }
            }
          }
          // scan for positive numbers up to SnakeLength and add one to those screen memory locations
          for (uint8_t x=0; x<=7; x++)      // The ordering of this update takes an array that is illustrated in the source code in the way it is viewed on screen.
          {
            for (uint8_t y=0; y<=3; y++)
            {
              if (screen_memory[y][x] > 0) { screen_memory[y][x]=screen_memory[y][x]+(signed int)1; }
            }
          }
      }
      screen_memory[SnakeHeadY][SnakeHeadX] = 1; // new snake head position
      OldSnakeLength = SnakeLength;
    }
    AreYouAWinner();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  SNAKE GAME STATE MACHINE LOGIC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UpdateSnakeGame()
{
  if ((nowTime - SnakeGameUpdateLastRunTime) >= SnakeGameUpdatePeriod)
  {
    switch(GameState)
    {
    case 0: // Startup screen
      // Use the default values in the screen memory array as the starting point for the game.
      Winner = false;
      GameOver = false;
      SnakeLength = 1; 
      RandomStartMap(); // internally clears upper left so snake can start there
      GameState = 1;
      break;
    case 1: // Setup a new game
      GameState = 2;
      break;
    case 2: // Play
      SnakeGameLogic();
      // Test : if upper right core is touched, game over!
      // if ( CorePhysicalStateChanged == 0b10000000000000000000000000000000 ) { GameOver = true ;}
      if (GameOver) { GameOverTime = nowTime; GameState = 3; }
      if (Winner)  { GameOverTime = nowTime; GameState = 4; }
      break;
    case 3: // Game Over = Red Screen
      for (uint8_t x=0; x<=7; x++)
      {
        for (uint8_t y=0; y<=3; y++)
        {
          screen_memory[y][x] = -1;
        }
      }
      if ((nowTime - GameOverTime) > GameOverTimeAutoReset)
      {
        GameState = 0;
      }
      break;
    case 4: // Winner = Green Screen
      for (uint8_t x=0; x<=7; x++)
      {
        for (uint8_t y=0; y<=3; y++)
        {
          screen_memory[y][x] = -2;
        }
      }
      if ((nowTime - GameOverTime) > GameOverTimeAutoReset)
      {
        GameState = 0;
      }
      break;
    default:
      break;
    }
    SnakeGameUpdateLastRunTime = nowTime;
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  MAIN LOOP
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop(void)
{
  StartLoopTime = millis();
  nowTime = millis();
  ButtonPressDuration_ms = Button1State(0);

  switch(TopLevelStateMachine)
  {
    case 0:      // Startup
      GetPhysicalCoreState();
      ReadCoresUpdateDisplay();
      CheckForSerialCommand();
      SendSerialPacketUpdate();
      // ScrollCoreMemory();
      TopLevelStateMachine = 1;
      break;
    case 1:      // Default starting mode for simple drawing mode
      GameState = 0;
      ReadCoresUpdateDisplay();
      CheckForSerialCommand();
      SendSerialPacketUpdate();
   // if(get_gesture()=="E") { write_word(0xffffffff); }                                              // SWIPE LEFT ALONG BOTTOM ROW TO CLEAR SCREEN (does not work in drawing move because the cores aren't detected as changing)
      if(ButtonPressDuration_ms > 50) { write_word(0xffffffff); }                                     // short press to clear screen
      if (read_logical_word()==0xD9F7FBF7) { TopLevelStateMachine = 2; }                              // WRITE T FAR LEFT TO TEST MAGNETIC FLUX RANGE
      if (read_logical_word()==0xD9FEDDE6) { TopLevelStateMachine = 3; }                              // WRITE G FAR LEFT TO PLAY A GAME
      break;
    case 2:      // Mode for testing where the stylus is with LED(s) indication, and 0 zero in the logical 32 bit long as to what cores are blocked, persistant.
      GameState = 0;
      //CorePhysicalStateChanged = CheckForCoreStateChange(); // ***************** When this line is uncommented, case 3 stops working ?!??!?! two can't exist?
      write_word(0xffffffff);
      CorePhysicalStateChanged = read_logical_word();
      ReadCoresUpdateDisplay();
      CheckForSerialCommand();
      SendSerialPacketUpdate();
      if (read_logical_word()==0x7F9F6FBF) { write_word(0xffffffff); TopLevelStateMachine = 1; }      // WRITE BACK ARROW FAR RIGHT TO GO BACK TO DEFAULT MODE
      break;
    case 3:      // Setup a game of snake
      // CorePhysicalStateChanged = CheckForCoreStateChange();   // Look for stylus touch [32 bit long physical screen position]. Moved into game logic because when two instances are active it doesn't work for some reason. Memory?
      CheckForCoreStateChange();
      UpdateSnakeGame();                                      // Video memory array is updated based on game logic [4x8 array of signed 8 bit integers]
      UpdateDisplayFromScreenArray();                         // Video display is updated based on memory array 4x8, 0,0 is lower right, integer values are assigned colors.
      // if(get_gesture()=="E") { GameState = 0 ; }                                              // SWIPE LEFT ALONG BOTTOM ROW TO START GAME OVER (doesn't work yet)
      #ifdef DEBUG_SNAKE_IN_TOP_LEVEL_STATE_MACHINE
      //ReadCoresUpdateDisplay(); // will need to remove this because it is the test mode core write and blue LEDs.
      CheckForSerialCommand();
      SendSerialPacketUpdate();
      if(get_gesture()=="E") { TopLevelStateMachine = 0; }                                            // SWIPE LEFT ALONG BOTTOM ROW TO EXIT GAME
      #endif
      // WRITE BACK ARROW FAR RIGHT TO GO BACK TO DEFAULT MODE
      if (read_logical_word()==0x7F9F6FBF) { write_word(0xffffffff); GameState = 0; TopLevelStateMachine = 1; }
      if(ButtonPressDuration_ms > 50) { GameState = 0; }                                     // short press to restart game
      break;
    default:
      break;
  }

  EndLoopTime = millis(); 
  LoopTime = EndLoopTime - StartLoopTime;  
}
