/*
 * Timer.h
 * Gestiona el reloj global del sistema (Timer 4).
 * Imita la funcionalidad millis() de Arduino.
 */

#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>

class Timer {
    private:
    volatile uint32_t system_millis; // Contador de milisegundos desde el inicio.

    public:
    Timer() {
        system_millis = 0;
    }

    void init() {
        /* 
         * Inicializa el Timer 4 en modo CTC para interrumpir cada 1ms.
         */
        
        // Modo CTC (Clear Timer on Compare Match)
        TCCR4A &= ~((1 << WGM41) | (1 << WGM40));
        TCCR4B &= ~(1 << WGM43);
        TCCR4B |= (1 << WGM42);

        // Preescalador a 64
        TCCR4B |= (1 << CS41) | (1 << CS40);
        TCCR4B &= ~(1 << CS42);

        // Interrupción en 1ms (125000 / 1000 -1 = 124 ticks)
        OCR4A = 124;

        // Habilitar interrupción OCR4A.
        TIMSK4 |= (1 << OCIE4A);
    }

    void addTick() {
        /* 
         * Incrementa el contador de milisegundos.
         * Uso exclusivo en ISR.
         */
        system_millis++;
    }

    uint32_t millis() {
        /* 
         * Devuelve el tiempo actual en ms (uint32_t).
         */
        uint32_t time;
        uint8_t oldSREG = SREG;		// Guarda estado acutal de las interrupciones
        cli();						// Deshabilita interrupciones para lectura
        time = system_millis;
        SREG = oldSREG;				// Restaura el registro de estado original
        // sei();					// SREG ya restaurado; cli() no es necesario.
        return time;
    }
};
