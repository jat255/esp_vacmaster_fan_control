/*
  Arduino sketch to replay the fixed RF433 codes captured from the
  Vacmaster remote (see receive.ino) using an M5Stack RF433T connected
  to an AtomS3 Lite. Used to validate the captured codes actually
  control the fan before committing to the ESPHome firmware.

  In Arduino IDE, run/upload this sketch, open the Serial Monitor at
  115200 baud, and send a digit (0-3) to fire the matching command.
*/

#include <RCSwitch.h>
RCSwitch tx = RCSwitch();

// All four codes share a common 20-bit prefix (213760) with a one-hot
// bit for the button - a fixed-code (non-rolling) encoder.
const unsigned long POWER  = 213761;  // toggle, not a distinct on/off code
const unsigned long SPEED1 = 213762;  // low
const unsigned long SPEED2 = 213764;  // medium
const unsigned long SPEED3 = 213768;  // high

void setup() {
  Serial.begin(115200);
  // The RF433T's Grove cable carries RF_TX on its yellow wire, which
  // lands on the AtomS3 Lite's G2 pin (GPIO2). This differs from the
  // RF433R receiver, whose RX signal is on GPIO1 (see receive.ino) -
  // the two Units don't share a signal pin.
  tx.enableTransmit(2);
  tx.setProtocol(1);       // rc-switch protocol 1, 350us pulse length
  tx.setPulseLength(350);  // matches the timing captured from the remote
  tx.setRepeatTransmit(10);  // single frames get ignored by the fan
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
