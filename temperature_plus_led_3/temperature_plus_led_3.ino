#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

// ROM = 40 19 180 220 6 0 0 142  Outside
// ROM = 40 247 29 222 6 0 0 188  3 pin solded indoors

OneWire  ds(2);  // on pin 2 (a 4.7K resistor is necessary)

void setup(void) {
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode( 9, OUTPUT); 
  pinMode( 8, OUTPUT);
  pinMode( 7, OUTPUT);
  pinMode( 6, OUTPUT);
  pinMode( 5, OUTPUT);
  Serial.begin(9600);
}

void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  const byte outdoor = 142;
  const byte indoor = 188;
  
  if ( !ds.search(addr)) {
    //Serial.println("No more addresses.");
    //Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  //Serial.print("ROM =");
  //for( i = 0; i < 8; i++) {
    //Serial.write(' ');
    //Serial.print(addr[i]);
  //}

  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return;
  }
  //Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  //Serial.print("  Data = ");
  //Serial.print(present, HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
  //Serial.print(" CRC=");
  //Serial.print(OneWire::crc8(data, 8), HEX);
  //Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;

  switch(addr[7]){
    case outdoor:
      Serial.print("Outdoor ");
      break;
    case indoor:
      Serial.print("Indoor  ");
      break;
    default:
      Serial.print("Address unknown ");
      Serial.print(addr[7]);
      break;
  }

  Serial.print("temperature = ");
  Serial.print(celsius);
  Serial.print(" C, ");
  Serial.print(fahrenheit);
  Serial.println(" F");

  // machine readable
  // m=188:temp1
  Serial.print("m=");
  // print last byte of addr
  Serial.print(addr[7]);
  Serial.print(":");
  Serial.println(celsius);

  /*
  if (celsius > 8) {
    digitalWrite(5, HIGH);
  }else{
    digitalWrite(5, LOW); 
  }

  if (celsius > 11) {
    digitalWrite(6, HIGH);
  }else{
    digitalWrite(6, LOW); 
  }

  if (celsius > 14) {
    digitalWrite(7, HIGH);
  }else{
    digitalWrite(7, LOW); 
  }

  if (celsius > 17) {
    digitalWrite(8, HIGH);
  }else{
    digitalWrite(8, LOW); 
  }

  if (celsius > 20) {
    digitalWrite(9, HIGH);
  }else{
    digitalWrite(9, LOW); 
  }

  if (celsius > 23) {
    digitalWrite(10, HIGH);
  }else {
    digitalWrite(10, LOW);
  }

  if (celsius > 26) {
    digitalWrite(11, HIGH);
  }else {
    digitalWrite(11, LOW);
  }

  if (celsius > 30) {
    digitalWrite(12, HIGH);
  }else {
    digitalWrite(12, LOW);
  }
}*/

  // 8 LEDs, so loop 8 times
  // each LED is 3 degrees Celsius apart, meaning loop counter times 3 + starting from 8 degrees celsius
  // pin assignment starts at 5 and goes up by 1 for each step
  byte pin=5;
  for(i=0; i<=7; i=i+1) {
    if (celsius > i*3+8) {
      digitalWrite(pin+i, HIGH);
    }else {
      digitalWrite(pin+i, LOW);
    }
  }

} // main loop ends here

