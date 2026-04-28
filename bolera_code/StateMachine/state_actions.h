#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../Classes/Display.h"
#include "../Classes/Led.h"
#include "../Classes/Motor.h"
#include "../Classes/Motor2.h"
#include "../Classes/PinsManager.h"
#include "../Classes/LimitSwitch.h"
#include "../Classes/LimitSwitch3.h"
#include "../Classes/SwitchUser.h"
#include "../Classes/Timer.h"

#include "status.h"
#include "../motor_dirs.h"

class StateActionBase {
	protected:
		Status* status;
	public:
		explicit StateActionBase(Status* s): status(s) {}
		~StateActionBase() = default;
		
		virtual States check_transitions() { return States::NO_CHANGE; };
		
		virtual void entry() {};
		virtual void run()	 {};
		virtual void exit()  {};
};

/* ============================ */
/*		       INIT			    */
/* ============================ */

class InitState : public StateActionBase {
	public:
	explicit InitState(Status* s) : StateActionBase(s) {}
		
	void entry() override {
		UCSR0B = 0;
		
		// --- CONFIGURACIÓN DE PINES PWM COMO SALIDAS ---
		DDRB |= (1 << PB4) | (1 << PB5) | (1 << PB6) | (1 << PB7);
		DDRL |= (1 << PL3);

		// --- CONFIGURACIÓN DE TIMERS ---
		// NOTA: Usamos '=' para limpiar la configuración previa del bootloader.

		// Timer 0 (M4 - OCR0A)
		TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);	// Fast PWM
		TCCR0B = (1 << CS01) | (1 << CS00);

		// Timer 1 (M2 - OCR1A, M3 - OCR1B)
		TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);  // Fast PWM (8 bit)
		TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);		// Prescaler 64
		OCR1AH = 0;	OCR1AL = 0;
		OCR1BH = 0;	OCR1BL = 0;

		// Timer 2 (M1 - OCR2A)
		TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);	// Fast PWM
		TCCR2B = (1 << CS22);									// Prescaler 64

		// Timer 5 (M5 - OCR5AL)
		TCCR5A = (1 << COM5A1) | (1 << WGM50);					// Fast PWM (8 bit)
		TCCR5B = (1 << WGM52) | (1 << CS51) | (1 << CS50);		// Prescaler 64
		OCR5AH = 0; OCR5AL = 0;
		
		// Inicializacion de Clases
		status->sw1->init(); status->sw2->init(); status->sw3->init(); status->sw4->init(); status->sw5->init(); status->sw6->init();
		status->m1->init();  status->m2->init();  status->m3->init();  status->m4->init();  status->m5->init();
		status->led->init();
		status->display->init();
		status->pinsManager->init();
		status->timer->init();
		
		status->m5->setEscapeTime(800);
	}
		
	States check_transitions() override {
		// Init pasa inmediatamente a calibracin
		return States::Calibration;
	}
};

/* ============================ */
/*		   CALIBRATION			*/
/* ============================ */

class CalibrationState : public StateActionBase {
	private:
	    bool m2_started = false;
        bool m3_started = false;

    // Helpers de legibilidad
		bool m1_at_up()   { return !status->m1->isMoving() && status->m1->isAt(M1_SIDE_UP); }
		bool m4_at_open() { return !status->m4->isMoving() && status->m4->isAt(M4_SIDE_OPEN); }

	public:
		explicit CalibrationState(Status *s) : StateActionBase(s) {}
			
        void entry() override {
	        status->led->off();
	        m2_started = false;
	        m3_started = false;
	        
	        // Subimos la velocidad a 255 (100% de fuerza) para evitar bloqueos mecánicos
	        status->m1->move(M1_DIR_UP,		status->current_time, 255);
	        status->m4->move(M4_DIR_OPEN,	status->current_time, 255);
	        status->m5->move(M5_DIR_DOWN,	status->current_time, 255);
	        
	        status->m2->stop();
	        status->m3->stop();
        }
        
