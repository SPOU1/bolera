/*
 * PinsManager.h
 * Gestiona los bolos con sistema de antirrebotes integrador y bloqueo de bolos ya caídos.
*/

#pragma once
#include <avr/io.h>

class PinsManager {
    private:
    static const uint8_t NUM_PINS = 6;
    static const uint8_t SAMPLES  = 10;

    uint8_t  counter[NUM_PINS];      // Integrador para cada pin
    bool     stableState[NUM_PINS];  // Último estado estable confirmado
    bool     pinMask[NUM_PINS];      // true = bolo en pie
    uint8_t  score;
    uint32_t lastSampleTime;

    public:
    PinsManager() {
        score          = 0;
        lastSampleTime = 0;
        for (uint8_t i = 0; i < NUM_PINS; i++) {
            counter[i]     = SAMPLES;
            stableState[i] = true;
            pinMask[i]     = true;
        }
    }

    void init() {
        DDRK  &= ~0x3F;  // PK0-PK5 como entradas
        PORTK |=  0x3F;  // Pull-ups

        // PCICR  |= (1 << PCIE2);   // Habilitar PCINT2
        // PCMSK2 |= 0x3F;           // Habilitar PCINT16-PCINT21
    }

    void update(uint32_t currentTime) {
        /*
         * Actualiza el estado de los bolos.
         * Implementa un sistema de antirrebotes integrador: requiere SAMPLES lecturas consecutivas iguales para confirmar un cambio de estado.
         * Bloquea los bolos ya caídos, solo suman una vez.
         */
        if (currentTime == lastSampleTime) return;  // Evitar muestrear más de una vez por ms
        lastSampleTime = currentTime;

        uint8_t currentState = PINK & 0x3F;

        for (uint8_t i = 0; i < NUM_PINS; i++) {
            if (!pinMask[i]) continue; // Bolo ya caído, ignorar

            bool reading = !(currentState & (1 << i));

            // Integrador saturante (antirrebotes)
            if (reading  && counter[i] < SAMPLES) counter[i]++;
            else if (!reading && counter[i] > 0)  counter[i]--;

            // Flanco descendente: bolo se levanta (resetear el estado estable)
            if (stableState[i] && counter[i] == 0) {
                stableState[i] = false;
            }

            // Flanco ascendente confirmado: bolo derribado
            if (!stableState[i] && counter[i] == SAMPLES) {
                stableState[i] = true;
                score++;
                pinMask[i] = false; // Bloquear bolo caído, no contar más
            }
        }
    }

    /*
     * Devuelve la puntuación actual (número de bolos caídos).
     */
    uint8_t getScore() { return score; }

    void reset() {
        score          = 0;
        lastSampleTime = 0;
        for (uint8_t i = 0; i < NUM_PINS; i++) {
            counter[i]     = SAMPLES;
            stableState[i] = true;
            pinMask[i]     = true;
        }
    }
};