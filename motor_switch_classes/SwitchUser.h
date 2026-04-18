#pragma once
#include <avr/io.h>

class SwitchUser {
	private:
	volatile uint8_t* pinReg;
	volatile uint8_t* portReg;
	volatile uint8_t* ddrReg;
	uint8_t mask;
	
	uint32_t lastChangeTimestamp;
	const uint32_t debounceDelay = 50; //ms
	
	bool lastState;
	bool stableState;
	bool flagPressed;
	
	public:
	SwitchUser(volatile uint8_t* pinR, volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
		pinReg = pinR;
		portReg = portR;
		ddrReg = ddrR;
		mask = m;
		
		lastChangeTimestamp = 0;
		lastState = false;
		stableState = false;
		flagPressed = false;
	}
	
	void init() {
		*ddrReg &= ~mask;	// Input
		*portReg |= mask;	// Pull-up
	}
	
	void update(uint32_t currentTime) {
		bool reading = !(*pinReg & mask);
		
		if (reading != lastState) {
			lastChangeTimestamp = currentTime;
		}
		
		if ((currentTime - lastChangeTimestamp) > debounceDelay) {
			if (reading != stableState) {
				stableState = reading;
				if (stableState) {
					flagPressed = true;
				}
			}
		}
		lastState = reading;
	}
	
	bool consumeClick() {
		if (flagPressed) {
			flagPressed = false;
			return true;
		}
		return false;
	}
};