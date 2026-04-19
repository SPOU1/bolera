#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../Classes/Display.h"
#include "../Classes/Led.h"
#include "../Classes/Motor.h"
#include "../Classes/Motor2.h"
#include "../Classes/PinsManager.h"
#include "../Classes/SwitchMotor.h"
#include "../Classes/SwitchMotor2.h"
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
		virtual void run() {};
		virtual void exit() {};
};

/* ============================ */
/*		       INIT			    */
/* ============================ */

class InitState : public StateActionBase {
	public:
	explicit InitState(Status* s) : StateActionBase(s) {}
		
	void entry() override {
		// Inicialización de Clases
		status->sw1->init(); status->sw2->init(); status->sw3->init(); status->sw4->init(); status->sw5->init(); status->sw6->init();
		status->m1->init(); status->m2->init(); status->m3->init(); status->m4->init(); status->m5->init();
		status->led->init();
		status->display->init();
		status->pinsManager->init();
		status->timer->init();
		
		// Timer 0 (M4 - OCR0A)
		TCCR0A |= (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);	// Fast PWM
		TCCR0B |= (1 << CS01) | (1 << CS00);					// Prescaler 64
		
		// Timer 1 (M2 - OCR1A, M3 - OCR1B)
		TCCR1A |= (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10); // Fast PWM (8 bit)
		TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);		// Prescaler 64
		
		// Timer 2 (M1 - OCR2A)
		TCCR2A |= (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);	// Fast PWM
		TCCR2B |= (1 << CS22);									// Prescaler 64
		
		// Timer 5 (M5 - OCR5AL)
		TCCR5A |= (1 << COM5A1) | (1 << WGM50);					// Fast PWM (8 bit)
		TCCR5B |= (1 << WGM52) | (1 << CS51) | (1 << CS50);		// Prescaler 64
	}
	
	States check_transitions() override {
		// Init pasa inmediatamente a calibración
		return States::Calibration;
	}
};

/* ============================ */
/*		   CALIBRATION			*/
/* ============================ */

class CalibrationState : public StateActionBase {
	public:
		explicit CalibrationState(Status *s) : StateActionBase(s) {}
			
		void entry() override {
			status->m1->calibrate(M1_GO_UP, status->current_time);
			status->m4->calibrate(M4_GO_OPEN, status->current_time);
			status->m5->calibrate(M5_GO_DOWN, status->current_time);
		}
		
		void run() override {
			if (!status->m1->getIsMoving() && status->sw1->getState()==M1_IS_UP) {
				if (!status->m2->getIsMoving() && status->sw2->getState() != M2_IS_RIGHT) {
					status->m2->calibrate(M2_GO_RIGHT, status->current_time);				
				}
			}
			
			if (!status->m4->getIsMoving() && status->sw4->getState()==M4_IS_OPEN) {
				if (!status->m3->getIsMoving() && status->sw3->getState() != M3_IS_FORWARD) {
					status->m3->calibrate(M3_GO_FORWARD, status->current_time);
				}
			}
			
		}
		
		States check_transitions() override {
			bool m1_rdy = status->sw1->getState() == M1_IS_UP;
			bool m2_rdy = status->sw2->getState() == M2_IS_RIGHT;
			bool m3_rdy = status->sw3->getState() == M3_IS_FORWARD;
			bool m4_rdy = status->sw4->getState() == M4_IS_OPEN;
			bool m5_rdy = status->sw5->getState() == M5_IS_DOWN;
			
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
			step_m34 = 0;
			timer_m1 = 0;
			
			status->m1->startMoving(M1_GO_UP, status->current_time);
			status->m4->startMoving(M4_GO_OPEN, status->current_time);
		}
				
		void run() override {
			if(step_m12 == 0 && status->sw1->getState() == M1_IS_UP) {
				if (status->sw2->getState() == M2_IS_RIGHT) {
					status->m1->startMoving(M1_GO_DOWN, status->current_time);
					step_m12 = 1;
				}
				else if (!status->m2->getIsMoving()) {
					status->m2->startMoving(M2_GO_RIGHT, status->current_time);
				}
			}
			else if (step_m12 == 1 && status->sw1->getState() == M1_IS_DOWN) {
				timer_m1 = status->current_time;
				step_m12 = 2;
			}
			else if (step_m12 == 2 && (status->current_time - timer_m1) >= 1000) {
				status->m1->startMoving(M1_GO_UP, status->current_time);
				step_m12 = 3;
			}
			else if (step_m12 == 3 && status->sw1->getState() == M1_IS_UP) {
				step_m12 = 4;
			}
			
			if (step_m34 == 0 && status->sw4->getState() == M4_IS_OPEN) {
				status->m3->startMoving(M3_GO_FORWARD, status->current_time);
				step_m34 = 1;
			}
			else if (step_m34 == 1 && status->sw3->getState() == M3_IS_FORWARD) {
				status->m4->startMoving(M4_GO_CLOSE, status->current_time);
				step_m34 = 2;
			}
			else if (step_m34 == 2 && status->sw4->getState() == M4_IS_CLOSE) {
				status->m3->startMoving(M3_GO_BACKWARD, status->current_time);
				step_m34 = 3;
			}
			else if (step_m34 == 3 && status->sw3->getState() == M3_IS_BACKWARD) {
				step_m34 = 4;
			}
		}
		
