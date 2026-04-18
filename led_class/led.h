#pragma once
#include <avr/io.h>

class Led {
	private:
	volatile uint8_t* portReg;
	volatile uint8_t* ddrReg;
	uint8_t mask;

	// Timer control
	uint32_t lastTime;
	uint32_t blinkInterval;
	
	public:
	Led(volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
		portReg	= portR;
		ddrReg	= ddrR;
		mask	= m;
		
		lastTime = 0;
		blinkInterval =		100; // 100ms
	}
	
	void init() {
		*ddrReg |= mask;	// Output
		*portReg &= ~mask;	// 0
	}
	
	void on() {
		*portReg |= mask;
	}
	
	void off() {
		*portReg &= ~mask;
	}
	
	void toggle() {
		*portReg ^= mask;
	}
	
	void update(uint32_t currentTime, bool endGame) {
		if (endGame) {
			if (currentTime - lastTime >= blinkInterval) {
				toggle();
				lastTime = currentTime;
			}
		}
		// else se queda en el último estado puesto por on/off
	}
};