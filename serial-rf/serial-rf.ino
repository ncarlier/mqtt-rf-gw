/**
  Simple serial RF Gateway.

  Copyright (c) 2018 Nicolas CARLIER (https://github.com/ncarlier)
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
  Somfy RTS protocol code belong to Nickduino (https://github.com/Nickduino).
  This code is under Creative Commons CC-BY-NC-SA
  If you want to learn more about the Somfy RTS protocol, check out https://pushstack.wordpress.com/somfy-rts-protocol/
  The rolling code will be stored in EEPROM, so that you can power the Arduino off.
    
   Easiest way to make it work for you:
    - Choose a remote number
    - Choose a starting point for the rolling code. Any unsigned int works, 1 is a good start
    - Upload the sketch
    - Long-press the program button of YOUR ACTUAL REMOTE until your blind goes up and down slightly
    - send 'prog' to the serial terminal
  To make a group command, just repeat the last two steps with another blind (one by one)
  
  Then:
    - "up" will make it to go up
    - "stop" make it stop
    - "down" will make it to go down
*/

#define DEBUG 1

#include "SoftwareSerial.h"
#include <RCSwitch.h>
#include <EEPROM.h>

// PINS configuration
#define RF_DATA_PIN 5

// Somfy RTS protocol parameters
#define SYMBOL 640
#define UP 0x2
#define STOP 0x1
#define DOWN 0x4
#define PROG 0x8
#define EEPROM_ADDRESS 0
#define REMOTE 0x279696  // REMOTE NUMBER <TO CHANGE>

// Somfy dynamic parameters
unsigned int initialRollingCode = 28; // INITIAL ROLLING CODE <TO CHANGE>
unsigned int rollingCode = 0;
byte frame[7];
byte checksum;

// Create RC switch object
RCSwitch mySwitch = RCSwitch();

// Input command
String inputCmd = "";

// Whether the command is complete
boolean cmdComplete = false;


#if DEBUG > 0
void debug(char *fmt, ... ){
  char tmp[128]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(tmp, 128, fmt, args);
  va_end (args);
  Serial.println(tmp);
}
#endif

void setup() {
  // initialize the LED_BUILTIN pin as an output
  pinMode(LED_BUILTIN, OUTPUT);

  // initialize serial
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // init RF transmitter
  mySwitch.enableTransmit(RF_DATA_PIN);

  // Optional set pulse length.
  // mySwitch.setPulseLength(320);

  // reserve 200 bytes for the inputCmd:
  inputCmd.reserve(200);

  // Init EEPROM
  if (EEPROM.get(EEPROM_ADDRESS, rollingCode) < initialRollingCode) {
    rollingCode = initialRollingCode;
    EEPROM.put(EEPROM_ADDRESS, rollingCode);
  }

  blinkLed(200, 2);
  #if DEBUG > 0
    debug("Virtual remote %4X ready with rolling code = %d", REMOTE, rollingCode);
  #endif
}

void loop() {
  if (cmdComplete) {
    processCmd(inputCmd);
    // clear the cmd:
    inputCmd = "";
    cmdComplete = false;
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputCmd += inChar;
    if (inChar == '\n') {
      cmdComplete = true;
    }
  }
}

void blinkLed(int ms, int nb) {
  if (nb < 0) {
    for(;;) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(ms);
      digitalWrite(LED_BUILTIN, LOW);
      delay(ms);
    }
  } else {
    for (int i = 0 ; i < nb ; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(ms);
      digitalWrite(LED_BUILTIN, LOW);
      delay(ms);
    }
  }
}

void processCmd(String strCmd) {
  if (strCmd.length() == 14) {
    // decoding command
    // ex: 1;11111;01000
    char cmd[14];
    strCmd.toCharArray(cmd, 14);
    cmd[1] = '\0';
    cmd[7] = '\0';
    cmd[13] = '\0';
    digitalWrite(LED_BUILTIN, HIGH);
    char* addr1 = &cmd[2];
    char* addr2 = &cmd[8];
    if ((char)cmd[0] == '1') {
      #if DEBUG > 0
        debug("Switching ON the outlet #%s|%s ...", addr1, addr2);
      #endif
      mySwitch.switchOn(addr1, addr2);
    } else {
      #if DEBUG > 0
        debug("Switching OFF the outlet #%s|%s ...", addr1, addr2);
      #endif
      mySwitch.switchOff(addr1, addr2);
    }
    digitalWrite(LED_BUILTIN, LOW);
  } else if (strCmd == "up\n") {
    #if DEBUG > 0
      debug("Opening the blind...");
    #endif
    processRTSCommand(UP);
  } else if (strCmd == "down\n") {
    #if DEBUG > 0
      debug("Closing the blind...");
    #endif
    processRTSCommand(DOWN);
  } else if (strCmd == "stop\n") {
    #if DEBUG > 0
      debug("Stopping the blind...");
    #endif
    processRTSCommand(STOP);
  } else if (strCmd == "prog\n") {
    #if DEBUG > 0
      debug("Setting the blind in prog mode...");
    #endif
    processRTSCommand(PROG);
  } else {
    #if DEBUG > 0
      Serial.println("Unable to decode command: " + strCmd);
    #endif
    blinkLed(200, 3);
  }
}

