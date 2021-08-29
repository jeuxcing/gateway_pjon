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


void wire_receiver(uint8_t *payload, uint16_t length, const PJON_Packet_Info &info) {
  Serial.write((uint8_t)length);
  Serial.write(payload, length);
  Serial.flush();
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
          Serial.print((uint8_t)6);
          Serial.print(pong);
          Serial.flush();
          last_ping = millis();
        }

        return true;
      } 
  }

  return false;
}

uint8_t buffer[50];

bool serial2pjon(uint8_t* message, uint8_t length) {  
  if(connected) {
    uint16_t result;
    if (message[0] != 'L' and message[0] != 'S')
      result = main_bus.send_packet_blocking(message[0], message, length);
    else if (message[0] == 'L') {
      // command
      buffer[0] = message[0];
      // Line
      buffer[1] = message[2];
      // Led
      uint8_t segment_size = message[1] == 'R' ? 12 : 24;
      buffer[2] = message[3] * segment_size + message[4];
      // Colors
      buffer[3] = message[5];
      buffer[4] = message[6];
      buffer[5] = message[7];
      
      result = main_bus.send_packet_blocking(message[1], buffer, length);
    } else if (message[0] == 'S') {
      // command
      buffer[0] = message[0];
      // Line
      buffer[1] = message[2];
      // Led start
      uint8_t segment_size = message[1] == 'R' ? 12 : 24;
      buffer[2] = message[3] * segment_size + message[4];
      // Led stop
      buffer[3] = message[3] * segment_size + message[5];
      // Colors
      buffer[4] = message[6];
      buffer[5] = message[7];
      buffer[6] = message[8];
      
      result = main_bus.send_packet_blocking(message[1], buffer, length);
    }
    
    return result;
  }

  return false;
}

void setup() {
  serial_setup();
  pjon_setup();
}


void debug() {
  uint8_t msg[10];
  msg[0] = 'L';
  uint8_t red[] = {0, 100, 0};
  uint8_t black[] = {0, 0, 0};
  
  for (int l=0 ; l<3 ; l++) {
    msg[1] = l;

    /*
    for (int led=0 ; led<36 ; led++) {
      msg[2] = led;
      memcpy((msg+3), red, 3);
      uint16_t result = main_bus.send_packet_blocking('R', msg, 6);
      main_bus.send_packet_blocking('H', msg, 6);
      main_bus.send_packet_blocking('V', msg, 6);
      Serial.println(result);
      delay(50);
      memcpy((msg+3), black, 3);
      main_bus.send_packet_blocking('H', msg, 6);
      main_bus.send_packet_blocking('V', msg, 6);
      main_bus.send_packet_blocking('R', msg, 6);
    }

    for (int led=0 ; led<48 ; led++) {
      msg[2] = led;
      memcpy((msg+3), red, 3);
      uint16_t result = main_bus.send_packet_blocking('H', msg, 6);
      main_bus.send_packet_blocking('V', msg, 6);
      main_bus.send_packet_blocking('R', msg, 6);
      Serial.println(result);
      delay(50);
      memcpy((msg+3), black, 3);
      main_bus.send_packet_blocking('H', msg, 6);
      main_bus.send_packet_blocking('V', msg, 6);
      main_bus.send_packet_blocking('R', msg, 6);
    }/**/

    for (int led=0 ; led<48 ; led++) {
      msg[2] = led;
      memcpy((msg+3), red, 3);
      uint16_t result = main_bus.send_packet_blocking('V', msg, 6);
      main_bus.send_packet_blocking('H', msg, 6);
      main_bus.send_packet_blocking('R', msg, 6);
      Serial.println(result);
      delay(100);
      memcpy((msg+3), black, 3);
      main_bus.send_packet_blocking('V', msg, 6);
      main_bus.send_packet_blocking('H', msg, 6);
      main_bus.send_packet_blocking('R', msg, 6);
    }
  }
}


void loop() {
  debug();
  //serial_loop();
  //pjon_loop();
}
