// Character Map

// A full alphabet table is 26 x 4 x 4 bits = 54 bytes (if stored maximum efficiently)
// Array arranged directly as " CORE MEMORY!" = 13 elements, numbered 0 to 12
// Need to use program memory for constants because over 77% usage in UNO causes instability.
// https://forum.arduino.cc/index.php?topic=45681.0
const static bool character_font[14][4][4] PROGMEM = {   // Character, Row, Column
{
  {0,0,0,0}, //  
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0}
},
{
  {0,1,1,1}, // C
  {0,1,0,0},
  {0,1,0,0},
  {0,1,1,1}
},
{
  {0,1,1,1}, // O
  {0,1,0,1},
  {0,1,0,1},
  {0,1,1,1}
},
{
  {0,1,1,1}, // R
  {0,1,0,1},
  {0,1,1,0},
  {0,1,0,1}
},
{
  {0,1,1,1}, // E
  {0,1,0,0},
  {0,1,1,0},
  {0,1,1,1}
},
{
  {0,0,0,0}, //  
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0}
},
{
  {0,1,0,1}, // M
  {0,1,1,1},
  {0,1,0,1},
  {0,1,0,1}
},
{
  {0,1,1,1}, // E
  {0,1,0,0},
  {0,1,1,0},
  {0,1,1,1}
},
{
  {0,1,0,1}, // M
  {0,1,1,1},
  {0,1,0,1},
  {0,1,0,1}
},
{
  {0,1,1,1}, // O
  {0,1,0,1},
  {0,1,0,1},
  {0,1,1,1}
},
{
  {0,1,1,1}, // R
  {0,1,0,1},
  {0,1,1,0},
  {0,1,0,1}
},
{
  {0,1,0,1}, // Y
  {0,1,0,1},
  {0,0,1,0},
  {0,0,1,0}
},
{
  {0,1,0,0}, // !
  {0,1,0,0},
  {0,0,0,0},
  {0,1,0,0}
}
};

/*

const bool character_C[4][4] = {
  {0,1,1,1},
  {0,1,0,0},
  {0,1,0,0},
  {0,1,1,1}
};
const bool character_O[4][4] = {
  {0,1,1,1},
  {0,1,0,1},
  {0,1,0,1},
  {0,1,1,1}
};
const bool character_R[4][4] = {
  {0,1,1,1},
  {0,1,0,1},
  {0,1,1,0},
  {0,1,0,1}
};
const bool character_E[4][4] = {
  {0,1,1,1},
  {0,1,0,0},
  {0,1,1,0},
  {0,1,1,1}
};
const bool character_space[4][4] = {
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0},
  {0,0,0,0}
};

T
 0 0 0 0
 0 1 0 1
 0 1 0 1
 0 1 0 1

G
 0 0 0 0
 0 0 1 1
 0 0 1 0
 0 0 0 0

BACK ARROW
 1 1 1 1 1 1 1 0
 1 1 1 1 1 1 0 0
 1 1 1 1 1 1 0 0
 1 1 1 1 1 1 1 0


 */
