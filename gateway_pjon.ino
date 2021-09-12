#include <PJONSoftwareBitBang.h>


#define NET_PIN 12
#define NET_ID 'M' /* As Main address on this 1-wire network */


PJONSoftwareBitBang bus;


void cmd_handler(uint8_t * payload, uint16_t length, const PJON_Packet_Info &info);
void read_serial();

void setup() {
  // Serial Init
  Serial.begin(115200);

  // Led init
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Bus init
  bus.strategy.set_pin(NET_PIN);
  bus.set_id(NET_ID);
  // TODO: from 1-wire to serial
  bus.set_receiver(cmd_handler);
} 

void loop() {
    if (Serial.available())
      read_serial();
    delay(1);
}


uint8_t buffer[254];

void read_serial() {
  int val = Serial.read();
  // If no data return
  if (val == -1)
    return;

  // Synchronisation data arrays (in case of packet loss)
  if (val == 255) {
    while (val == 255)
      val = Serial.read();
    // TODO: Notify synch on serial
    digitalWrite(LED_BUILTIN,HIGH);
    return;
  }

  // Read the packet
  buffer[0] = (uint8_t)val;
  uint8_t idx=1;
  while (val > 0) {
    // TODO: Add a timeout
    if (Serial.available()) {
      buffer[idx++] = (uint8_t)Serial.read();
      val -= 1;
    } else {
      delay(1);
    }
  }

  uint8_t msg[16];

  // Modify the packet for line coordinates
  if (buffer[1] == 'L') {
    msg[0] = 6;
    msg[1] = 'L';
    msg[2] = buffer[3]; // Line coordinate
    msg[3] = buffer[4] * (buffer[1] == 'R' ? 12 : 24) + buffer[5]; // Led coordinate
    // Colors
    memcpy(msg+4, buffer+6, 3);
  }

  // Send the packet on the 1-Wire
  bus.send_packet_blocking(msg[0], msg+1, buffer[2]);
}

void cmd_handler(uint8_t * payload, uint16_t length, const PJON_Packet_Info &info) {
  // TODO  
}
