#include <Arduino.h>
#include <Servo.h>

#define VR 1
#define BTN 3
#define OUT 2
#define ESC_MIN 1000
#define ESC_MAX 2000

Servo esc;

void setup(){
	pinMode(VR,INPUT);analogReadResolution(10);
	pinMode(BTN,INPUT_PULLUP);
	esc.attach(OUT,ESC_MIN,ESC_MAX);
}
void loop(){
	esc.writeMicroseconds(map(analogRead(VR),0,1023,ESC_MIN,ESC_MAX));
	delay(20);
}
