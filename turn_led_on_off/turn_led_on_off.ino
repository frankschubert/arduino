/* 
 *  Blink
 *  
 *  Turns on LED for one second, then off for one second, repeatedly.
 */

const int ledPin = 11; // number of the pin connected to LED

void setup() {
  // put your setup code here, to run once:

  pinMode(ledPin, OUTPUT); // initialize the digital pin as an output

}

void loop() {
  // put your main code here, to run repeatedly:

  digitalWrite(ledPin, HIGH); // turn the LED on
  delay(1000); // wait 1 second
  digitalWrite(ledPin, LOW); // turn the LED off
  delay(1000); // wait 1 second

}
