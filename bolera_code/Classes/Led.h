#pragma once
#include <avr/io.h>

class Led {
	/*
	 * ==== Led ====
	 * Control de diodo LED.
	 * Opera de forma no bloqueante.
	 */
	private:
	volatile uint8_t* portReg;
	volatile uint8_t* ddrReg;
	uint8_t mask;

	// Timer control
	uint32_t lastTime;
	uint32_t blinkInterval;
	
	bool isActive;
	
	public:
	Led(volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
		portReg	= portR;
		ddrReg	= ddrR;
		mask	= m;
		
		lastTime = 0;
		blinkInterval =	100; // 100ms
		isActive = false;
	}
	
	void init() {
		*ddrReg |= mask;	// Output
		*portReg &= ~mask;	// 0
	}
	
	void on() {
		isActive = true;
		*portReg |= mask;
	}
	
	void off() {
		isActive = false;
		*portReg &= ~mask;
	}
	
	void toggle() {
		*portReg ^= mask;
	}
	
	void update(uint32_t currentTime, bool lastTurn) {
		// Parpadeo de último turno
		if(isActive) {	
			if (lastTurn) {
				if (currentTime - lastTime >= blinkInterval) {
					toggle();
					lastTime = currentTime;
				}
			} else {
				*portReg |= mask;
			}
		} else {
			*portReg &= ~mask;
		}
	}
};