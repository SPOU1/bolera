/* 
 * Motor2.h
 * Gestiona el motor 2 con sistema de homing y escape.
 * Se asocia a un LimitSwitch3 para control de posición.
 */

#pragma once
#include <avr/io.h>
#include "LimitSwitch3.h"

class Motor2 {
    public:
    static const uint32_t ESCAPE_MS      = 300;
    static const uint32_t HOMING_HOLD_MS = 250;
    static const uint8_t  DEFAULT_SPEED  = 204; // 80%

    private:
    volatile uint8_t* portDIR;
    volatile uint8_t* ddrDIR;
    uint8_t maskDIR;

    volatile uint8_t* portEN;
    volatile uint8_t* ddrEN;
    uint8_t maskEN;

    volatile uint8_t* ocrPWM;

    LimitSwitch3* sw;

    bool moving;
    bool direction; // true = hacia LEFT
    bool escaping;
    uint32_t escapeStart;

    LimitSwitch3::Position targetPosition;

    bool isHoming;
    uint32_t homingPressStart;
    LimitSwitch3::Position homingTarget;

    public:
    Motor2(volatile uint8_t* pDIR, volatile uint8_t* dDIR, uint8_t mDIR,
           volatile uint8_t* pEN,  volatile uint8_t* dEN,  uint8_t mEN,
           volatile uint8_t* rPWM, LimitSwitch3* swPtr = nullptr) {
        portDIR   = pDIR; ddrDIR = dDIR; maskDIR = mDIR;
        portEN    = pEN;  ddrEN  = dEN;  maskEN  = mEN;
        ocrPWM    = rPWM;
        sw        = swPtr;
        moving    = false;
        direction = true;
        escaping  = false;
        escapeStart = 0;
        targetPosition = LimitSwitch3::Position::UNKNOWN;
        isHoming = false; homingPressStart = 0; homingTarget = LimitSwitch3::Position::UNKNOWN;
    }

    void init() {
        *ddrDIR  |= maskDIR;
        *ddrEN   |= maskEN;
        *portDIR &= ~maskDIR;
        *portEN  &= ~maskEN;
        *ocrPWM   = 0;
    }

    void startHoming(bool dir, LimitSwitch3::Position target, uint32_t currentTime, uint8_t speed = DEFAULT_SPEED) {
        /*
         * Inicia el proceso de homing.
         * El motor se mueve en la dirección indicada hasta que el switch confirma la posición objetivo.
         */
        if (sw == 0) return;

        isHoming = true;
        homingTarget = target;
        homingPressStart = 0;

        // Escape al arrancar homing
        escaping = true;
        escapeStart = currentTime;
        sw->forceReleased();

        if (dir) 	*portDIR |= maskDIR;
        else 		*portDIR &= ~maskDIR;

        *ocrPWM = speed;

        direction = dir;
        moving = true;
    }

    void move(bool dir, LimitSwitch3::Position target, uint32_t currentTime, uint8_t speed = DEFAULT_SPEED) {
        /*
         * Inicia un movimiento hacia la posición objetivo.
         * El motor se mueve en la dirección indicada hasta que el switch confirma la posición objetivo.
         * Implementa escape al iniciar el movimiento para evitar falsos positivos en el switch.
         */
        
        if (sw == 0) return;

        targetPosition = target;
        LimitSwitch3::Position nextPos = computeNextExpected(dir);
        sw->setNextExpected(nextPos);

        // Escape al iniciar movimiento (ignorar switch durante ESCAPE_MS)
        escaping = true;
        escapeStart = currentTime;
        sw->forceReleased();

        if (dir) *portDIR |= maskDIR;
        else     *portDIR &= ~maskDIR;

        *ocrPWM = speed;

        direction = dir;
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

        // HOMING
        if (isHoming) {
            // Si estamos en escape, esperamos a que termine sin procesar el switch
            if(escaping) {
                if ((currentTime - escapeStart) >= ESCAPE_MS) {
                    escaping = false;
                    sw->consumePress(); // Limpiamos flancos residuales
                }
                return;
            }

            // Si no estamos en escape, procesamos el switch
            if (sw->isPressed()) {
                // Para evitar falsos positivos, mantener presionado durante HOMING_HOLD_MS
                if (homingPressStart == 0) {
                    homingPressStart = currentTime;
                }

                // Si se mantiene presionado HOMING_HOLD_MS, confirmamos homing
                else if ((currentTime - homingPressStart) >= HOMING_HOLD_MS) {
                    stop();
                    sw->forcePressed(homingTarget);
                    targetPosition = homingTarget;
                    isHoming = false;
                }
            }
            else {
                homingPressStart = 0;   // Resetear temporizador si se suelta el switch
                sw->consumePress();     // Limpiamos flancos residuales
            }
            return;
        }

        // MOVIMIENTO NORMAL
        // Si estamos en escape, esperamos a que termine sin procesar el switch
        if(escaping) {
            // Si el tiempo de escape ha terminado, verificamos el switch
            if ((currentTime - escapeStart) >= ESCAPE_MS) {
                escaping = false;
                if (sw->isPressed()) {
                    stop();
                }
                sw->consumePress(); // Limpiar flancos residuales
            }
            return;
        }

        // Si no estamos en escape, procesamos el switch
        if (sw->consumePress()) {
            // Verificar si posición actual es igual a posición objetivo
            LimitSwitch3::Position arrived = sw->getPosition();
            if (arrived == targetPosition) {
                stop();
            }
            
            else {
                // Si no hemos llegado, actualizamos la posición esperada para el próximo flanco
                LimitSwitch3::Position nextPos = computeNextExpected(direction);
                sw->setNextExpected(nextPos);
                escaping = true;
                escapeStart = currentTime;
            }
        }
    }

    bool isAt(LimitSwitch3::Position pos) {
        // Devuelve true si motor detenido y posición correcta.
        if (sw == 0) return false;
        return !moving && (sw->getPosition() == pos);
    }

    bool isMoving() {
        return moving;
    }

    private:
    LimitSwitch3::Position computeNextExpected(bool dir) {
        /*
         * Calcula la próxima posición esperada según la dirección del movimiento y la posición actual del switch.
         */
        if (sw == nullptr) return LimitSwitch3::Position::UNKNOWN;
        LimitSwitch3::Position current = sw->getPosition();

        if (dir) {
            if (current == LimitSwitch3::Position::RIGHT)  return LimitSwitch3::Position::MIDDLE;
            if (current == LimitSwitch3::Position::MIDDLE) return LimitSwitch3::Position::LEFT;
            return LimitSwitch3::Position::LEFT;
        } else {
            if (current == LimitSwitch3::Position::LEFT)   return LimitSwitch3::Position::MIDDLE;
            if (current == LimitSwitch3::Position::MIDDLE) return LimitSwitch3::Position::RIGHT;
            return LimitSwitch3::Position::RIGHT;
        }
    }
};