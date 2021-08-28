#include <PJONSoftwareBitBang.h>

#define NET_PIN 2

PJONSoftwareBitBang main_bus;


#define TIMEOUT 5000
#define CONNECTION_ID 42

/* serial global variable*/
bool connected = false;
long last_ping = 0;
char ledPin = LED_BUILTIN;

/* serial section */
void serial_setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void pjon_setup() {
  main_bus.strategy.set_pin(NET_PIN);
  main_bus.set_id('M');
  main_bus.set_receiver(wire_receiver);
  main_bus.begin();
}

void serial_loop() {
  if (!connected) {
    connection();
  } 
  else {
    if (millis() - last_ping > TIMEOUT)
      connected = false;

    if (Serial.available())
      read_serial();
  }
}

void pjon_loop() {
  main_bus.receive();
}

void connection () {
  Serial.flush();

  connected = false;

  // Connection
  while (!connected) {
    // Send hello using the serial port
    if (Serial.available()) {      
      String str = Serial.readString();
      int val = str.toInt();
      
      if(val == CONNECTION_ID)
        connected = true;
    } else {
      Serial.print("HELLO ");
      Serial.println((uint8_t)CONNECTION_ID);
      delay(500);
    }
  }
  Serial.flush();
  Serial.println("CONNECTED");
  Serial.flush();

  // turn the LED on when connected
  digitalWrite(ledPin, HIGH);
  last_ping = millis();
}


uint8_t read_buffer[255] = {0};
void read_serial(){
  size_t message_length = Serial.read();
  Serial.readBytes(read_buffer, message_length);

  if(pong(read_buffer, message_length)) {}
  else if(serial2pjon(read_buffer, message_length)) {}
}

bool pong(uint8_t* message, uint8_t length) {
  if(length == 6) {
    if(message[0] == 'P' &&
       message[1] == 'I' &&
       message[2] == 'N' &&
       message[3] == 'G' &&
       message[4] == ' ' &&
       message[5] == '?') {
        String pong = "PONG !";
        connected = true;

        if (connected) {
          Serial.println(pong);
          last_ping = millis();
        }

        return true;
      } 
  }

  return false;
}

bool serial2pjon(uint8_t* message, uint8_t length) {
  if(connected) {
    main_bus.send_packet_blocking(message[0], message, length);
    return true;
  }

  return false;
}

void setup() {
  serial_setup();
  pjon_setup();
}

void loop() {
  serial_loop();
  pjon_loop();
}
