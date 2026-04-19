#include <avr/io.h>
#include <avr/interrupt.h>
#include "Classes/Display.h"
#include "Classes/Led.h"
#include "Classes/Motor.h"
#include "Classes/Motor2.h"
#include "Classes/PinsManager.h"
#include "Classes/SwitchMotor.h"
#include "Classes/SwitchMotor2.h"
#include "Classes/SwitchUser.h"
#include "Classes/Timer.h"

#include "StateMachine/state_machine.h"
#include "StateMachine/state_actions.h"
#include "StateMachine/status.h"

PinsManager* pinsPtr = 0;
ISR(PCINT2_vect) { if(pinsPtr) pinsPtr->onInterrupt(); }
	
Timer* timerPtr = 0;
ISR(TIMER4_COMPA_vect) { if(timerPtr) timerPtr->addTick(); }

int main(void) {
	// Instanciación de Hardware
	SwitchMotor sw1(&PINK, &PORTK, &DDRK, (1 << PK6));
	SwitchMotor2 sw2(&PINK, &PORTK, &DDRK, (1 << PK7));
	SwitchMotor sw3(&PIND, &PORTD, &DDRD, (1 << PD0));
	SwitchMotor sw4(&PIND, &PORTD, &DDRD, (1 << PD1));
	SwitchMotor sw5(&PIND, &PORTD, &DDRD, (1 << PD2));
	SwitchUser sw6(&PIND, &PORTD, &DDRD, (1 << PD3));
	
	Motor m1(&PORTB, &DDRB, (1 << PB0), &PORTB, &DDRB, (1 << PB4), &OCR2A, &sw1);
	Motor2 m2(&PORTB, &DDRB, (1 << PB1), &PORTB, &DDRB, (1 << PB5), &OCR1AL, &sw2);
	Motor m3(&PORTB, &DDRB, (1 << PB2), &PORTB, &DDRB, (1 << PB6), &OCR1BL, &sw3);
	Motor m4(&PORTB, &DDRB, (1 << PB3), &PORTB, &DDRB, (1 << PB7), &OCR0A, &sw4);
	Motor m5(&PORTD, &DDRD, (1 << PD4), &PORTL, &DDRL, (1 << PL3), &OCR5AL, &sw5);
	
	volatile uint8_t* segPorts[7] = {&PORTL, &PORTL, &PORTL, &PORTD, &PORTL, &PORTL, &PORTL};
	volatile uint8_t* segDdrs[7]  = {&DDRL,  &DDRL,  &DDRL,  &DDRD,  &DDRL,  &DDRL,  &DDRL};
	uint8_t segMasks[7]           = {(1<<PL0), (1<<PL1), (1<<PL2), (1<<PD6), (1<<PL4), (1<<PL5), (1<<PL6)};
	Display display(
	segPorts,
	segDdrs,
	segMasks,
	&PORTL, &DDRL, (1<<PL7)
	);
	
	Led led(&PORTD, &DDRD, (1<<PD7));
	PinsManager pinsManager;
	Timer timer;
	
	// Interrupt pointers
	pinsPtr = &pinsManager;
	timerPtr = &timer;
	
	// Vincular a state machine
	StateMachine sm;
	Status& st = sm.get_status();
	
	st.m1 = &m1; st.m2 = &m2; st.m3 = &m3; st.m4 = &m4; st.m5 = &m5;
	st.sw1 = &sw1; st.sw2 = &sw2; st.sw3 = &sw3; st.sw4 = &sw4; st.sw5 = &sw5; st.sw6 = &sw6;
	st.led = &led;
	st.display = &display;
	st.pinsManager = &pinsManager;
	st.timer = &timer;
	
	sm.start();
	
	return 0;
}