        void run() override {
	        if (!m2_started && m1_at_up()) {
		        // Subimos la velocidad a 255 para el barrido
		        status->m2->startHoming(M2_DIR_RIGHT, LimitSwitch3::Position::RIGHT, status->current_time, 255);
		        m2_started = true;
	        }
	        
	        if (!m3_started && m4_at_open()) {
		        // Subimos la velocidad a 255
		        status->m3->move(M3_DIR_FORWARD, status->current_time, 255);
		        m3_started = true;
	        }
        }
		
		States check_transitions() override {
			bool m1_rdy = m1_at_up();
			bool m2_rdy = status->m2->isAt(M2_POS_RIGHT);
			bool m3_rdy = status->m3->isAt(M3_SIDE_FORWARD);
			bool m4_rdy = m4_at_open();
			bool m5_rdy = status->m5->isAt(M5_SIDE_DOWN);
    
			if (m1_rdy && m2_rdy && m3_rdy && m4_rdy && m5_rdy) {
				return States::Idle;
			}
			return States::NO_CHANGE;
		}
};

/* ============================ */
/*		       IDLE			    */
/* ============================ */

class IdleState : public StateActionBase {
	public:
		explicit IdleState(Status* s) : StateActionBase(s) {}
			
		void entry() override {
			status->led->on();
			status->game_running = false;
		}
		
		States check_transitions() override {
			if (status->sw6->consumeClick()) {
				return States::Carga;
			}
			return States::NO_CHANGE;
		}
		
		void exit() override {
			status->score = 0;
			status->game_start_time = status->current_time;
			status->is_last_turn = false;
			status->game_running = true;
			status->pinsManager->reset();
			status->led->off();
		}
};

/* ============================ */
/*		       CARGA			*/
/* ============================ */

class CargaState : public StateActionBase {
	private:
		uint8_t step_m12 = 0;
		uint8_t step_m34 = 0;
		uint32_t timer_m1 = 0;
	public:
		explicit CargaState(Status* s) : StateActionBase(s) {}

		void entry() override {
			step_m12 = 0;
			timer_m1 = 0;
			
			if (status->is_armed) {
				step_m34 = 4;
				status->m1->move(M1_DIR_UP, status->current_time);
			} else {
				step_m34 = 0;
				status->m4->move(M4_DIR_OPEN, status->current_time);
			}
		}
				
		void run() override {
			// ==========================================
			// FASE 1: Secuencia de M3 y M4 (Bolos)
			// ==========================================
			if (step_m34 == 0 && status->m4->isAt(M4_SIDE_OPEN)) {
				status->m3->move(M3_DIR_FORWARD, status->current_time, 255);
				step_m34 = 1;
			}
			else if (step_m34 == 1 && status->m3->isAt(M3_SIDE_FORWARD)) {
				status->m4->move(M4_DIR_CLOSE, status->current_time);
				step_m34 = 2;
			}
			else if (step_m34 == 2 && status->m4->isAt(M4_SIDE_CLOSE)) {
				status->m3->move(M3_DIR_BACKWARD, status->current_time, 255);
				step_m34 = 3;
			}
			else if (step_m34 == 3 && status->m3->isAt(M3_SIDE_BACKWARD)) {
				step_m34 = 4; // Finalizada la Fase 1
				
				status->is_armed = true; // --- NUEVO: Guardamos que ya se han armado si veníamos de Idle
				
				// AHORA damos permiso a M1 para arrancar la Fase 2
				status->m1->move(M1_DIR_UP, status->current_time);
			}
			
			// ==========================================
			// FASE 2: Secuencia de M1 y M2 (Recarga)
			// Solo se ejecuta si la Fase 1 ha terminado (step_m34 == 4)
			// ==========================================
			if (step_m34 == 4) {
				if(step_m12 == 0 && status->m1->isAt(M1_SIDE_UP)) {
					// Asumiendo que usas LimitSwitch3::Position::RIGHT (o tu define M2_POS_RIGHT)
					if (status->m2->isAt(LimitSwitch3::Position::RIGHT)) { 
						status->m1->move(M1_DIR_DOWN, status->current_time);
						step_m12 = 1;
					}
					else if (!status->m2->isMoving()) {
						status->m2->move(M2_DIR_RIGHT, LimitSwitch3::Position::RIGHT, status->current_time);
					}
				}
				else if (step_m12 == 1 && status->m1->isAt(M1_SIDE_DOWN)) {
					timer_m1 = status->current_time;
					step_m12 = 2;
				}
				else if (step_m12 == 2 && (status->current_time - timer_m1) >= 1000) {
					status->m1->move(M1_DIR_UP, status->current_time);
					step_m12 = 3;
				}
				else if (step_m12 == 3 && status->m1->isAt(M1_SIDE_UP)) {
					step_m12 = 4; // Finalizada la Fase 2
				}
			}
		}
		
