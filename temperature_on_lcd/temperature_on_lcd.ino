#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>

/********************************************************************/
// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 6
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

#define TEMPERATURE_PRECISION 12
#define MAX_SENSORS 3
DeviceAddress Thermometer[MAX_SENSORS];
byte numberOfDevices;
byte Limit;

DeviceAddress ThermometerOutsideAddress = { 0x28, 0x13, 0xB4, 0xDC, 0x06, 0x00, 0x00, 0x8E };
DeviceAddress ThermometerInsideAddress = { 0x28, 0xF7, 0x1D, 0xDE, 0x06, 0x00, 0x00, 0xBC };

/********************************************************************/
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// initialize varibales for air quality sensor
#define AIR_QUALITY_SENSOR_PIN 8
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
//float ugm3;
float ratio = 0;
float concentration = 0;
// end initialize variables for air quality sensor

// initialize variables for smoothing of aqi readings
const int numReadings = 100;
int readings[numReadings];
int readIndex = 0;
int total = 0;
int average = 0;
// end of smoothing variables

/* OneWire debug function for printing a device address to serial */
void printAddress(DeviceAddress deviceAddress) {
  Serial.print(F("{ "));
  for (uint8_t i = 0; i < 8; i++) {
    // zero pad the address if necessary
    Serial.print(F("0x"));
    if (deviceAddress[i] < 16) {
      Serial.print("0");
    };
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) {
      Serial.print(F(", "));
    };
  };
  Serial.print(F(" }"));
};

/* OneWire function to print MAC address */
void printMAC(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    //if (deviceAddress[i] < 16) {Serial.print("0");};
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) {
      Serial.print("-");
    };
  };
};

/* OneWire function to compare to OneWire device addresses */
bool isSame(DeviceAddress a, DeviceAddress b) {
  if ( memcmp( (const void *)a, (const void *)b, sizeof(a)) == 0) {
    return true;
  } else {
    return false;
  }
}

// convert from particles/0.01 ft3 to μg/m3
float pcs2ugm3 (float concentration_pcs)
{
  float pi = 3.14159;
  // All particles are spherical, with a density of 1.65E12 µg/m3
  float density = 1.65 * pow (10, 12);
  // The radius of a particle in the PM2.5 channel is .44 µm
  float r25 = 0.44 * pow (10, -6);
  float vol25 = (4 / 3) * pi * pow (r25, 3);
  float mass25 = density * vol25; // ug
  float K = 3531.5; // per m^3

  return concentration_pcs * K * mass25;
}

// https://www3.epa.gov/airquality/particlepollution/2012/decfsstandards.pdf
static struct aqi {
  float clow;
  float chigh;
  int llow;
  int lhigh;
} aqi[] = {
  {0.0,    12.4,   0, 50},
  {12.1,   35.4,  51, 100},
  {35.5,   55.4, 101, 150},
  {55.5,  150.4, 151, 200},
  {150.5, 250.4, 201, 300},
  {250.5, 350.4, 301, 350},
  {350.5, 500.4, 401, 500},
};

// Guidelines for the Reporting of Daily Air Quality – the Air Quality Index (AQI)
// https://www3.epa.gov/ttn/oarpg/t1/memoranda/rg701.pdf
//
// Revised air quality standards for particle pollution and updates to the air quality index (aqi)
// https://www3.epa.gov/airquality/particlepollution/2012/decfsstandards.pdf
//
// calculate AQI (Air Quality Index) based on μg/m3 concentration
int ugm32aqi (float ugm3)
{
  int i;

  for (i = 0; i < 7; i++) {
    if (ugm3 >= aqi[i].clow &&
        ugm3 <= aqi[i].chigh) {
      // Ip =  [(Ihi-Ilow)/(BPhi-BPlow)] (Cp-BPlow)+Ilow,
      return ((aqi[i].lhigh - aqi[i].llow) / (aqi[i].chigh - aqi[i].clow)) *
             (ugm3 - aqi[i].clow) + aqi[i].llow;
    }
  }

  return 0;
}
// function to smooth aqi aka average
int getNewAverageAqi(int newAqi) {
   total = total - readings[readIndex];
   readings[readIndex] = newAqi;
   total = total + readings[readIndex];
   readIndex++;
   // if we are at the end of the array, wrap around
   if (readIndex >= numReadings) {
    readIndex = 0;
   }
   return (total / numReadings);
}
// end smoothing aqi calc

