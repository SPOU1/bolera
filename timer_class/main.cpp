#include <avr/io.h>
#include <avr/interrupt.h>
#include "Display.h"
#include "Led.h"
#include "Timer.h"

// Display
volatile uint8_t* segPorts[7] = {&PORTL, &PORTL, &PORTL, &PORTD, &PORTL, &PORTL, &PORTL};
volatile uint8_t* segDdrs[7]  = {&DDRL,  &DDRL,  &DDRL,  &DDRD,  &DDRL,  &DDRL,  &DDRL};
uint8_t segMasks[7]           = {(1<<PL0), (1<<PL1), (1<<PL2), (1<<PD6), (1<<PL4), (1<<PL5), (1<<PL6)};

Display display(segPorts, segDdrs, segMasks, &PORTL, &DDRL, (1<<PL7));

// LED
Led led(&PORTD, &DDRD, (1<<PD7));

// Timer
Timer systemTimer;
Timer* timerPtr = 0;

// Timer 4
ISR(TIMER4_COMPA_vect) {
	if (timerPtr) {
		timerPtr->addTick();
	}
}

int main() {
	timerPtr = &systemTimer;

	cli();
	
	display.init();
	led.init();
	systemTimer.init();
	
	sei();

	display.setScore(67);
	bool endGame = true;

	while (1) {
		uint32_t currentTime = systemTimer.millis();

		display.update(currentTime, endGame);
		led.update(currentTime, endGame);
	
	}
	
}