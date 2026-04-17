#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>

class Timer {
	private:
	volatile uint32_t system_millis;
	
	public:
	Timer() {
		system_millis = 0;
	}
	
	void init() {
		// CTC
		TCCR4B &= ~((1 << WGM43) | (1 << WGM41) | (1 << WGM40));
		TCCR4B |= (1 << WGM42);

		// 64 prescaler
		TCCR4B |= (1 << CS41) | (1 << CS40);
		TCCR4B &= ~(1 << CS42);

		// 1ms interrupt
		OCR4A = 124;
		TIMSK4 |= (1 << OCIE4A);
	}
	
	void addTick() {
		system_millis++;
	}
	
	uint32_t millis() {
		uint32_t time;
		uint8_t oldSREG = SREG;
		cli();
		time = system_millis;
		SREG = oldSREG;
		sei();
		return time;
	}

};