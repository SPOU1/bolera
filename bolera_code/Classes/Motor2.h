#pragma once
#include <avr/io.h>
#include "LimitSwitch3.h"

class Motor2 {
	public:
	static const uint32_t ESCAPE_MS = 300; // Escape largo (incondicional)
	static const uint32_t HOMING_HOLD_MS = 250; 
	static const uint8_t DEFAULT_SPEED = 204; // VOLVEMOS A 80% PARA EVITAR BLOQUEO DE DRIVERS
	
	private:
	volatile uint8_t* portDIR;
	volatile uint8_t* ddrDIR;
	uint8_t maskDIR;
	
	volatile uint8_t* portEN;
	volatile uint8_t* ddrEN;
	uint8_t maskEN;
	
	volatile uint8_t* ocrPWM;

	LimitSwitch3* sw; 
	
	bool moving;
	bool direction; // true = hacia LEFT
	bool escaping;
	uint32_t escapeStart;

	LimitSwitch3::Position targetPosition; 
	
	bool isHoming;
	uint32_t homingPressStart;
	LimitSwitch3::Position homingTarget;
	
	public:
	Motor2(volatile uint8_t* pDIR, volatile uint8_t* dDIR, uint8_t mDIR,
           volatile uint8_t* pEN,  volatile uint8_t* dEN,  uint8_t mEN,
           volatile uint8_t* rPWM, LimitSwitch3* swPtr = nullptr) {
		portDIR = pDIR; ddrDIR = dDIR; maskDIR = mDIR;
		portEN  = pEN;  ddrEN  = dEN;  maskEN  = mEN;
		ocrPWM = rPWM;
		sw = swPtr;
		moving = false;
		direction = true;
		escaping = false;
		escapeStart = 0;
		targetPosition = LimitSwitch3::Position::UNKNOWN;
		isHoming = false; homingPressStart = 0; homingTarget = LimitSwitch3::Position::UNKNOWN;
	}
		
	void init() {
		*ddrDIR |= maskDIR;
		*ddrEN |= maskEN;
		*portDIR &= ~maskDIR;
		*portEN &= ~maskEN;
		*ocrPWM = 0;
	}
	
	void startHoming(bool dir, LimitSwitch3::Position target, uint32_t currentTime, uint8_t speed = DEFAULT_SPEED) {
		if (sw == 0) return;

		isHoming = true;
		homingTarget = target;
		homingPressStart = 0;

		// ESCAPE INCONDICIONAL SIEMPRE AL ARRANCAR
		escaping = true;
		escapeStart = currentTime;
		sw->forceReleased();

		if (dir) 	*portDIR |= maskDIR;
		else 		*portDIR &= ~maskDIR;

		*ocrPWM = speed;
        // Eliminado la manipulación manual de portEN. El Timer toma el control.

		direction = dir;
		moving = true;
	}
	
	void move(bool dir, LimitSwitch3::Position target, uint32_t currentTime, uint8_t speed = DEFAULT_SPEED) {
		if (sw == 0) return;

		targetPosition = target;
		LimitSwitch3::Position nextPos = computeNextExpected(dir);
		sw->setNextExpected(nextPos);

		// ESCAPE INCONDICIONAL SIEMPRE AL ARRANCAR
		escaping = true;
		escapeStart = currentTime;
		sw->forceReleased(); 

		if (dir) 	*portDIR |= maskDIR; 
		else 		*portDIR &= ~maskDIR; 

		*ocrPWM = speed;

		direction = dir;
		moving = true;
	}

	void stop() {
		*ocrPWM = 0;
		moving = false;
		escaping = false;
	}

	void update(uint32_t currentTime) {
		if (!moving || sw == 0) return;
		
		if (isHoming) {
			if(escaping) {
				if ((currentTime - escapeStart) >= ESCAPE_MS) {
					escaping = false;
					
					// NUEVO: Si sigue pulsado tras el escape en homing
					if (sw->isPressed()) {
						stop();
						sw->forcePressed(homingTarget);
						targetPosition = homingTarget;
						isHoming = false;
					}
					sw->consumePress();
				}
				return;
			}

			if (sw->isPressed()) {
				if (homingPressStart == 0) {
					homingPressStart = currentTime;
					} else if ((currentTime - homingPressStart) >= HOMING_HOLD_MS) {
					stop();
					sw->forcePressed(homingTarget);
					targetPosition = homingTarget;
					isHoming = false;
				}
				} else {
				homingPressStart = 0;
				sw->consumePress();
			}
			return;
		}

		if(escaping) {
			if ((currentTime - escapeStart) >= ESCAPE_MS) {
				escaping = false;
				
				// NUEVO: Verificación post-escape para movimiento normal
				if (sw->isPressed()) {
					stop();
				}
				sw->consumePress();
			}
			return;
		}

		if (sw->consumePress()) {
			LimitSwitch3::Position arrived = sw->getPosition();
			if (arrived == targetPosition) {
				stop();
				} else {
				LimitSwitch3::Position nextPos = computeNextExpected(direction);
				sw->setNextExpected(nextPos);
				escaping = true;
				escapeStart = currentTime;
			}
		}
	}

	bool isAt(LimitSwitch3::Position pos) {
		if (sw == 0) return false;
		// MEJORA: Un motor está en la posición si no se mueve Y
		// o bien el sensor lo confirma, o bien es nuestra posición objetivo actual.
		return !moving && (sw->getPosition() == pos);
	}

	bool isMoving() {
		return moving;
	}

	private:
	LimitSwitch3::Position computeNextExpected(bool dir) {
        if (sw == nullptr) return LimitSwitch3::Position::UNKNOWN;
        LimitSwitch3::Position current = sw->getPosition();
 
        if (dir) {  
            if (current == LimitSwitch3::Position::RIGHT)  return LimitSwitch3::Position::MIDDLE;
            if (current == LimitSwitch3::Position::MIDDLE) return LimitSwitch3::Position::LEFT;
            return LimitSwitch3::Position::LEFT; 
        } else {    
            if (current == LimitSwitch3::Position::LEFT)   return LimitSwitch3::Position::MIDDLE;
            if (current == LimitSwitch3::Position::MIDDLE) return LimitSwitch3::Position::RIGHT;
            return LimitSwitch3::Position::RIGHT; 
        }
    }
};