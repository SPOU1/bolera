#pragma once
#include <avr/io.h>
#include "SwitchMotor2.h"

class Motor {
	/*
	 * ==== Motor2 ====
	 * Gestiona un motor usando PWM, adaptado para 3 posiciones.
	 * Soprta rutinas de escape y calibración sin bloquear el programa principal.
	 */
	private:
	volatile uint8_t* portDIR;
	volatile uint8_t* ddrDIR;
	uint8_t maskDIR;
	
	volatile uint8_t* portEN;
	volatile uint8_t* ddrEN;
	uint8_t maskEN;
	
	volatile uint8_t* ocrPWM; // Registro Output Compare para controlar la velocidad (Duty Cycle).
	
	SwitchMotor2* eorSwitch; // Puntero a clase SwitchMotor2. Switch fin de carrera (3 posiciones) asociado al motor.
	
	bool isMoving;
	
	// Escape
	bool isEscaping;
	uint32_t escapeStartTimestamp;
	const uint32_t escapeDelay = 100;
	
	// Calibración
	bool isCalibrating;
	bool isWaitingAfterCalib;
	uint32_t stateTimer;
	bool calibDirection;
	
	public:
	Motor(
		volatile uint8_t* pDIR, volatile uint8_t* dDIR, uint8_t mDIR,
		volatile uint8_t* pEN,  volatile uint8_t* dEN,  uint8_t mEN,
		volatile uint8_t* rPWM,
		SwitchMotor2* sw = 0
	) {
		portDIR = pDIR; ddrDIR = dDIR; maskDIR = mDIR;
		portEN  = pEN;  ddrEN  = dEN;  maskEN  = mEN;
		ocrPWM  = rPWM;
		eorSwitch = sw;
		
		isMoving = false;
		isEscaping = false;
		isCalibrating = false;
		isWaitingAfterCalib = false;
	}
	
	void init() {
		/* 
		 * ==== init() ====
		 * Configura ambos pines como salida e inicializa velocidad 0.
		 */
		*ddrDIR |= maskDIR;	// Output
		*ddrEN |= maskEN;	// Output
		*portDIR &= ~maskDIR;
		*portEN &= ~maskEN;
		
		*ocrPWM = 0;
	}
	
	void setDirection(bool forward) {
		/* 
		 * ==== setDirection(bool forward) ====
		 * Establece la dirección de giro del motor.
		 *  - forward: [bool] Dirección de giro.
		 */
		if (forward)	*portDIR |= maskDIR;
		else			*portDIR &= ~maskDIR;
	}
	
	void setSpeed(uint8_t speed) {
		/* 
		 * ==== setSpeed(bool speed) ====
		 * Establece la velociad de rotación del motor.
		 *  - speed: [uint8_t] Velocidad (PWM) de rotación.
		 */
		*ocrPWM = speed;
	}
	
	void stop() {
		/* 
		 * ==== stop() ====
		 * Para el motor, y resetea las variables de estado.
		 */
		setSpeed(0);
		*portEN &= ~maskEN;
		isMoving = false;
	}
	
	bool getIsMoving() {
		/*
		 * ==== getIsMoving() ====
		 * Devuelve si el motor se está moviendo.
		 */
		return isMoving;
	}
	
	void calibrate(bool direction, uint32_t currentTime) {
		/*
		 * ==== calibrate(bool direction, uint32_t currentTime) ====
		 * Inicia la secuencia de calibración (buscar referencia).
		 * Disminuye la velocidad y activa el temporizador.
		 *  - direction: [bool] Dirección de giro durante la calibración.
		 *  - currentTime: [uint32_t] Tiempo actual en ms.
		 */
		
		startMoving(direction, currentTime);
		
		if(isMoving) {
			setSpeed(153); // 60% PWM
		}
		
		isCalibrating = true;
		isWaitingAfterCalib = false;
		calibDirection = direction;
		stateTimer = currentTime;		
	}
	
	void startMoving(bool direction, uint32_t currentTime) {
		/*
		 * ==== startMoving(bool direction, uint32_t currentTime) ====
		 * Ordena el arranque del motor en una dirección determinada. Parará en la siguiente posición encontrada.
		 *  - direction: [bool] Dirección de giro.
		 *  - currentTime: [uint32_t] Tiempo actual en ms.
		 */
		bool startFromSide = false;
	
		if (eorSwitch != 0) {
			SwitchMotor::SwState state = eorSwitch->getState();
			
			// No moverse si ya está en posición de destino (extremos).
			if ((direction && state == SwitchMotor::SIDE_1) ||
				(!direction && state == SwitchMotor::SIDE_2)) {
				return;
			}
			if (state != SwitchMotor::MOVING) {
				startFromSide = true;
			}
			
			// Prepara el switch para escuchar la colisión.
			// eorSwitch->forceState(SwitchMotor::MOVING); (incluido en setExpectedDirection en SwitchMotor2)
			eorSwitch->setExpectedDirection(direction);
		}
		
		setDirection(direction);
		setSpeed(204); // 80% PWM
		isMoving = true;
		
		// Si arranca en CUALQUIER sensor, activar escape temporal.
		if (startFromSide || (eorSwitch != 0 && eorSwitch->isPressed())) {	
			isEscaping = true;
			escapeStartTimestamp = currentTime;
		} else {
			isEscaping = false;
		}
	}
	
	void update(uint32_t currentTime) {
		/*
		 * ==== update(uint32_t currentTime) ====
		 * Actualiza el estado del motor en cada iteración.
		 * Revisa colisiones y timeouts.
		 *  - currentTime: [uint32_t] Tiempo actual en ms.
		 */
		
		// 1. Pausa post-calibración
		if (isWaitingAfterCalib) {
			if ((currentTime - stateTimer) >= 200) {
				isWaitingAfterCalib = false;
				if (eorSwitch != 0) {
					// Asumimos calibración a algún extremo (no en SIDE_MIDDLE)
					if (calibDirection)	eorSwitch->forceState(SwitchMotor::SIDE_1);
					else				eorSwitch->forceState(SwitchMotor::SIDE_2);
				}	
			}
			return;
		}
		
		// 2. Control del movimiento general
		if (eorSwitch != 0 && isMoving) {
			if (isEscaping) {
				// Ignora colisiones hasta salir de zona de switch
				if (!eorSwitch->isPressed()) {
					if((currentTime - escapeStartTimestamp) > escapeDelay) {
						isEscaping = false;
						eorSwitch->consumePressedFlag(); 
					}
				} else {
					escapeStartTimestamp = currentTime;
				}
			} else {
				// Si se pulsa el switch (a SIDE_MIDDLE o un extremo):
				if(eorSwitch->consumePressedFlag()) {
					stop();
				
					if (isCalibrating) {
						isCalibrating = false;
						isWaitingAfterCalib = true;
						stateTimer = currentTime;
					}
				}
				// Timeout de seguridad para calibración.
				else if (isCalibrating && (currentTime - stateTimer) > 10000) {
					stop();
					isCalibrating = false;
				}
			}
		}
	}
};