		States check_transitions() override {
			if (step_m12 == 4 && step_m34 == 4) {
				return States::Armado;
			}
			return States::NO_CHANGE;
		}
};

/* ============================ */
/*		      ARMADO			*/
/* ============================ */

class ArmadoState : public StateActionBase {
	private:
	bool going_left = true; 
	public:
	explicit ArmadoState(Status* s) : StateActionBase(s) {}
	
	void entry() override {
		status->led->on();
		going_left = true;
		
		// M2 viene del extremo DERECHO (al final de Carga). 
		// Lo mandamos al IZQUIERDO para iniciar el barrido. 
		// Al hacerlo, pasará por el centro ignorándolo gracias a tu clase Motor2.
		status->m2->move(M2_DIR_LEFT, LimitSwitch3::Position::LEFT, status->current_time);
	}
	
	void run() override {
		if (!status->m2->isMoving()) {
			
			if (going_left) {
				// Si íbamos a la izquierda y llegamos a la izquierda
				if(status->m2->isAt(LimitSwitch3::Position::LEFT)) {
					going_left = false; // Rebotar hacia la derecha (pero solo hasta el MEDIO)
					status->m2->move(M2_DIR_RIGHT, LimitSwitch3::Position::MIDDLE, status->current_time);
				} else {
					// Rescate por si se detiene antes de tiempo
					status->m2->move(M2_DIR_LEFT, LimitSwitch3::Position::LEFT, status->current_time);
				}
			} else {
				// Si íbamos hacia la derecha (hacia el CENTRO) y llegamos al CENTRO
				if (status->m2->isAt(LimitSwitch3::Position::MIDDLE)) {
					going_left = true; // Rebotar hacia la izquierda
					status->m2->move(M2_DIR_LEFT, LimitSwitch3::Position::LEFT, status->current_time);
				}
				else {
					// Nos aseguramos de que el destino es siempre MIDDLE, y nunca enviarlo a RIGHT
					status->m2->move(M2_DIR_RIGHT, LimitSwitch3::Position::MIDDLE, status->current_time);
				}
			}
		}
	}
	
	States check_transitions() override {
		if (status->sw6->consumeClick()) {
			return States::Disparo;
		}
		return States::NO_CHANGE;
	}
	
	void exit() override {
		status->led->off();
		status->m2->stop(); // Congelamos el motor en la posición exacta donde disparó el jugador
	}
};

/* ============================ */
/*		      DISPARO			*/
/* ============================ */
class DisparoState : public StateActionBase {
	private:
	uint32_t start_time = 0;
	bool m2_finished = false;
	
	public:
	explicit DisparoState(Status* s) : StateActionBase(s) {}

	void entry() override {
		start_time = status->current_time;
		m2_finished = false;
		
		status->is_armed = false; 
		
		// --- CRÍTICO: Abrimos M4 instantáneamente. 
		// Como es el retenedor, esto es lo que dispara físicamente la bola.
		status->m4->move(M4_DIR_OPEN, status->current_time);
	}

