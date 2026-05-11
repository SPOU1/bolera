/*
 * SwitchUser.h
 * Gestiona el interruptor de usuario (SW6) con antirrebotes integrador.
 */

#pragma once
#include <avr/io.h>

class SwitchUser {
    private:
    volatile uint8_t* pinReg;
    volatile uint8_t* portReg;
    volatile uint8_t* ddrReg;
    uint8_t mask;
    
    static const uint8_t DEBOUNCE_SAMPLES = 20;
    uint8_t  counter;
    uint32_t lastSampleTime;
    bool stableState;
    bool flagPressed;
    
    public:
    SwitchUser(volatile uint8_t* pinR, volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
        pinReg = pinR;
        portReg = portR;
        ddrReg = ddrR;
        mask = m;
        
        lastSampleTime = 0;
        flagPressed = false;
        
        bool initialReading = !(*pinReg & mask);
        stableState = initialReading;
        counter = stableState ? DEBOUNCE_SAMPLES : 0;
    }
    
    void init() {
        *ddrReg &= ~mask;	// Input
        *portReg |= mask;	// Pull-up
    }
    
    void update(uint32_t currentTime) {
        /*
         * Actualiza el estado del interruptor.
         * Implementa un antirrebotes integrador: requiere DEBOUNCE_SAMPLES lecturas consecutivas iguales para cambiar el estado estable.
         */
        if (currentTime == lastSampleTime) return;  // Evitar muestrear más de una vez por ms
        
        lastSampleTime = currentTime;
        
        bool reading = !(*pinReg & mask);  // Activo bajo: 0 = presionado, 1 = no presionado
        
        // Lógica antirrebotes integrador, aumenta o disminuye el contador según la lectura actual.
        if (reading && counter < DEBOUNCE_SAMPLES) counter++;
        else if (!reading && counter > 0)          counter--;
        
        // no pulsado -> pulsado: estable = pulsado
        if (!stableState && counter == DEBOUNCE_SAMPLES) {
            stableState = true;
            flagPressed = true;
        }

        // pulsado -> no pulsado: estable = no pulsado
        else if (stableState && counter == 0) {
            stableState = false;
        }
    }
    
    bool consumeClick() {
        /*
         * Devuelve true si se ha detectado un nuevo click.
         * El flag se resetea al consumir el click.
         */
        
        if (flagPressed) {
            flagPressed = false;
            return true;
        }
        return false;
    }
};