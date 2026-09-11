/*
  Example arduino sketch to print the values
  seen by an M5Stack RF433R connected to and
  AtomS3 Lite. Can be used to determine what
  codes an existing RF remote sends to later
  replay those using an RF433T unit.

  In Arduino IDE, run/upload this sketch and
  monitor the Serial Monitor while pressing
  buttons on the remote nearby to see the data
  that is received.
*/

#include <RCSwitch.h>

// GPIO1: the RF433R's Grove cable carries RF_RX on its white wire, which
// lands on the AtomS3 Lite's G1 pin (its yellow wire is NC). This differs
// from the RF433T transmitter, whose RF_TX signal is on the yellow wire
// (G2/GPIO2, see transmit.ino) - the two Units don't share a signal pin.
const int RX_INTERRUPT = 1;
const int SERIAL_BAUD = 9600;

RCSwitch rfListener = RCSwitch();

void logCapturedCode() {
  Serial.print("code=");
  Serial.print(rfListener.getReceivedValue());
  Serial.print(" bits=");
  Serial.print(rfListener.getReceivedBitlength());
  Serial.print(" pulse=");
  Serial.print(rfListener.getReceivedDelay());
  Serial.print("protocol=");
  Serial.println(rfListener.getReceivedProtocol());
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  rfListener.enableReceive(RX_INTERRUPT);
}

void loop() {
  if (!rfListener.available()) {
    return;
  }
  logCapturedCode();
  rfListener.resetAvailable();
}
