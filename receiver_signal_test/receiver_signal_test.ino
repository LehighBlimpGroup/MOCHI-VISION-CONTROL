#include <IBusBM.h>

IBusBM IBus; 
HardwareSerial MySerial0(0);

void setup() {
  // initialize serial port for debug
  Serial.begin(115200);
  // iBUS connected to MySerial0
  // Configure MySerial0 on pins TX=D6 and RX=D7 (-1, -1 means use the default)
  MySerial0.begin(115200, SERIAL_8N1, -1, -1);
  IBus.begin(MySerial0, IBUSBM_NOTIMER);
  Serial.println("Start iBUS sensor");
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  Serial.print("\nHere");
  IBus.loop();
  for (byte i = 0; i<5; i++){
    int value = IBus.readChannel(i);
    Serial.print("\nCh");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(value);
    Serial.print(" ");
  }
  delay(20);                      // wait for a second
}

