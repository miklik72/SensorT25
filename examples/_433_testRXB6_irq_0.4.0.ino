/*
Thermo sensor Sencor T25 reader via RXB6 receiver
Show data for sensors every 10 seconds.
Output to serial console
Miklik, 20.6.2016

*/

#include <SensorT25.h>

#define IRQ_PIN 2             // RF input with irq

void setup()
{
  Serial.begin(9600);
  SensorT25::enable(IRQ_PIN);
  Serial.println("Start");           
}

void loop()
{  
  for(int i = 0; i < SENSCOUNT; i++) {
    Serial.print("CH ");
    Serial.print(i+1);                //channel
    Serial.print("-V ");
    Serial.print(SensorT25::isValid(i));         //valid data
    Serial.print("-SID ");
    Serial.print(SensorT25::getSID(i));          //sensor ID
    Serial.print("-TEMP ");
    Serial.print(SensorT25::getTemperature(i),1);
    Serial.print("-AGE ");
    Serial.print(SensorT25::getValueAge(i));
    Serial.println();
  }
  delay(10000);
}