	void run() override {
		// Pasados 3 segundos del disparo, mandamos M2 a su zona de aparcamiento
		if ((status->current_time - start_time) >= 3000) {
			if (!m2_finished) {
				if (status->m2->isAt(LimitSwitch3::Position::RIGHT)) {
					m2_finished = true;
				}
				else if (!status->m2->isMoving()) {
					status->m2->startHoming(M2_DIR_RIGHT, LimitSwitch3::Position::RIGHT, status->current_time);
				}
			}
		}
	}

	States check_transitions() override {
		// Salimos cuando pasen los 5 segundos totales de espera de la bola
		// Y confirmemos que la compuerta M4 se abrió completamente (vástago liberado)
		// Y M2 esté aparcado.
		if ((status->current_time - start_time >= 5000) && m2_finished && status->m4->isAt(M4_SIDE_OPEN)) {
			return States::Retorno;
		}
		return States::NO_CHANGE;
	}
	
	void exit() override {
		status->score += status->pinsManager->getScore();
		status->display->setScore(status->score);
		
		status->pinsManager->reset();
	}
};

/* ============================ */
/*		      RETORNO			*/
/* ============================ */

class RetornoState : public StateActionBase {
	private:
		uint8_t step_m5 = 0;
		uint8_t step_m34 = 0; // --- NUEVO: Control paralelo de M3 y M4
		
	public:
		explicit RetornoState(Status* s) : StateActionBase(s) {}

		void entry() override {
			step_m5 = 0;
			step_m34 = 0;
			
			// Arrancamos el movimiento nada más entrar al estado
			status->m5->move(M5_DIR_UP, status->current_time, 255);

			// --- NUEVO: Si no es el último turno, arrancamos la compuerta de los bolos
			if (!status->is_last_turn) {
				status->m4->move(M4_DIR_OPEN, status->current_time);
			}
		}
				
		void run() override {
			// Si el tiempo NO se ha agotado (Turno Normal)
			if (!status->is_last_turn) {
				
				// --- LÓGICA M5 (Pelota) ---
				if (step_m5 == 0 && status->m5->isAt(M5_SIDE_UP)) {
					status->m5->move(M5_DIR_DOWN, status->current_time, 255);
					step_m5 = 1;
				}

				// --- NUEVO: LÓGICA PARALELA M3/M4 (Bolos) ---
				// Paso 0: Como M4 ya viene abierto del disparo, empezamos mandando a M3 adelante.
				if (step_m34 == 0 && status->m4->isAt(M4_SIDE_OPEN)) {
					status->m3->move(M3_DIR_FORWARD, status->current_time, 255);
					step_m34 = 1;
				}
				else if (step_m34 == 1 && status->m3->isAt(M3_SIDE_FORWARD)) {
					// Paso 1: M3 llegó adelante. Cerramos la compuerta M4.
					status->m4->move(M4_DIR_CLOSE, status->current_time);
					step_m34 = 2;
				}
				else if (step_m34 == 2 && status->m4->isAt(M4_SIDE_CLOSE)) {
					// Paso 2: Compuerta cerrada. Retiramos a M3 hacia atrás.
					status->m3->move(M3_DIR_BACKWARD, status->current_time, 255);
					step_m34 = 3;
				}
				else if (step_m34 == 3 && status->m3->isAt(M3_SIDE_BACKWARD)) {
					step_m34 = 4; // Secuencia completada
					status->is_armed = true; 
				}

			}
			else {
				// Si el tiempo SÍ se ha agotado (último turno)
				// M5 solo sube. Como ya le dimos la orden en el entry(), aquí no hay que hacer nada más.
			}
		}
		
		States check_transitions() override {
			if (!status->is_last_turn) {
				// --- NUEVO: Transición espera a que M5 baje Y los bolos (M3/M4) hayan terminado
				if (step_m5 == 1 && status->m5->isAt(M5_SIDE_DOWN) && step_m34 == 4) {
					return States::Carga; 
				}
			} 
			else {
				// Transición para último turno: Esperamos a que llegue ARRIBA
				if (status->m5->isAt(M5_SIDE_UP)) {
					return States::Idle; 
				}
			}
			
			return States::NO_CHANGE;
		}
};