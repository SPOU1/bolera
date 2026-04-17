#include <avr/io.h>
#include <avr/interrupt.h>
#include "Led.h"
#include "Switch.h"

Led led(&PORTD, &DDRD, (1<<PD7));

Switch sw6(&PIND, &PORTD, &DDRD, (1<< PD3));
Switch* sw6ptr = 0;

ISR(INT3_vect) {
	if (sw6ptr) sw6ptr->onInterrupt();
}

void initISR() {
	EICRA |= (1 << ISC31);
	EICRA &= ~(1 << ISC30);
	EIMSK |= (1 << INT3);
}

int main() {
	sw6ptr = &sw6;
	
	cli();
	led.init();
	sw6.init();
	initISR();
	
	sei();
	
	while(1) {
		if(sw6.consumePressedFlag()) {
			led.toggle();
		}
	}
	return 0;
}