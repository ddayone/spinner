// Birthday Spinner for Isabelle
// Made by Lennert for my little girl
// All code can be used, reused, changed, mangled, etc. 

#include <PinChangeInterrupt.h> // mimics attachInterrupt() but a PCI for ATtiny
#include <avr/sleep.h>
#include "font.h"

// clear and set interup routines
#ifndef cbi
  #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
  #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

const byte interruptPin = 10; // hall sensor connected to this pin
byte buttonPin = 9; // pushbutton connected to this pin

const int charHeight = 7;
const int charWidth = 5;
int rows=7;

char charArray[11];                       // holds characters to display
unsigned long lastTimeUs;                // time in us since magnet was sensed
unsigned long spinTimeUs;                // time in us since magnet was sensed
bool spinning = true;                    // if false, sleep timer will start to track
unsigned long startTimeUs;               // time in us since current spinning cycle started
unsigned long revTimeUs = 0;                 // time (us) of last full rotation
unsigned long dwellTimeUs = 20;               // time (us) between LED changes (based on rotation speed)
unsigned long dwellTimeUsNew = 20;               // time (us) between LED changes (based on rotation speed)
unsigned long dwellTimeSum = 20;
volatile unsigned long revolutions = 0;  // track number of revolutions since last start

unsigned long curTimeUs;

volatile boolean rotationFlag = false;  // modified by ISR

void setup() {
  pinMode(interruptPin, INPUT_PULLUP); // set hall as input with internal pull-up
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(interruptPin),MAG_ISR,FALLING);  // Catch hall sensor on falling interrupt
  //(FALLING, not CHANGE --> with change two interrupts are generated)
  
  pinMode(buttonPin, INPUT_PULLUP); // set button as input with internal pull-up
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(buttonPin),BUT_ISR,FALLING);  // Catch button press on falling interrupt
  
  DDRA = 0b11111111;    // All pins in PORTA are outputs *LEDS are connected to PORTA i.e. PA0,1,2,3,4,5,6
  PORTA = 0;            // Turn off the leds

  lastTimeUs = micros(); // set time in us to current time
  curTimeUs = lastTimeUs;
  
  
  strcpy (charArray, " ISABELLE  ");
}

void loop() {
  //strcpy (charArray, text);
  if ((micros() - spinTimeUs) > 1000000UL){  //  sleep after 1 seconds
    blinkLEDs();
    system_sleep();
    revolutions = 0;
    lastTimeUs = micros();
  }
  if ((micros() - lastTimeUs) > 2000000UL){ // less than 1 rev / sec
    if (spinning){
      spinning = false;
    }
  }
  if (rotationFlag){ // we are spinning!
    rotationFlag = false;
    if (!spinning){
      spinning = true;
      startTimeUs = micros(); 
    }
    spinTimeUs = micros();
/*  //not doing the calculations here as they are unreliable.   
    lastTimeUs = curTimeUs;
    curTimeUs = micros(); //set revTime in microsec to current time so we can calculate how long 1 revolution t
    revTimeUs = curTimeUs - lastTimeUs;
    dwellTimeUs = ((0.3 * dwellTimeUsNew) + ((1 - 0.3) * dwellTimeUs)); // smoothing the reading to prevent jumping of text
    dwellTimeSum = ((0.3 * dwellTimeUs) + ((1 - 0.3) * dwellTimeSum)); // smoothing the reading to prevent jumping of text
    dwellTimeUs = (dwellTimeUs + + ( 2 * dwellTimeSum)) /3 ;
    dwellTimeUsNew = revTimeUs * 3UL / 360UL; // 3 degrees per column (approx...:)  
*/    
    if ( revolutions < 30 ) {
      strcpy (charArray, "-----------");
    }
    if ( revolutions > 45 ){
      strcpy (charArray, " ISABELLE  ");
    }  
    if ( revolutions > 60 ){
      strcpy (charArray, " 12 JAAR   ");
    }
    if ( revolutions > 75 ){
      strcpy (charArray, "TO THE MOON");
    }        
    if ( revolutions > 90 ){ 
      strcpy (charArray, " AND BACK!!");
      revolutions = 30;
    }
    writeText();
  }
}

void MAG_ISR(void) { // Interrups being called on passing of magnet and updating numbers every interrupt
  lastTimeUs = curTimeUs; // setting old-time
  curTimeUs = micros(); //set revTime in microsec to current time so we can calculate how long 1 revolution takes
  rotationFlag = true; // Increment volatile variables
  revolutions += 1;//increment revolutions to change display
  revTimeUs = curTimeUs - lastTimeUs; // calculating rotation-time
  dwellTimeUs = ((0.3 * dwellTimeUsNew) + ((1 - 0.3) * dwellTimeUs)); // smoothing the new reading
  dwellTimeSum = ((0.3 * dwellTimeUs) + ((1 - 0.3) * dwellTimeSum)); // smoothing the previous reading 
  dwellTimeUs = (dwellTimeUs + ( 2 * dwellTimeSum)) /3; // Averaging the waittime between new and old (smoothed readings) to prevent letters from jumping around
  dwellTimeUsNew = revTimeUs * 3UL / 360UL; // 3 degrees per column (approx...:) - being smoothed even more by the calculations of dwellTimeUs before...
}

void writeText(){
  for(int k=0; k< sizeof(charArray); k++){
    char c = charArray[k];
    if(c){
      printLetter(c);
    }
  }
}

void printLetter(char ch){
// https://github.com/reger-men/Arduion-POV-clock/blob/master/clock.ino
  // make sure the character is within the alphabet bounds (defined by the font.h file)
  // if it's not, make it a blank character
  if (ch < 32 || ch > 126){
    ch = 32;
    }
  // subtract the space character (converts the ASCII number to the font index number)
  ch -= 32;
  // step through each byte of the character array
  for (int i=0; i<charWidth; i++) {
    char b = pgm_read_byte_near(&(font[ch][i]));
    PORTA = b;
    delayMicroseconds(dwellTimeUs);
  }
  
  //clear the LEDs
  PORTA = 0;
  delayMicroseconds(dwellTimeUs);
}

void blinkLEDs() {
    PORTA = 0x7f;
    delay(20);
    PORTA = 0;
    delay(20);
    PORTA = 0x7f;
    delay(20);
    PORTA = 0;
}

bool touched(){
  // returns true if touched, false if not.  Light LED until touch released
  bool touchVal = digitalRead(buttonPin);
  if (!touchVal){
    while(!digitalRead(buttonPin)){ // wait till touch release
      PORTA = 0x7f;
    }
    PORTA = 0;
    return (true);
  }
  else{
    return (false);
  }
}

void BUT_ISR() {
  //if (touched()){
    strcpy (charArray, "KING LENNERT ");  
    PORTA = 0x40;
    delay(1000);
  //}
  //delay(20);
  PORTA = 0;//digitalWrite(LEDS[0], LOW);
}

// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}
