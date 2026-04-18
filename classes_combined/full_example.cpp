#include <avr/io.h>
#include <avr/interrupt.h>

#include "Display.h"
#include "Led.h"
#include "Motor.h"
#include "PinsManager.h"
#include "SwitchMotor.h"
#include "SwitchUser.h"
#include "Timer.h"

// ==== Variables globales y punteros de Interrupción ====
Timer systemTimer;
Timer* timerPtr = &systemTimer;
volatile uint32_t endGameTimestamp;

PinsManager pins;
PinsManager* pinsPtr = &pins;

ISR(TIMER4_COMPA_vect) { timerPtr->addTick(); }
ISR(PCINT2_vect) { pinsPtr->onInterrupt(); }
	
// ==== Instanciación de Hadware =====
SwitchUser sw6(&PIND, &PORTD, &DDRD, (1<<PD3));

SwitchMotor sw1(&PINK, &PORTK, &DDRK, (1<<PK6));
Motor m1(&PORTB, &DDRB, (1<<PB0), &PORTB, &DDRB, (1<<PB4), &OCR2A, &sw1);
#define M1_GO_UP	true
#define M1_GO_DOWN	false
#define M1_IS_UP	SwitchMotor::SIDE_1
#define M1_IS_DOWN	SwitchMotor::SIDE_2

Led led(&PORTD, &DDRD, (1<<PD7));

volatile uint8_t* segPorts[7] = {&PORTL, &PORTL, &PORTL, &PORTD, &PORTL, &PORTL, &PORTL};
volatile uint8_t* segDdrs[7]  = {&DDRL,  &DDRL,  &DDRL,  &DDRD,  &DDRL,  &DDRL,  &DDRL};
uint8_t segMasks[7]           = {(1<<PL0), (1<<PL1), (1<<PL2), (1<<PD6), (1<<PL4), (1<<PL5), (1<<PL6)};
Display display(segPorts, segDdrs, segMasks, &PORTL, &DDRL, (1<<PL7));

void PWMTimersInit() {
	// Timer 2 (M1)
	TCCR2A |= (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
	TCCR2A &= ~(1 << COM2A0);
	TCCR2B |= (1 << CS22);
	TCCR2B &= ~((1 << CS21) | (1 << CS20));
}

void init() {
	cli();
	systemTimer.init();
	pins.init();
	sw6.init();
	sw1.init();
	m1.init();
	display.init();
	led.init();
	PWMTimersInit();
	sei();
}

// ==== MÁQUINA DE ESTADOS =====
enum GameState {
	CALIBRATING,
	AWAIT_INIT,
	IN_GAME,
	END_GAME
};
GameState currentState = AWAIT_INIT;

void update(uint32_t currentTime, GameState state) {
	sw6.update(currentTime);
	sw1.update(currentTime);
	m1.update(currentTime);
	display.update(currentTime, (state == AWAIT_INIT));
	led.update(currentTime, (state == IN_GAME));
}

void setup() {
	init();
	m1.calibrate(M1_GO_UP, systemTimer.millis());
	endGameTimestamp = 0;
	currentState = CALIBRATING;
}

void loop() {
	uint32_t currentTime = systemTimer.millis();
	update(currentTime, currentState);
	
	switch (currentState) {
		case CALIBRATING:
			if (!m1.getIsMoving()) {
				currentState = AWAIT_INIT;
			}
			break;	
		
		case AWAIT_INIT:
			if(sw6.consumeClick()) {
				pins.reset();
				display.setScore(0);
				currentState = IN_GAME;
			}
			break;
		case IN_GAME:
			display.setScore(pins.getScore());
			if (pins.getScore() >= 6) {
				currentState = END_GAME;
				endGameTimestamp = currentTime;
				m1.startMoving(M1_GO_DOWN, currentTime);
			}
			break;
		case END_GAME:
			if(!m1.getIsMoving() && (currentTime - endGameTimestamp) > 2000) {
				if (sw1.getState() == M1_IS_DOWN) {
					m1.startMoving(M1_GO_UP, currentTime);
				}
				
				if (sw1.getState() == M1_IS_UP) {
					currentState = AWAIT_INIT;	
				}
			}
			break;
	}
}

int main() {
	setup();
	while(1) {
		loop();
	}
	return 0;
}