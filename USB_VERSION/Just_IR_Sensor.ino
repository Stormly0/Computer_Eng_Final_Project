#include <IRremote.hpp>
// In use irl

// C++ code
//

#define IR_SENSOR 4
#define PIN_OUT 3

void setup()
{
  Serial.begin(9600); 
  pinMode(LED_BUILTIN, OUTPUT);
  IrReceiver.begin(IR_SENSOR); // Start the receiver
}
void loop() {
  if (IrReceiver.decode()) {
      unsigned long Data = IrReceiver.decodedIRData.decodedRawData; 
      IrReceiver.resume(); // Enable receiving of the next value
      // Checks the code to see if the on button has been pressed and outputs the signal to the other arduino 
      if(Data == 0xBC43FF00){
        Serial.println("On button has been pressed"); 
        digitalWrite(PIN_OUT,HIGH); 
        Serial.println("Output High"); 
      }

 
      Serial.println(Data,HEX);
     
  }
    // Turns the pin back off 
    digitalWrite(PIN_OUT,LOW); 
    Serial.println("Output Low"); 
}