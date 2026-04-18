#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>

class Timer {
	/*
	 * ==== Timer ====
	 * Gestiona el reloj global del sistema (System Tick) utiliando el Timer 4.
	 * Propociona una base de tiempo en milisegundos de forma no bloqueante.
	 */
	
	private:
	volatile uint32_t system_millis; // Contador de milisegundos desde el inicio.
	
	public:
	Timer() {
		system_millis = 0;
	}
	
	void init() {
		/* 
		 * ==== init() ====
		 * Inicializa el Timer 4 en modo CTC para interrumpir cada 1ms.
		 * Asume frecuencia de reloj (F_CPU) de 8MHz.
		 */
		
		// Modo CTC (Clear Timer on Compare Match)
		TCCR4B &= ~((1 << WGM43) | (1 << WGM41) | (1 << WGM40));
		TCCR4B |= (1 << WGM42);

		// Preescalador a 64 (8MHz / 64 = 125kHz)
		TCCR4B |= (1 << CS41) | (1 << CS40);
		TCCR4B &= ~(1 << CS42);

		// Interrupción en 1ms (125000 / 1000 -1 = 124 ticks)
		OCR4A = 124;
		
		// Habilitar interrupción por comparación A.
		TIMSK4 |= (1 << OCIE4A);
	}
	
	void addTick() {
		/* 
		 * ==== addTick() ====
		 * Incrementa el reloj. Debe ser llamado exclusivamente durante la ISR.
		 */
		system_millis++;
	}
	
	uint32_t millis() {
		/* 
		 * ==== millis() ====
		 * Devuelve el tiempo actual en ms.
		 *  - return: [uint32_t] Milisegundos transcurridos desde inicio.
		 */
		uint32_t time;
		uint8_t oldSREG = SREG;		// Guarda estado acutal de las interrupciones
		cli();						// Deshabilita interrupciones para lectura
		time = system_millis;
		SREG = oldSREG;				// Restaura el registro de estado original
		// sei();					// Nota: SREG ya restaurado; cli() no es necesario.
		return time;
	}
};
