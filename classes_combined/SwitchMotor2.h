#pragma once
#include <avr/io.h>

class SwitchMotor2 {
	/*
	 * ==== SwitchMotor2 ====
	 * Gestiona switch fin de carrera con filtro antirebote por software y tracking de 3 posiciones.
	 */
	public:
	enum SwState {
		MOVING = 0,		// Entre posiciones
		SIDE_1 = 1,		// Extremo 1
		SIDE_2 = 2,		// Extremo 2
		SIDE_MIDDLE = 3	// Posición intermedia
		};
	
	private:
	volatile uint8_t* pinReg;
	volatile uint8_t* portReg;
	volatile uint8_t* ddrReg;
	uint8_t mask;
	
	volatile SwState currentState;
	volatile SwState nextExpectedState;	// Guarda a qué estado pasará en el próximo click.
	volatile bool flagPressed;
	bool expectedDirection;	// Indica hacia que lado se está moviendo el motor.
	
	uint32_t lastDebounceTimestamp;
	bool lastReading;
	bool stableState;
	const uint32_t debounceDelay = 20;	//ms de espera para ignorar ruido mecánico.
	
	public:
	SwitchMotor(volatile uint8_t* pinR, volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
		pinReg  = pinR;
		portReg = portR;
		ddrReg  = ddrR;
		mask    = m;
		currentState = MOVING;
		nextExpectedState = SIDE_1;
		flagPressed = false;
		expectedDirection = false;
		
		lastDebounceTimestamp = 0;
		lastReading = false;
		stableState = false;
	}
	
	void init() {
		/* 
		 * ==== init() ====
		 * Configura el pin como entrada y activa resistencia pull-up interna.
		 */
		*ddrReg &= ~mask;	// Input
		*portReg |= mask;	// Pull-up
	}
	
	bool isPressed() {
		/* 
		 * ==== isPressed() ====
		 * Devuelve el estado acutal de la señal del switch.
		 *  - return: [bool] true si el switch está presionado. flase en caso contrario.
		 */
		return !(*pinReg & mask);	// Lógica invertida (Pull-up)
	}

	SwState getState() {
		/* 
		 * ==== getState() ====
		 * Devuelve el estado acutal del actuador tras el filtrado.
		 *  - return: [SwState] Estado relativo en el que está el actuador.
		 */
		return currentState;
	}
	
	void update(uint32_t currentTime) {
		/* 
		 * ==== update(uint32_t currentTime) ====
		 * Actualiza el estado del switch. Debe llamarse en el loop principal.
		 *  - currentTime: [uint32_t] Tiempo actual en ms.
		 */
		
		bool reading = isPressed();
		
        if (reading != lastReading) {
			lastDebounceTimestamp = currentTime;
		}
		
		if ((currentTime - lastDebounceTimestamp) > debounceDelay) {
			if (reading != stableState) {
				stableState = reading;
				
				if (stableState) {
					flagPressed = true;
					currentState = nextExpectedState;
				}
			}
		}
		lastReading = reading;
    }
	
	void forceState(SwState s) {
		/* 
		 * ==== forceState(SwState s) ====
		 * Establece el estado del actuador. Debe ser usado tras la calibración.
		 *  - s: [SwState] Estado actual del actuador.
		 */
		currentState = s;
		flagPressed = false;
	}
	
	void setExpectedDirection(bool direction) {
		/* 
		 * ==== setExpectedDirection(bool direction) ====
		 * Establece la dirección y prepara el próximo estado lógico a alcanzar.
		 *  - direction: [bool] true si la dirección es hacia SIDE_1, false en caso contrario.
		 */
		expectedDirection = direction;
		flagPressed = false;
		
		// Lógica de 3 posiciones
		if (direction) { // hacia SIDE_1
			if (currentState == SIDE_2) nextExpectedState = SIDE_MIDDLE;
			else if (currentState == SIDE_MIDDLE) nextExpectedState = SIDE_1;
			else nextExpectedState = SIDE_1;
		} else {
			if (currentState == SIDE_1) nextExpectedState = SIDE_MIDDLE;
			else if (currentState == SIDE_MIDDLE) nextExpectedState = SIDE_2;
			else nextExpectedState = SIDE_2;
		}
		
		currentState = MOVING
	}
	
	bool consumePressedFlag() {
		/* 
		 * ==== consumePressedFlag() ====
		 * Comprueba si se pulsó el switch y limpia la bandera.
		 *  - return: [bool] true si hubo un click no procesado, false en caso contrario.
		 */
		if (flagPressed) {
			flagPressed = false;
			return true;
		}
		return false;
	}
};