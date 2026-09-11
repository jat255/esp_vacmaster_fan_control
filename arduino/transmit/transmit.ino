#include <RCSwitch.h>
RCSwitch tx = RCSwitch();

const unsigned long POWER  = 213761;
const unsigned long SPEED1 = 213762;
const unsigned long SPEED2 = 213764;
const unsigned long SPEED3 = 213768;

void setup() {
  Serial.begin(115200);
  tx.enableTransmit(2);       // your RF433T data pin
  tx.setProtocol(1);
  tx.setPulseLength(350);
  tx.setRepeatTransmit(10);
  Serial.println("Ready. Send 0=power 1=speed1 2=speed2 3=speed3");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case '0': Serial.println("POWER");  tx.send(POWER, 24);  break;
      case '1': Serial.println("SPEED1"); tx.send(SPEED1, 24); break;
      case '2': Serial.println("SPEED2"); tx.send(SPEED2, 24); break;
      case '3': Serial.println("SPEED3"); tx.send(SPEED3, 24); break;
    }
  }
}