void setup(void) {
  // start serial port
  Serial.begin(9600);
  printMAC(ThermometerOutsideAddress);
  // Start up the OneWire sensor library
  sensors.begin();
  // detecting OneWire devices
  Serial.print(F("Locating devices..."));
  numberOfDevices = sensors.getDeviceCount();
  Serial.print(F("Found "));
  Serial.print(numberOfDevices, DEC);
  Serial.println(F(" devices."));
  Serial.print(F("Parasite power is: "));
  if (sensors.isParasitePowerMode()) {
    Serial.println(F("ON"));
  }
  else {
    Serial.println(F("OFF"));
  };
  for (byte index = 0; index < numberOfDevices; index++) {
    if (sensors.getAddress(Thermometer[index], index)) {
      Serial.print(F("Found device "));
      Serial.print(index, DEC);
      Serial.print(F(" with address: "));
      printMAC(Thermometer[index]);
      Serial.println();

      Serial.print(F("Setting resolution to "));
      Serial.println(TEMPERATURE_PRECISION, DEC);
      sensors.setResolution(Thermometer[index], TEMPERATURE_PRECISION);
      delay(750 / (1 << (12 - TEMPERATURE_PRECISION)));
      Serial.print(F("Resolution actually set to: "));
      Serial.print(sensors.getResolution(Thermometer[index]), DEC);
      Serial.println();
    }
    else {
      Serial.print(F("Found ghost device at "));
      Serial.print(index, DEC);
      Serial.print(F(" but could not detect address. Check power and cabling"));
    };
  };
  // initialize aqi smoothing readings array
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  // end init aqi smoothing

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // set up the air quality sensor pin mode to input
  pinMode(AIR_QUALITY_SENSOR_PIN, INPUT);
  // need timer for quality sensor calcs
  starttime = millis();
}
void loop(void) {

  // air quality sensor
  duration = pulseIn(AIR_QUALITY_SENSOR_PIN, LOW);
  lowpulseoccupancy = lowpulseoccupancy + duration;
  if ((millis() - starttime) > sampletime_ms)
  {
    ratio = lowpulseoccupancy / (sampletime_ms * 10.0); // Integer percentage 0=>100
    concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
    Serial.print(lowpulseoccupancy);
    Serial.print(",");
    Serial.print(ratio);
    Serial.print(",");
    Serial.print(concentration);
    Serial.print(" pcs/0.01cf;");
    float ugm3 = pcs2ugm3(concentration);
    int aqi = ugm32aqi(ugm3);
    Serial.print(ugm3);
    Serial.print(" μg/m3;");
    Serial.print("AQI = ");
    Serial.println(aqi);
    lowpulseoccupancy = 0;
    starttime = millis();
    lcd.setCursor(0, 1);
    lcd.print("AQI:");
    lcd.setCursor(4, 1);
    lcd.print(aqi);
    lcd.print(" avg:");
    lcd.print(getNewAverageAqi(aqi));
    lcd.print("     ");
  }
  // end air quality sensor stuff


  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensors.requestTemperatures(); // Send the command to get temperature readings
  for (int index = 0; index < numberOfDevices; index++) {
    //Serial.print("index: ");
    //Serial.println(index);
    if ( isSame(Thermometer[index], ThermometerOutsideAddress) ) {
      lcd.setCursor(0, 0);
      lcd.print("O:");
      lcd.setCursor(2, 0);
      lcd.print(int(sensors.getTempCByIndex(index)));
      lcd.print((char) 223);
      lcd.print("C");
    } else if ( isSame(Thermometer[index], ThermometerInsideAddress) ) {
      lcd.setCursor(8, 0);
      lcd.print("I:");
      lcd.setCursor(10, 0);
      lcd.print(int(sensors.getTempCByIndex(index)));
      lcd.print((char) 223);
      lcd.print("C");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("n/a");
      lcd.setCursor(4, 0);
      lcd.print(int(sensors.getTempCByIndex(index)));
      lcd.print((char) 223);
      lcd.print("C");
    }
  }
  //delay(1000);
}
