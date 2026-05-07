#pragma once
#include "Classes/Motor.h"
#include "Classes/Motor2.h"
#include "Classes/LimitSwitch3.h"

/*
 * ============================================================
 *  motor_dirs.h  —  Constantes de dirección y posición
 * ============================================================
 *
 *  Traduce los nombres físicos de la maqueta (ARRIBA, ABAJO,
 *  ABIERTO, CERRADO...) a los tipos que usan Motor y Motor2.
 *
 *  Para Motor (2 posiciones):
 *    MX_DIR_YYY  → bool pasado a Motor::move()
 *    MX_SIDE_YYY → Motor::Side pasado a Motor::isAt() / Motor::forceAt()
 *
 *  Para Motor2 (3 posiciones):
 *    M2_DIR_YYY  → bool pasado a Motor2::move()
 *    M2_POS_YYY  → LimitSwitch3::Position para destino y consultas
 *
 *  Convención de lados:
 *    Side::A = extremo de la dirección true
 *    Side::B = extremo de la dirección false
 * ============================================================
 */

// --- M1: Elevador de carga ---
//  true  = subir  → llega a ARRIBA  (Side::A)
//  false = bajar  → llega a ABAJO   (Side::B)
#define M1_DIR_UP     true
#define M1_DIR_DOWN   false
static const Motor::Side M1_SIDE_UP   = Motor::Side::A;
static const Motor::Side M1_SIDE_DOWN = Motor::Side::B;

// --- M2: Sistema lanzador (3 posiciones) ---
//  true  = hacia izquierda
//  false = hacia derecha
#define M2_DIR_LEFT   true
#define M2_DIR_RIGHT  false
static const LimitSwitch3::Position M2_POS_LEFT   = LimitSwitch3::Position::LEFT;
static const LimitSwitch3::Position M2_POS_MIDDLE = LimitSwitch3::Position::MIDDLE;
static const LimitSwitch3::Position M2_POS_RIGHT  = LimitSwitch3::Position::RIGHT;

// --- M3: Vástago del lanzador ---
//  true  = adelante → llega a FORWARD  (Side::A)
//  false = atrás    → llega a BACKWARD (Side::B)
#define M3_DIR_FORWARD   true
#define M3_DIR_BACKWARD  false
static const Motor::Side M3_SIDE_FORWARD  = Motor::Side::A;
static const Motor::Side M3_SIDE_BACKWARD = Motor::Side::B;

// --- M4: Retención del lanzador ---
//  true  = cerrar → llega a CLOSE (Side::A)
//  false = abrir  → llega a OPEN  (Side::B)
#define M4_DIR_CLOSE  true
#define M4_DIR_OPEN   false
static const Motor::Side M4_SIDE_CLOSE = Motor::Side::A;
static const Motor::Side M4_SIDE_OPEN  = Motor::Side::B;

// --- M5: Elevador de retorno ---
//  true  = subir  → llega a UP   (Side::A)
//  false = bajar  → llega a DOWN (Side::B)
#define M5_DIR_UP    true
#define M5_DIR_DOWN  false
static const Motor::Side M5_SIDE_UP   = Motor::Side::A;
static const Motor::Side M5_SIDE_DOWN = Motor::Side::B;