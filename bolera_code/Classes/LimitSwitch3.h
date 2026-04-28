#pragma once
#include <avr/io.h>

class LimitSwitch3 {
	public:
	enum class Position : uint8_t {
		 UNKNOWN = 0,    // Estado inicial, no calibrado
		 LEFT = 1,
		 MIDDLE = 2,
		 RIGHT = 3
	};

	static const uint8_t SAMPLES = 10; // Número de muestras para confirmar un cambio

	
    private:
    volatile uint8_t* pinReg;
    volatile uint8_t* portReg;
    volatile uint8_t* ddrReg;
    uint8_t mask;

    uint8_t counter;            // Integrador
    bool stableState;           // Último estado estable confirmado
    bool flagPressed;           // Flanco de subida pendiente de consumir
    uint32_t lastSampleTime;    // Para evitar muestrear más de una vez por ms

    Position currentPosition;       // Posición actual del switch (LEFT, MIDDLE, RIGHT)
    Position nextExpected;			// Próxima posición esperada para validar movimientos

    public:
    LimitSwitch3(volatile uint8_t* pinR, volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
		pinReg = pinR;
		portReg = portR;
		ddrReg = ddrR;
		mask =m;
		
		counter = 0;
		stableState = false;
		flagPressed = false;
		lastSampleTime = 0;
		currentPosition = Position::UNKNOWN;
		nextExpected = Position::UNKNOWN;
	}
    
    void init() {
        *ddrReg &= ~mask; // Input
        *portReg |= mask; // Pull-up
    }
    
    void update(uint32_t currentTime) {
        if (currentTime == lastSampleTime) return; // Evitar muestrear más de una vez por ms
        lastSampleTime = currentTime;

        // Leer estado actual (lógica invertida por pull-up)
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
            currentPosition = nextExpected; // Actualizar posición actual al esperado
        }
    }

    bool isPressed() {
        return stableState;
    }

    bool consumePress() {
        if (flagPressed) {
            flagPressed = false; // Consumir el flanco detectado
            return true;
        }
        return false;
    }

    Position getPosition() {
        return currentPosition;
    }

    void setNextExpected(Position pos) {
        nextExpected = pos;
    }

    void forcePressed(Position pos) {
        counter = SAMPLES; // Forzar estado presionado
        stableState = true;
        flagPressed = false; // No genera evento, ya lo sabemos.
        currentPosition = pos;
        nextExpected = pos; // Sincronizar posición esperada con la forzada
    }

    void forceReleased() {
        counter = 0; // Forzar estado suelto
        stableState = false;
        flagPressed = false; // No hay pulso pendiente
    }
};