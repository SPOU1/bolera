/*
 * LimitSwitch.h
 * Gestiona el los switches asociados a los motores con sistema de antirrebotes integrador.
 */

#pragma once
#include <avr/io.h>

class LimitSwitch {
	public:
	static const uint8_t SAMPLES = 10; // Número de muestras para confirmar un cambio
	
    private:
    volatile uint8_t* pinReg;
    volatile uint8_t* portReg;
    volatile uint8_t* ddrReg;
    uint8_t mask;

    uint8_t counter;    // Integrador
    bool stableState;   // Último estado estable confirmado
    bool flagPressed;   // Flanco de subida pendiente de consumir
	uint32_t lastSampleTime;

    public:
    LimitSwitch(volatile uint8_t* pinR, volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
		pinReg  = pinR;
		portReg = portR;
		ddrReg  = ddrR;
		mask    = m;
		
		counter = 0;
		stableState = false;
		flagPressed = false;
		lastSampleTime = 0;
	}
    
    void init() {
        *ddrReg  &= ~mask; // Input
        *portReg |=  mask; // Pull-up
    }
    
    void update(uint32_t currentTime) {
        /*
         * Actualiza el estado del switch.
         * Implementa un sistema de antirrebotes integrador: requiere SAMPLES lecturas consecutivas iguales para cambiar el estado estable.
         */
        if (currentTime == lastSampleTime) return; // Evitar muestrear más de una vez por ms
        lastSampleTime = currentTime;

        bool reading = !(*pinReg & mask);

        // Integrador saturante
        if(reading && counter < SAMPLES) counter++;
        else if(!reading && counter > 0) counter--;

        // Flanco descendente (pulsado->suelto)
        if (stableState && counter == 0) {
            stableState = false;
        }

        // Flanco ascendente (suelto->pulsado)
        if (!stableState && counter == SAMPLES) {
            stableState = true;
            flagPressed = true; // Marcar que se ha detectado un nuevo pulso
        }
    }

    bool isPressed() {
        // Devuelve true si se ha detectado un nuevo pulso.
        // No se resetea el flag
        return stableState;
    }

    bool consumePress() {
        /*
         * Devuelve true si se ha detectado un nuevo pulso y resetea el flag.
         */
        if (flagPressed) {
            flagPressed = false; // Consumir el flanco detectado
            return true;
        }
        return false;
    }

    // void forcePressed() {
    //     counter     = SAMPLES; // Forzar estado presionado
    //     stableState = true;
    //     flagPressed = false;   // No genera evento
    // }

    void forceReleased() {
        counter     = 0;     // Forzar estado suelto
        stableState = false;
        flagPressed = false;
    }
};