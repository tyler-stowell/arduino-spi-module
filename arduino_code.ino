#include<SPI.h>
#include "pins_arduino.h"

unsigned char output = 97;

void setup(){
	Serial.begin(9600);
	pinMode(MISO, OUTPUT);
	SPCR |= _BV(SPE);
	SPI.attachInterrupt();
}

ISR(SPI_STC_vect){
	unsigned char input = SPDR;
	Serial.println(input);
	SPDR = output;
}

void loop(){

}
