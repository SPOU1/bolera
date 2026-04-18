#include <avr/io.h>
#include <avr/interrupt.h>

#include "SwitchUser.h"
#include "Led.h"

Led led(&PORTD, &DDRD, (1<<PD7));
SwitchUser sw6(&PIND, &PORTD, &DDRD, (1<<PD3));

int main() {
	cli();
	led.init();
	sw6.init();
	sei();
	
	while(1) {
		if(sw6.consumeClick()) {
			led.toggle();
		}
	}
}