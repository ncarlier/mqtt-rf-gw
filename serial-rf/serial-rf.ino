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
#define DEBUG 1

#include "SoftwareSerial.h"
#include <RCSwitch.h>

// PINS configuration
const int RF_DATA_PIN = 10;

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

  blinkLed(200, 2);
  #if DEBUG > 0
      debug("ready!");
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
  } else {
    #if DEBUG > 0
      Serial.println("Unable to decode command: " + strCmd);
    #endif
    blinkLed(200, 3);
  }
}

