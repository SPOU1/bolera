#pragma once
#include <avr/io.h>
#include "LimitSwitch.h"

class Motor {
	public:
	enum class Side : uint8_t {
		UNKNOWN = 0,
		A = 1,
		B = 2
	};

	static const uint8_t DEFAULT_SPEED = 204; 

	private:
	volatile uint8_t* portDIR;
	volatile uint8_t* ddrDIR;
	uint8_t maskDIR;
	
	volatile uint8_t* portEN;
	volatile uint8_t* ddrEN;
	uint8_t maskEN;
	
	volatile uint8_t* ocrPWM; 
	
	LimitSwitch* sw;

	bool moving;
	bool direction; 
	Side lastSide; 

	bool escaping;
	uint32_t escapeStart;
	uint32_t escapeTimeMs = 300;

	public:
    Motor(volatile uint8_t* pDIR, volatile uint8_t* dDIR, uint8_t mDIR,
          volatile uint8_t* pEN,  volatile uint8_t* dEN,  uint8_t mEN,
          volatile uint8_t* rPWM, LimitSwitch* swPtr = nullptr) {
		portDIR = pDIR;	ddrDIR = dDIR; maskDIR = mDIR;
		portEN	= pEN;	ddrEN  = dEN;  maskEN  = mEN;
		ocrPWM = rPWM;
		sw = swPtr;
		moving = false;
		direction = true;
		lastSide = Side::UNKNOWN;
		escaping = false;
		escapeStart = 0;
	}

	void init() {
		*ddrDIR |= maskDIR;
		*ddrEN |= maskEN;
		*portDIR &= ~maskDIR;
		*portEN &= ~maskEN;
		*ocrPWM = 0;
	}

	void setEscapeTime(uint32_t ms) {
		escapeTimeMs = ms;
	}

	void move(bool dir, uint32_t currentTime, uint8_t speed = DEFAULT_SPEED) {
		Side targetSide = dir ? Side::A : Side::B;
		if (lastSide == targetSide) return;

		// ESCAPE INCONDICIONAL SIEMPRE AL ARRANCAR (Evita falsos positivos por vibración)
		escaping = true;
		escapeStart = currentTime;
		if(sw != 0) sw->forceReleased();

		if (dir) *portDIR |= maskDIR; 
		else     *portDIR &= ~maskDIR; 

		*ocrPWM = speed;

		direction = dir;
		lastSide = Side::UNKNOWN; 
		moving = true;
	}

	void stop() {
		*ocrPWM = 0;
		moving = false;
		escaping = false;
	}

	void update(uint32_t currentTime) {
		if (!moving || sw == 0) return;
		
		if(escaping) {
			if ((currentTime - escapeStart) >= escapeTimeMs) {
				escaping = false;
				
				// NUEVO: Si después del escape el sensor sigue pulsado,
				// significa que ya estamos en el tope. Paramos.
				if (sw->isPressed()) {
					stop();
					lastSide = direction ? Side::A : Side::B;
				}
				
				sw->consumePress(); // Limpiar el pulso del arranque
			}
			return;
		}

		if (sw->consumePress()) {
			stop();
			lastSide = direction ? Side::A : Side::B;
		}
	}
	
	bool isMoving() {
		return moving;
	}

	bool isAt(Side side) {
		return !moving && lastSide == side;
	}

	void forceAt(Side side) {
		moving = false;
		escaping = false;
		lastSide = side;
	}
};