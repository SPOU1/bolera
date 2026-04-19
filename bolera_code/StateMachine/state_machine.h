#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>

#include "status.h"
#include "state_actions.h"

class StateMachine {
	private:
	Status status;
	
	#define NUM_STATES 7
	// State Actions instances
	InitState init_state{&status};
	CalibrationState calibration_state{&status};
	IdleState idle_state{&status};
	CargaState carga_state{&status};
	ArmadoState armado_state{&status};
	DisparoState disparo_state{&status};
	RetornoState retorno_state{&status};

	// States Lookup Array
	StateActionBase* state_actions[NUM_STATES];

	public:
	explicit StateMachine() {
		// Populate lookup array
		state_actions[static_cast<uint8_t>(States::Init)] = &init_state;
		state_actions[static_cast<uint8_t>(States::Calibration)] = &calibration_state;
		state_actions[static_cast<uint8_t>(States::Idle)] = &idle_state;
		state_actions[static_cast<uint8_t>(States::Carga)] = &carga_state;
		state_actions[static_cast<uint8_t>(States::Armado)] = &armado_state;
		state_actions[static_cast<uint8_t>(States::Disparo)] = &disparo_state;
		state_actions[static_cast<uint8_t>(States::Retorno)] = &retorno_state;
	}

	void step() {
		status.update_sensors();

		// 1. Get from current state the transition state (if any)
		StateActionBase* current_action = state_actions[static_cast<uint8_t>(status.getState())];
		States next_state = current_action->check_transitions();

		// 2. Perform transition if needed
		if (next_state != States::NO_CHANGE && next_state != status.getState()) {
			current_action->exit();
			status.setState(next_state);
			state_actions[static_cast<uint8_t>(status.getState())]->entry();
		}

		// 3. Perform action of the (potentially new) current state
		state_actions[static_cast<uint8_t>(status.getState())]->run();
	}

	void start() {
		// 1. Trigger initial state entry
		state_actions[static_cast<uint8_t>(status.getState())]->entry();

		while (1) {
			step();
		}
	}
	
	Status& get_status() { return status; }
};