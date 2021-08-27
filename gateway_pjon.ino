#include <EEPROM.h>

#define EEPROM_ID_ADDRESS 1
#define TIMEOUT 5000

bool connected = false;
long last_ping = 0;
char ledPin = LED_BUILTIN;

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}


void loop() {
  if (!connected) {
    connection();
  } 
  else {
    if (millis() - last_ping > TIMEOUT)
      connected = false;

    if (Serial.available())
      readInput();
  }
}


void connection () {
  Serial.flush();

  connected = false;
  char ARDUINO_ID = EEPROM.read(EEPROM_ID_ADDRESS);

  // Connection
  while (!connected) {
    // Send hello using the serial port
    if (Serial.available()) {      
      String str = Serial.readString();
      int val = str.toInt();
      
      if(val == ARDUINO_ID)
        connected = true;
    } else {
      Serial.print("HELLO ");
      Serial.println((uint8_t)ARDUINO_ID);
      delay(1000);
    }
  }
  Serial.flush();
  Serial.println("CONNECTED");
  Serial.flush();

  // turn the LED on when connected
  digitalWrite(ledPin, HIGH);
  last_ping = millis();
}


void readInput(){
  String str = "";
  String ping = "PING ?";

  str = Serial.readStringUntil('\n');

  if (ping.equals(str))
    pong();
}

void pong () {
  String pong = "PONG !";

  connected = true;

  if (connected) {
    Serial.println(pong);
    last_ping = millis();
  }

}
