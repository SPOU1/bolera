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
	uint32_t escapeStartTimestamp;
	const uint32_t escapeDelay = 100;
	
	bool isCalibrating;
	bool isWaitingAfterCalib;
	uint32_t stateTimer;
	bool calibDirection;
	
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
		isCalibrating = false;
		isWaitingAfterCalib = false;
	}
	
	void init() {
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
	
	void calibrate(bool direction, uint32_t currentTime) {
		goTo(direction, currentTime);
		
		if(isMoving) {
			setSpeed(153);
		}
		
		isCalibrating = true;
		isWaitingAfterCalib = false;
		calibDirection = direction;
		stateTimer = currentTime;		
	}
	
	void goTo(bool direction, uint32_t currentTime) {
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
			escapeStartTimestamp = currentTime;
		} else {
			isEscaping = false;
		}
	}
	
	void update(uint32_t currentTime) {
		// post Calibration pause (200ms)
		if (isWaitingAfterCalib) {
			if ((currentTime - stateTimer) >= 200) {
				isWaitingAfterCalib = false;
				if (eorSwitch != 0) {
					if (calibDirection)	eorSwitch->forceState(SwitchMotor::SIDE_1);
					else				eorSwitch->forceState(SwitchMotor::SIDE_2);
				}	
			}
			return;
		}
		
		// Normal control
		if (eorSwitch != 0 && isMoving) {
			
			if (isEscaping) {
				if (!eorSwitch->isPressed()) {
					if((currentTime - escapeStartTimestamp) > escapeDelay) {
						isEscaping = false;
						eorSwitch->consumePressedFlag();
					}
				} else {
					escapeStartTimestamp = currentTime;
				}
			} else {
				if(eorSwitch->consumePressedFlag()) {
					stop();
				
					if (isCalibrating) {
						isCalibrating = false;
						isWaitingAfterCalib = true;
						stateTimer = currentTime;
					}
				}
				// Safety timeout (10s)
				else if (isCalibrating && (currentTime - stateTimer) > 10000) {
					stop();
					isCalibrating = false;
				}
			}
		}
	}
};