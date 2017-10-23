#include <EEPROM.h>

unsigned int eeprom_index_max = 170;
unsigned int value;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i=0; i<=eeprom_index_max; i++) {
    Serial.print("[");
    Serial.print(i);
    Serial.print("] ");
    value = EEPROM.read(i);
    Serial.print(value);
    int remainder = i % 10;
    
    if ((i!=0) && (remainder == 0)){
      Serial.println();
    }
    else{
      Serial.print(" ");
    }
  }
  Serial.println();
  delay(10000);
}
