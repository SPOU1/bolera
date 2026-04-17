/*

#include <avr/io.h>
#define F_CPU 8000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "Switch.h"
#include "Motor.h"

// MOTOR 4
Switch sw4(&PIND, &PORTD, &DDRD, (1<<PD1));

Motor motor4(
&PORTB, &DDRB, (1<<PB3),	// DIR
&PORTB, &DDRB, (1<<PB7),	// EN
&OCR0A,					// Timer 5 (low)
&sw4						// Switch 5
);

// Switch Pointers
Switch* sw4ptr = 0;

// Interrupts
ISR(INT1_vect) {
	if (sw4ptr) sw4ptr->onInterrupt();
}

void initDebugLed() {
	DDRD |= (1<<PD7);
	PORTD &= ~(1<<PD7);
}

void initISRs() {
	// Flanco de bajada
	EICRA |= (1 << ISC11);
	EICRA &= ~((1 << ISC10));
	EIMSK |= (1 << INT1);
}

void initPWMTimers() {
	// Timer 5 (low)
	TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler 64
}

int main() {
	
	cli();
	sw4ptr = &sw4;
	
	initDebugLed();
	initISRs();
	initPWMTimers();
	
	sw4.init();
	motor4.init();
	sei();
	
	// Arranque visual
	for(int i = 0; i < 3; i++) {
		PORTD |= (1 << PD7);
		_delay_ms(150);
		PORTD &= ~(1 << PD7);
		_delay_ms(150);
	}

	#define M4_CLOSE 1
	#define M4_OPEN 0

	motor4.calibrate(M4_OPEN);
	
	_delay_ms(200);

	motor4.goTo(M4_CLOSE);
	
	while(1) {
		// Update motors
		motor4.update();

		if (!motor4.getIsMoving()) {
			PORTD ^= (1<<PD7);
			
			if (sw4.getState() == Switch::SIDE_1) motor4.goTo(M4_OPEN);
			else if (sw4.getState() == Switch::SIDE_2) motor4.goTo(M4_CLOSE);
		}
	}
	return 0;
}

*/