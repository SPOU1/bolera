#pragma once
#include <avr/io.h>

class PinsManager {
	private:
	uint8_t pinsMask;
	uint8_t score;
	
	public:
	PinsManager() {
		pinsMask = 0b00111111;
		score = 0;
	}
	
	void init() {
		DDRK &= ~0x3F; // 0b11000000
		PORTK |= 0x3F;
		
		// PCINTs
		PCICR |= (1 << PCIE2);
		PCMSK2 |= 0x3F;
	}
	
	void onInterrupt() {
		uint8_t pinsState = PINK & 0x3F;
		
		for (int i=0; i<6; i++) {
			if (!(pinsState & (1<<i)) && (pinsMask & (1<<i))) {
				score++;
				pinsMask &= ~(1<<i);
			}
		}
	}
	
	uint8_t getScore() {return score;}
		
	void reset() {
		pinsMask = 0b00111111;
		score = 0;
	}
};