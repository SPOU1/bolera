#pragma once
#include <avr/io.h>

class PinsManager {
    private:
    static const uint8_t NUM_PINS = 6;
    static const uint8_t SAMPLES  = 10;  // Muestras para confirmar ca�da

    // volatile bool changed;

    uint8_t counter[NUM_PINS];      // Integrador por pin
    bool    stableState[NUM_PINS];  // �ltimo estado estable confirmado
    bool    pinMask[NUM_PINS];      // true = bolo en pie
    uint8_t score;
    uint32_t lastSampleTime;

    public:
    PinsManager() {
        // changed        = false;
        score          = 0;
        lastSampleTime = 0;
        for (uint8_t i = 0; i < NUM_PINS; i++) {
            counter[i]     = SAMPLES;   // Asumimos bolos en pie al inicio
            stableState[i] = true;
            pinMask[i]     = true;
        }
    }

    void init() {
        DDRK  &= ~0x3F;  // PK0�PK5 como entradas
        PORTK |=  0x3F;  // Pull-ups activados

        // PCICR  |= (1 << PCIE2);   // Habilitar grupo PCINT2 (puerto K)
        // PCMSK2 |= 0x3F;           // Habilitar PCINT16�PCINT21
    }

    // // Llamado desde ISR � m�nimo trabajo
    // void onInterrupt() {
    //     changed = true;
    // }

    // Llamado cada ms desde update_sensors() en Status
    void update(uint32_t currentTime) {
        if (currentTime == lastSampleTime) return;
        lastSampleTime = currentTime;

        // Solo muestreamos si la ISR detect� actividad reciente.
        // No bloqueamos el muestreo: seguimos integrando hasta estabilizar.
        uint8_t currentState = PINK & 0x3F;

        for (uint8_t i = 0; i < NUM_PINS; i++) {
            if (!pinMask[i]) continue; // Bolo ya ca�do, ignorar

            // L�gica invertida por pull-up: 0 en el pin = pulsado = bolo ca�do
            bool reading = !(currentState & (1 << i));

            // Integrador saturante
            if (reading  && counter[i] < SAMPLES) counter[i]++;
            else if (!reading && counter[i] > 0)  counter[i]--;

            // Flanco descendente: bolo se levanta (no deber�a pasar en juego,
            // pero reseteamos para no quedarnos en estado inconsistente)
            if (stableState[i] && counter[i] == 0) {
                stableState[i] = false;
            }

            // Flanco ascendente: bolo derribado
            if (!stableState[i] && counter[i] == SAMPLES) {
                stableState[i] = true;
                score++;
                pinMask[i] = false; // Bloqueamos: este bolo ya no suma m�s
            }
        }

        // changed = false;
    }

    uint8_t getScore() { return score; }

    bool isPinUp(uint8_t i) {
		// Retorna true si el bit 'i' en pinsMask es 1 (bolo en pie)
		return pinMask[i];
	}

    void reset() {
        score          = 0;
        // changed        = false;
        lastSampleTime = 0;
        for (uint8_t i = 0; i < NUM_PINS; i++) {
            counter[i]     = SAMPLES;
            stableState[i] = true;
            pinMask[i]     = true;
        }
    }
};