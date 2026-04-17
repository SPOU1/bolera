#pragma once

#include <avr/io.h>
#include <util/delay.h>
#include "SwitchMotor.h"

class Motor {
	private:
	volatile uint8_t* portDIR;
	volatile uint8_t* ddrDIR;
	uint8_t maskDIR;
	
	volatile uint8_t* portEN;
	volatile uint8_t* ddrEN;
	uint8_t maskEN;
	
	volatile uint8_t* ocrPWM;
	
	// Puntero a clase Switch
	SwitchMotor* eorSwitch;
	
	// Variables de estado
	bool isMoving;
	bool isEscaping;
	uint16_t escapeCounter;
	
	public:
	Motor(
		volatile uint8_t* pDIR, volatile uint8_t* dDIR, uint8_t mDIR,
		volatile uint8_t* pEN,  volatile uint8_t* dEN,  uint8_t mEN,
		volatile uint8_t* rPWM,
		SwitchMotor* sw = 0
	) {
		portDIR = pDIR; ddrDIR = dDIR; maskDIR = mDIR;
		portEN  = pEN;  ddrEN  = dEN;  maskEN  = mEN;
		ocrPWM  = rPWM;
		eorSwitch = sw;
		
		isMoving = false;
		isEscaping = false;
		escapeCounter = 0;
	}
	
	void init() {
		// Timer tiene que ser inicializado antes
	
		*ddrDIR |= maskDIR;	// Output
		*ddrEN |= maskEN;	// Output
		*portDIR &= ~maskDIR;
		*portEN &= ~maskEN;
		
		*ocrPWM = 0;
	}
	
	void setDirection(bool forward) {
		if (forward)	*portDIR |= maskDIR;
		else			*portDIR &= ~maskDIR;
	}
	
	void setSpeed(uint8_t speed) {
		*ocrPWM = speed;
	}
	
	void stop() {
		setSpeed(0);
		*portEN &= ~maskEN;
		isMoving = false;
	}
	
	bool getIsMoving() {
		return isMoving;
	}
	
	void calibrate(bool direction) {
		goTo(direction);
		
		if(isMoving) {
			setSpeed(153);
		}
		
		uint32_t timeout = 4000000;
		while (getIsMoving() && timeout > 0) {
			update();
			timeout--;
		}
		
		stop();
		_delay_ms(200);
		
		if (eorSwitch != 0) {
			if(direction)	eorSwitch->forceState(SwitchMotor::SIDE_1);
			else			eorSwitch->forceState(SwitchMotor::SIDE_2);
		}
		
	}
	
	void goTo(bool direction) {
		bool startFromSide = false;
	
		if (eorSwitch != 0) {
			SwitchMotor::SwState state = eorSwitch->getState();
			if ((direction && state == SwitchMotor::SIDE_1) ||
				(!direction && state == SwitchMotor::SIDE_2)) {
				return;
			}
			if (state != SwitchMotor::MOVING) {
				startFromSide = true;
			}
			
			eorSwitch->forceState(SwitchMotor::MOVING);
			eorSwitch->setExpectedDirection(direction);
		}
		
		setDirection(direction);
		setSpeed(204);
		isMoving = true;
		
		if (startFromSide || (eorSwitch != 0 && eorSwitch->isPressed())) {	
			isEscaping = true;
			escapeCounter = 0;
		} else {
			isEscaping = false;
		}
	}
	
	// Se debe llamar cada iteración
	void update() {
		if (eorSwitch != 0 && isMoving) {
			if (isEscaping) {
				if (!eorSwitch->isPressed()) {
					escapeCounter++;
					if (escapeCounter > 20000) {
						isEscaping = false;
						escapeCounter = 0;
						
						eorSwitch->consumePressedFlag();
					}
				} else {
					escapeCounter = 0;
				}
				
			} else {
				if (eorSwitch->consumePressedFlag()) {
					stop();
				}
			}
		}
	}
};