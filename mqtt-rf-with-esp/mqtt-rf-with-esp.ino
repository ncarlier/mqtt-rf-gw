/**
  MQTT RF Gateway using ESP8266 as network interface.

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

#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <WiFiEspUdp.h>
#include "SoftwareSerial.h"
#include <PubSubClient.h>
#include <RCSwitch.h>

// PINS configuration
const int ESP_RX_PIN = 2;
const int ESP_TX_PIN = 3;
const int RF_DATA_PIN = 10;

// WiFi configuration
//char ssid[] = "MyWiFiAP";  // YOUR WiFi SSID
//char pass[] = "########";  // YOUR WiFi password

// MQTT configuration
IPAddress mqtt_host(10, 0, 0, 5); // YOUR MQTT host IP
const int mqtt_port = 1883;       // YOUR MQTT host port

// MQTT topics
const char COMMAND_TOPIC[] = "rf/command";
const char COMMAND_RESULT_TOPIC[] = "rf/command/result";
const char STATUS_TOPIC[] = "rf/status";

// Initial WiFi status
int status = WL_IDLE_STATUS;

// Initialize the Ethernet client object
WiFiEspClient espClient;

// Initialize PubSub client
PubSubClient client(espClient);

// Initialize software serial connection with ESP module
SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN); // RX, TX

// Create RC switch object
RCSwitch mySwitch = RCSwitch();

// Variables used to alive loop
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

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

  #if DEBUG > 0
    // initialize serial for debugging
    Serial.begin(9600);
    debug("Init...");
  #endif

  // initialize serial for ESP module
  espSerial.begin(9600);

  // initialize ESP module
  WiFi.init(&espSerial);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    #if DEBUG > 0
      debug("Fatal error: WiFi ESP module not detected");
    #endif
    // fatal error: LED blinking forever
    blinkLed(1000, -1);
  }

  // attempt to connect to WiFi network
  Serial.println();
  while ( status != WL_CONNECTED) {
    #if DEBUG > 0
      debug("Attempting to connect to WPA SSID: %s", ssid);
    #endif
    status = WiFi.begin(ssid, pass);
  }

  randomSeed(micros());

  #if DEBUG > 0
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
  #endif

  // connect to MQTT host
  client.setServer(mqtt_host, mqtt_port);
  client.setCallback(callback);

  // init RF transmitter
  mySwitch.enableTransmit(RF_DATA_PIN);

  // Optional set pulse length.
  // mySwitch.setPulseLength(320);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // send alive status every 10 seconds
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "alive #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(STATUS_TOPIC, msg);
    client.loop();
  }

  // mitigate loop velocity
  delay(200);
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

void reconnect() {
  // loop until we're reconnected
  while (!client.connected()) {
    #if DEBUG > 0
      Serial.print("Attempting to connect to MQTT broker: ");
      Serial.println(mqtt_host);
    #endif
    // create a random client ID
    String clientId = "MQTT-RF-GW-";
    clientId += String(random(0xffff), HEX);
    // attempt to connect
    if (client.connect(clientId.c_str())) {
      #if DEBUG > 0
        debug("Connected!");
      #endif
      // once connected, publish an announcement...
      client.publish(STATUS_TOPIC, "HELLO");
      client.loop();
      // ... and resubscribe
      client.subscribe(COMMAND_TOPIC);
      client.loop();
    } else {
      #if DEBUG > 0
        debug("Unable to connect to MQTT broker. State: %s. Try again in 5 seconds...", client.state());
      #endif
      // wait 5 seconds before retrying
      blinkLed(500, 5);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (length == 13) {
    // decoding payload
    // ex: 1;11111;01000
    digitalWrite(LED_BUILTIN, HIGH);
    char cmd[14];
    strncpy(cmd, payload, 13);
    cmd[1] = '\0';
    cmd[7] = '\0';
    cmd[13] = '\0';
    char* addr1 = &cmd[2];
    char* addr2 = &cmd[8];
    if ((char)payload[0] == '1') {
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
    // send response...
    client.publish(COMMAND_RESULT_TOPIC, "ok");
    client.loop();
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    #if DEBUG > 0
      debug("Unable to decode incoming payload from topic %s:", topic);
      for (int i=0;i<length;i++) {
        Serial.print((char)payload[i]);
      }
      Serial.println();
    #endif
    // send response...
    client.publish(COMMAND_RESULT_TOPIC, "error: unable to decode incoming payload");
    client.loop();
  }
}

