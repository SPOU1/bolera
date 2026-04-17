#pragma once
#include <avr/io.h>

class Led {
	private:
	volatile uint8_t* portReg;
	volatile uint8_t* ddrReg;
	uint8_t mask;
	
	public:
	Led(volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
		portReg	= portR;
		ddrReg	= ddrR;
		mask	= m;
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
};