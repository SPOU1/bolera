/*
 * Motor.h
 * Gestiona los motores (1,3,4,5) con sistema de homing y escape.
 * Se asocia a un LimitSwitch para control de posición.
 */

#pragma once
#include <avr/io.h>
#include "LimitSwitch.h"

class Motor {
    public:
    // Definición de lados del motor para control de posición
    enum class Side : uint8_t {
        UNKNOWN = 0,
        A       = 1,
        B       = 2
    };

    static const uint8_t DEFAULT_SPEED = 204;

    private:
    volatile uint8_t* portDIR;
    volatile uint8_t* ddrDIR;
    uint8_t maskDIR;
    
    volatile uint8_t* portEN;
    volatile uint8_t* ddrEN;
    uint8_t maskEN;
    
    volatile uint8_t* ocrPWM; 
    
    LimitSwitch* sw;

    bool moving;
    bool direction;
    Side lastSide;

    bool     escaping;
    uint32_t escapeStart;
    uint32_t escapeTimeMs = 300;

    public:
    Motor(volatile uint8_t* pDIR, volatile uint8_t* dDIR, uint8_t mDIR,
          volatile uint8_t* pEN,  volatile uint8_t* dEN,  uint8_t mEN,
          volatile uint8_t* rPWM, LimitSwitch* swPtr = nullptr) {
        portDIR   = pDIR; ddrDIR = dDIR; maskDIR = mDIR;
        portEN    = pEN;  ddrEN  = dEN;  maskEN  = mEN;
        ocrPWM    = rPWM;
        sw        = swPtr;
        moving    = false;
        direction = true;
        lastSide  = Side::UNKNOWN;
        escaping  = false;
        escapeStart = 0;
    }

    void init() {
        *ddrDIR  |= maskDIR;
        *ddrEN   |= maskEN;
        *portDIR &= ~maskDIR;
        *portEN  &= ~maskEN;
        *ocrPWM   = 0;
    }

    void setEscapeTime(uint32_t ms) {
        // Actualiza el tiempo de escape.
        escapeTimeMs = ms;
    }

    void move(bool dir, uint32_t currentTime, uint8_t speed = DEFAULT_SPEED) {
        /*
         * Inicia un movimiento hacia el lado indicado.
         * El motor se mueve en la dirección indicada hasta que el switch confirma la posición objetivo.
         * Implementa escape al iniciar el movimiento para evitar falsos positivos en el switch.
         */

        // if (sw == 0) return;

        Side targetSide = dir ? Side::A : Side::B;
        if (lastSide == targetSide) return;

        // Escape al iniciar movimiento (ignorar switch durante escapeTimeMs)
        escaping = true;
        escapeStart = currentTime;
        if(sw != 0) sw->forceReleased();

        if (dir) *portDIR |= maskDIR; 
        else     *portDIR &= ~maskDIR; 

        *ocrPWM = speed;

        direction = dir;
        lastSide = Side::UNKNOWN; 
        moving = true;
    }

    void stop() {
        *ocrPWM = 0;
        moving = false;
        escaping = false;
    }

    void update(uint32_t currentTime) {
        /*
        * Actualiza el estado del motor en función del tiempo y el estado del switch.
        * Implementa lógica de escape para evitar falsos positivos en los switches.
        */
        if (!moving || sw == 0) return;
        
        // Si estamos en escape, esperamos a que termine sin procesar el switch
        if(escaping) {
            if ((currentTime - escapeStart) >= escapeTimeMs) {
                escaping = false;
                if (sw->isPressed()) {
                    stop();
                    lastSide = direction ? Side::A : Side::B;
                }
                sw->consumePress(); // Limpiar flancos residuales
            }
            return;
        }

        // Si no estamos en escape, procesamos el switch normalmente
        if (sw->consumePress()) {
            stop();
            lastSide = direction ? Side::A : Side::B;
        }
    }
    
    bool isMoving() {
        return moving;
    }

    bool isAt(Side side) {
        // Devuelve true si motor detenido y lado correcto.
        // if (sw == 0) return false;
        return !moving && lastSide == side;
    }

    // void forceAt(Side side) {
    //     moving = false;
    //     escaping = false;
    //     lastSide = side;
    // }
};