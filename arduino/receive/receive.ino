/*
  Example arduino sketch to print the values
  seen by an M5Stack RF433R connected to and
  AtomS3 Lite. Can be used to determine what
  codes an existing RF remote sends to later
  replay those using an RF433T unit.
*/

#include <RCSwitch.h>

const int RX_INTERRUPT = 1;  // GPIO2 on the AtomS3 Lite Grove port

RCSwitch rfListener = RCSwitch();

void logCapturedCode() {
  Serial.print("code=");
  Serial.print(rfListener.getReceivedValue());
  Serial.print(" bits=");
  Serial.print(rfListener.getReceivedBitlength());
  Serial.print(" pulse=");
  Serial.print(rfListener.getReceivedDelay());
  Serial.print("us protocol=");
  Serial.println(rfListener.getReceivedProtocol());
}

void setup() {
  Serial.begin(9600);
  rfListener.enableReceive(RX_INTERRUPT);
}

void loop() {
  if (!rfListener.available()) {
    return;
  }
  logCapturedCode();
  rfListener.resetAvailable();
}