void processRTSCommand(byte button) {
  buildRTSFrame(button);
  sendRTSFrame(2);
  for(int i = 0; i<2; i++) {
    sendRTSFrame(7);
  }
}

void buildRTSFrame(byte button) {
  unsigned int code;
  EEPROM.get(EEPROM_ADDRESS, code);
  frame[0] = 0xA7;         // Encryption key. Doesn't matter much
  frame[1] = button << 4;  // Which button did  you press? The 4 LSB will be the checksum
  frame[2] = code >> 8;    // Rolling code (big endian)
  frame[3] = code;         // Rolling code
  frame[4] = REMOTE >> 16; // Remote address
  frame[5] = REMOTE >>  8; // Remote address
  frame[6] = REMOTE;       // Remote address

  #if DEBUG > 0
    Serial.print("Frame: ");
    for(byte i = 0; i < 7; i++) {
      //  Displays leading zero in case the most significant nibble is a 0.
      if(frame[i] >> 4 == 0) { 
        Serial.print("0");
      }
      Serial.print(frame[i],HEX); Serial.print(" ");
    }
    Serial.println("");
  #endif
  
  // Checksum calculation: a XOR of all the nibbles
  checksum = 0;
  for(byte i = 0; i < 7; i++) {
    checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
  }
  checksum &= 0b1111; // We keep the last 4 bits only


  // Checksum integration
  // If a XOR of all the nibbles is equal to 0, the blinds will consider the checksum ok.
  frame[1] |= checksum;
  

  #if DEBUG > 0
    Serial.print("With checksum: ");
    for(byte i = 0; i < 7; i++) {
      if(frame[i] >> 4 == 0) {
        Serial.print("0");
      }
      Serial.print(frame[i],HEX); Serial.print(" ");
    }
    Serial.println("");
  #endif

  // Obfuscation: a XOR of all the bytes
  for(byte i = 1; i < 7; i++) {
    frame[i] ^= frame[i-1];
  }

  #if DEBUG > 0
    Serial.print("Obfuscated: ");
    for(byte i = 0; i < 7; i++) {
      if(frame[i] >> 4 == 0) {
        Serial.print("0");
      }
      Serial.print(frame[i],HEX); Serial.print(" ");
    }
    Serial.println("");
    debug("Rolling Code: %d", code);
  #endif

  // We store the value of the rolling code in the EEPROM.
  // It should take up to 2 adresses but the Arduino function takes care of it.
  EEPROM.put(EEPROM_ADDRESS, code + 1);
}

void sendRTSFrame(byte sync) {
  if (sync == 2) {
    // Only with the first frame.
    // Wake-up pulse & Silence
    PORTD |= 1<<RF_DATA_PIN;
    delayMicroseconds(9415);
    PORTD &= !(1<<RF_DATA_PIN);
    delayMicroseconds(89565);
  }

  // Hardware sync: two sync for the first frame, seven for the following ones.
  for (int i = 0; i < sync; i++) {
    PORTD |= 1<<RF_DATA_PIN;
    delayMicroseconds(4*SYMBOL);
    PORTD &= !(1<<RF_DATA_PIN);
    delayMicroseconds(4*SYMBOL);
  }

  // Software sync
  PORTD |= 1<<RF_DATA_PIN;
  delayMicroseconds(4550);
  PORTD &= !(1<<RF_DATA_PIN);
  delayMicroseconds(SYMBOL);
  
  
  //Data: bits are sent one by one, starting with the MSB.
  for(byte i = 0; i < 56; i++) {
    if(((frame[i/8] >> (7 - (i%8))) & 1) == 1) {
      PORTD &= !(1<<RF_DATA_PIN);
      delayMicroseconds(SYMBOL);
      PORTD ^= 1<<5;
      delayMicroseconds(SYMBOL);
    }
    else {
      PORTD |= (1<<RF_DATA_PIN);
      delayMicroseconds(SYMBOL);
      PORTD ^= 1<<5;
      delayMicroseconds(SYMBOL);
    }
  }
  
  PORTD &= !(1<<RF_DATA_PIN);
  delayMicroseconds(30415); // Inter-frame silence
}

