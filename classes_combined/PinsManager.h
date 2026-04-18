#pragma once
#include <avr/io.h>

class PinsManager {
	/*
	 * ==== PinsManager ====
	 * Gestiona la deteción óptica de los bolos mediante interrupciones PCINT.
	 */
	private:
	uint8_t pinsMask;	// Bolos en pie
	uint8_t score;		// Puntuación acumulada
	
	public:
	PinsManager() {
		pinsMask = 0b00111111;
		score = 0;
	}
	
	void init() {
		/*
		 * ==== init() ====
		 * Habilita los sensores y las interrupciones
		 */
		DDRK &= ~0x3F;	// 0b11000000 (entradas)
		PORTK |= 0x3F;	// Pull-ups
		
		PCICR |= (1 << PCIE2);	// Habilitar interrupciones PCINT2
		PCMSK2 |= 0x3F;			// Aplicar máscara a 6 primeros pines
	}
	
	void onInterrupt() {
		/*
		 * ==== onInterrupt() ====
		 * Función ejecutada por ISR(PCINT2_vect).
		 * Emplea máscara para evitar múltiples detecciones de los bolos.
		 */
		uint8_t pinsState = PINK & 0x3F;
		
		for (int i=0; i<6; i++) {
			// Si no se detecta bolo y en máscara está disponible
			if (!(pinsState & (1<<i)) && (pinsMask & (1<<i))) {
				score++;
				pinsMask &= ~(1<<i);	// Marcamos bolo como caido
			}
		}
	}
	
	uint8_t getScore() {return score;}
		
	void reset() {
		pinsMask = 0b00111111;
		score = 0;
	}
};