#include <avr/io.h>
#include <avr/interrupt.h>
#include "Timer.h"
#include "SwitchUser.h"
#include "SwitchMotor.h"
#include "Motor.h"

Timer systemTimer;
Timer* timerPtr = 0;

ISR(TIMER4_COMPA_vect) {
	if (timerPtr) timerPtr->addTick();
}

// User Switch in PD3
SwitchUser sw6(&PIND, &PORTD, &DDRD, (1<<PD3));

// Switch 1 in PK6
SwitchMotor sw1(&PINK, &PORTK, &DDRK, (1<<PK6));

// Motor 1 (M1_di=PB0, M1_en=PB4/OC2A)
Motor m1(&PORTB, &DDRB, (1<<PB0),
&PORTB, &DDRB, (1<<PB4),
&OCR2A, &sw1);

int main(void) {
	timerPtr = &systemTimer;
	
	cli();
	systemTimer.init();
	sw6.init();
	sw1.init();
	m1.init();
	sei();

	bool M1_UP = true;
	bool M1_DOWN = false;

	while (1) {
		uint32_t currentMillis = systemTimer.millis();
		
		// Update
		sw6.update(currentMillis);
		sw1.update(currentMillis);
		m1.update(currentMillis);
		
		if (sw6.consumeClick()) {
			if (!m1.getIsMoving()) {
				m1.goTo(M1_DOWN, currentMillis);
				} else {
				m1.stop();
			}
		}
	}
	return 0;
}