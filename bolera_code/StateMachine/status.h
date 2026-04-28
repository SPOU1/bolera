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

enum class States : uint8_t {
	Init = 0,
	Calibration = 1,
	Idle = 2,
	Carga = 3,
	Armado = 4,
	Disparo = 5,
	Retorno = 6,
	
	NO_CHANGE = 255
};

class Status {
	public:
		Status() : current_state(States::Init) {}
			
		~Status() = default;
		
		void setState(States s) { current_state = s; }
		States getState() 		{return current_state; }
							
		void update_sensors() {
			current_time = timer->millis();
			
			sw1->update(current_time);
			sw2->update(current_time);
			sw3->update(current_time);
			sw4->update(current_time);
			sw5->update(current_time);
			sw6->update(current_time);
			
			m1->update(current_time);
			m2->update(current_time);
			m3->update(current_time);
			m4->update(current_time);
			m5->update(current_time);
			
			led->update(current_time, is_last_turn);
			display->update(current_time, !game_running);
			
			if(game_running && ((current_time-game_start_time) >= 300000)) {
				is_last_turn = true;
			}
		}
				
		// Hardware
		Motor* m1; Motor2* m2; Motor* m3; Motor* m4; Motor* m5;
		LimitSwitch* sw1; LimitSwitch3* sw2; LimitSwitch* sw3; LimitSwitch* sw4; LimitSwitch* sw5; SwitchUser* sw6; 
		
		Led* led;
		Display* display;
		PinsManager* pinsManager;
		Timer* timer;
		
		// Variables de juego
		uint8_t score = 0;
		bool is_last_turn = false;
		bool game_running = false;
		bool is_armed = false;
		uint32_t game_start_time = 0;
		uint32_t current_time = 0;
		
	private:
		States current_state;			
};