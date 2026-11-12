// https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/ATtiny_x04.gif
#include <Arduino.h>
void setup(){
	pinMode(10,OUTPUT);// PA3
	pinMode(3,INPUT);// PA7
}
void loop(){
	digitalWrite(10,HIGH);delay(500);
	digitalWrite(10, LOW);delay(500);
}
