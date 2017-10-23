#include <EEPROM.h>
#include <OneWire.h>

OneWire  ds(2);  // on pin 2 (a 4.7K resistor is necessary)

const unsigned int array_size = 160;
byte temperatures[161];
unsigned int eeprom_index_cur = 0; // current index EEPROM address
unsigned int eeprom_index_max = 1023; // max EEPROM address
// wait time between sampling the temperature (in seconds)
const unsigned int interval = 600;
unsigned long previousMillis = 0; // will store the last time temp. was sampled and saved to EEPROM

void setup(void) {
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
}

void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  byte celsius;
  
  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
    return;
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) {
      //Serial.println("CRC is not valid!");
      return;
  }

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
      //Serial.println("Device is not a DS18x20 family device.");
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

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

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
  celsius = (int)raw / 16.0;

  if (celsius <= 22) {
    digitalWrite(10, HIGH);
  }else {
    digitalWrite(10, LOW);
  }

 if ((celsius > 22) && (celsius < 27)) {
    digitalWrite(11, HIGH);
  }else {
    digitalWrite(11, LOW);
  }

  if (celsius >= 27) {
    digitalWrite(12, HIGH);
  }else {
    digitalWrite(12, LOW);
  }

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > (long)interval*1000) {
    previousMillis = currentMillis;
    // if reached end of array, flip to first element
    if (eeprom_index_cur > eeprom_index_max) {
      eeprom_index_cur = 0;
    }
    // save temp in EEPROM
    EEPROM.update(eeprom_index_cur, celsius);
    eeprom_index_cur++;
  }
}