		States check_transitions() override {
			if (step_m12==4 && step_m34 ==4) {
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
		bool is_oscilating = false;
	public:
		explicit ArmadoState(Status* s) : StateActionBase(s) {}
		void entry() override {
			status->led->on();
			is_oscilating = false;
			status->m2->startMoving(M2_GO_LEFT, status->current_time);

		}
		
		void run() override {
			if (!status->m2->getIsMoving()) {
				if(!is_oscilating) {
					if(status->sw2->getState() == M2_IS_LEFT) {
						is_oscilating = true;
						status->m2->startMoving(M2_GO_RIGHT, status->current_time);
					} else {
						status->m2->startMoving(M2_GO_LEFT, status->current_time);
					}
				} else {
					if (status->sw2->getState() == M2_IS_LEFT) {
						status->m2->startMoving(M2_GO_RIGHT, status->current_time);
					}
					else if (status->sw2->getState() == M2_IS_MIDDLE) {
						status->m2->startMoving(M2_GO_LEFT, status->current_time);
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
			status->m2->stop();
		}
};

/* ============================ */
/*		      DISPARO			*/
/* ============================ */
class DisparoState : public StateActionBase {
	private:
	uint32_t start_time = 0;
	uint8_t step_m34 = 0;
	bool m2_finished = false;
	
	public:
	explicit DisparoState(Status* s) : StateActionBase(s) {}

	void entry() override {
		start_time = status->current_time;
		step_m34 = 0;
		m2_finished = false;
		
		status->m4->startMoving(M4_GO_OPEN, status->current_time);
	}

	void run() override {
		if (step_m34 == 0 && status->sw4->getState() == M4_IS_OPEN) {
			status->m2->startMoving(M2_GO_RIGHT, status->current_time);
			status->m3->startMoving(M3_GO_FORWARD, status->current_time);
			step_m34 = 1;
		}
		
		if (step_m34 >= 1 && !m2_finished) {
			if (status->sw2->getState() == M2_IS_RIGHT) {
				m2_finished = true;
			}
			else if (!status->m2->getIsMoving()) {
				status->m2->startMoving(M2_GO_RIGHT, status->current_time);
			}
		}

		if (step_m34 == 1 && status->sw3->getState() == M3_IS_FORWARD) {
			status->m4->startMoving(M4_GO_CLOSE, status->current_time);
			step_m34 = 2;
		}
		else if (step_m34 == 2 && status->sw4->getState() == M4_IS_CLOSE) {
			status->m3->startMoving(M3_GO_BACKWARD, status->current_time);
			step_m34 = 3;
		}
		else if (step_m34 == 3 && status->sw3->getState() == M3_IS_BACKWARD) {
			step_m34 = 4;
		}
	}

	States check_transitions() override {
		if ((status->current_time - start_time >= 5000) && m2_finished && (step_m34 == 4)) {
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
	uint8_t step = 0;
	uint32_t delay_timer = 0;
	
	public:
	explicit RetornoState(Status* s) : StateActionBase(s) {}

	void entry() override {
		step = 0;
		delay_timer = 0;
		
		status->m5->startMoving(M5_GO_UP, status->current_time);
	}

	void run() override {
		if (step == 0 && status->sw5->getState() == M5_IS_UP) {
			delay_timer = status->current_time;
			step = 1;
		}
		else if (step == 1 && (status->current_time - delay_timer >= 1000)) {
			status->m5->startMoving(M5_GO_DOWN, status->current_time);
			step = 2;
		}
		else if (step == 2 && status->sw5->getState() == M5_IS_DOWN) {
			delay_timer = status->current_time;
			step = 3;
		}
		else if (step == 3 && (status->current_time - delay_timer >= 2000)) {
			step = 4;
		}
	}

	States check_transitions() override {
		if (step == 4) {
			if (status->is_last_turn) return States::Idle;
			return States::Carga;
		}
		return States::NO_CHANGE;
	}
};