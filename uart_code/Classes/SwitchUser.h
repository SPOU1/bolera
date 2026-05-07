#pragma once
#include <avr/io.h>

class SwitchUser {
	private:
	volatile uint8_t* pinReg;
	volatile uint8_t* portReg;
	volatile uint8_t* ddrReg;
	uint8_t mask;
	
	// Filtro Integrador
	static const uint8_t DEBOUNCE_SAMPLES = 20;
	uint8_t counter;
	uint32_t lastSampleTime;
	bool stableState;
	bool flagPressed;
	
	public:
	SwitchUser(volatile uint8_t* pinR, volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
		pinReg = pinR;
		portReg = portR;
		ddrReg = ddrR;
		mask = m;
		
		lastSampleTime = 0;
		flagPressed = false;
		
		// Lectura inicial del pin para el estado base
		bool initialReading = !(*pinReg & mask);
		stableState = initialReading;
		counter = stableState ? DEBOUNCE_SAMPLES : 0;
	}
	
	void init() {
		*ddrReg &= ~mask;	// Input
		*portReg |= mask;	// Pull-up
	}
	
	void update(uint32_t currentTime) {
		if (currentTime == lastSampleTime) return;
		lastSampleTime = currentTime;
		
		bool reading = !(*pinReg & mask);
		
		if (reading && counter < DEBOUNCE_SAMPLES) counter++;
		else if (!reading && counter > 0)          counter--;
		
		if (!stableState && counter == DEBOUNCE_SAMPLES) {
			stableState = true;
			flagPressed = true;
		}
		else if (stableState && counter == 0) {
			stableState = false;
		}
	}
	
	bool consumeClick() {
		if (flagPressed) {
			flagPressed = false;
			return true;
		}
		return false;
	}
};