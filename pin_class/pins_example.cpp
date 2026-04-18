#include <avr/io.h>
#include <avr/interrupt.h>
#include "Timer.h"
#include "PinsManager.h"

Timer systemTimer;
Timer* timerPtr = 0;

PinsManager pins;
PinsManager* pinsPtr = 0;

ISR(TIMER4_COMPA_vect) {
	if (timerPtr) timerPtr->addTick();
}

ISR(PCINT2_vect) {
	if (pinsPtr) pinsPtr->onInterrupt();
}

int main(void) {
	timerPtr = &systemTimer;
	pinsPtr = &pins;
	
	cli();
	systemTimer.init();
	pins.init();
	sei();

	while (1) {
		uint32_t currentTime = systemTimer.millis();
		
		uint8_t score = pins.getScore();
		
		if (score == 6) {
			pins.reset();
		}
	}
	return 0;
}