#pragma once
#include <avr/io.h>

class SwitchUser {
	/*
	 * ==== SwitchUser ====
	 * Gestiona el Switch 6 con filtro antirebote por software.
	 * Opera mediante consulta periódica no bloqueante.
	 */
	
	private:
	volatile uint8_t* pinReg;
	volatile uint8_t* portReg;
	volatile uint8_t* ddrReg;
	uint8_t mask;
	
	uint32_t lastChangeTimestamp;
	const uint32_t debounceDelay = 50; //ms de espera para ignorar ruido mecánico.
	
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
		/* 
		 * ==== init() ====
		 * Configura el pin como entrada y activa resistencia pull-up interna.
		 */
		*ddrReg &= ~mask;	// Input
		*portReg |= mask;	// Pull-up
	}
	
	void update(uint32_t currentTime) {
		/* 
		 * ==== update(uint32_t currentTime) ====
		 * Actualiza el estado del switch. Debe llamarse en el loop principal.
		 *  - currentTime: [uint32_t] Tiempo actual en ms.
		 */
		bool reading = !(*pinReg & mask); // Lógica invertida por pull-up.
		
		if (reading != lastState) {
			lastChangeTimestamp = currentTime; // Reinicia temporizador de estabilización.
		}
		
		// Si la señal ha estado estable más tiempo que el delay
		if ((currentTime - lastChangeTimestamp) > debounceDelay) {
			if (reading != stableState) {
				stableState = reading;
				if (stableState) {
					flagPressed = true; // Registra la pulsación
				}
			}
		}
		lastState = reading;
	}
	
	bool consumeClick() {
		/* 
		 * ==== consumeClick() ====
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
