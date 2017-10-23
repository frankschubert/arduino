int ledPin;
int counter;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // opens serial port, sets data rate
  ledPin = 13;
  counter = 0;
  pinMode(ledPin, OUTPUT); // sets the digital pin as output
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(ledPin, HIGH); // light up led
  delay(50); // delay 0.05s
  digitalWrite(ledPin, LOW); // turn off led
  Serial.print("iteration: ");
  Serial.print(counter);
  Serial.println();
  delay(1000);
  counter = counter +2;
}
