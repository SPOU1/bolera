/* 
 * motor_dirs.h
 * Definición de constantes para direcciones y posiciones de motores 
*/

#pragma once
#include "Classes/Motor.h"
#include "Classes/Motor2.h"
#include "Classes/LimitSwitch3.h"

#define M1_DIR_UP     true
#define M1_DIR_DOWN   false
static const Motor::Side M1_SIDE_UP   = Motor::Side::A;
static const Motor::Side M1_SIDE_DOWN = Motor::Side::B;

#define M2_DIR_LEFT   true
#define M2_DIR_RIGHT  false
static const LimitSwitch3::Position M2_POS_LEFT   = LimitSwitch3::Position::LEFT;
static const LimitSwitch3::Position M2_POS_MIDDLE = LimitSwitch3::Position::MIDDLE;
static const LimitSwitch3::Position M2_POS_RIGHT  = LimitSwitch3::Position::RIGHT;

#define M3_DIR_FORWARD   true
#define M3_DIR_BACKWARD  false
static const Motor::Side M3_SIDE_FORWARD  = Motor::Side::A;
static const Motor::Side M3_SIDE_BACKWARD = Motor::Side::B;

#define M4_DIR_CLOSE  true
#define M4_DIR_OPEN   false
static const Motor::Side M4_SIDE_CLOSE = Motor::Side::A;
static const Motor::Side M4_SIDE_OPEN  = Motor::Side::B;

#define M5_DIR_UP    true
#define M5_DIR_DOWN  false
static const Motor::Side M5_SIDE_UP   = Motor::Side::A;
static const Motor::Side M5_SIDE_DOWN = Motor::Side::B;