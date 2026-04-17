#include <avr/io.h>
#define F_CPU 8000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "SwitchMotor.h"
#include "Motor.h"

// MOTOR 5 - Configuración según tu tabla: PD2 para Switch, PD4 para DIR, PL3 para EN
SwitchMotor sw5(&PIND, &PORTD, &DDRD, (1 << PD2));
SwitchMotor sw1(&PINK, &PORTK, &DDRK, (1 << PK6));

Motor motor5(
&PORTD, &DDRD, (1 << PD4),      // DIR (M5_di)
&PORTL, &DDRL, (1 << PL3),      // EN (M5_en)
(volatile uint8_t*)&OCR5A,      // Casting para Timer de 16 bits
&sw5                            // Switch 5
);

Motor motor1(
&PORTB, &DDRB, (1<<PB0),	// DIR
&PORTB, &DDRB, (1<<PB4),	// EN
&OCR2A,					// Timer 5 (low)
&sw1						// Switch 5
);

// Puntero para la ISR
SwitchMotor* sw5ptr = 0;
SwitchMotor* sw1ptr = 0;

// --- INTERRUPCIONES ---
// IMPORTANTE: Usamos INT2_vect porque el switch está en PD2 (INT2)
extern "C" ISR(INT2_vect) {
	if (sw5ptr) sw5ptr->onInterrupt();
}

ISR(PCINT2_vect) {
	if (sw1ptr) sw1ptr->onInterrupt();
}

void initDebugLed() {
	DDRD |= (1 << PD7);
	PORTD &= ~(1 << PD7);
}

void initISRsM5() {
	// Configuración para INT2 (PD2) flanco de bajada
	EICRA |= (1 << ISC21);
	EICRA &= ~(1 << ISC20);
	EIMSK |= (1 << INT2);
}

void initISRsM1() {
	// Habilitar interrupciones por cambio de pin en Puerto K
    PCICR |= (1 << PCIE2);    // Habilitar grupo PCINT2 (Puerto K)
    PCMSK2 |= (1 << PCINT22); // Habilitar pin específico PK6
}

void initPWMTimersM5() {
	// Timer 5 (16 bits) configurado en modo Fast PWM de 8 bits
	TCCR5A = (1 << COM5A1) | (1 << WGM51);
	TCCR5B = (1 << WGM52) | (1 << CS51) | (1 << CS50); // Prescaler 64
}

void initPWMTimersM1() {
    // Configurar Timer 2 para PWM rápido (Fast PWM)
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS22); // Prescaler 64 (difiere ligeramente del Timer 0)
}

// --- MAIN ---
extern "C" int main() {
	cli();
	sw5ptr = &sw5;
	sw1ptr = &sw1;
	
	initDebugLed();
	initISRsM1();
	initISRsM5();
	initPWMTimersM1();
	initPWMTimersM5();
	
	sw5.init();
	motor5.init();
	sw1.init();
	motor1.init();
	sei();

	// Arranque visual
	for(int i = 0; i < 3; i++) {
		PORTD |= (1 << PD7);
		_delay_ms(150);
		PORTD &= ~(1 << PD7);
		_delay_ms(150);
	}

	// Definiciones locales
	#define M5_ARRIBA 1
	#define M5_ABAJO 0
	#define M1_ARRIBA 1
	#define M1_ABAJO 0

	motor5.calibrate(M5_ABAJO);
	motor1.calibrate(M1_ABAJO);
	
	_delay_ms(100);

	motor5.goTo(M5_ARRIBA);
	while(motor5.getIsMoving()) {
		motor5.update();
	}
	
	motor5.goTo(M5_ABAJO);
	while(motor5.getIsMoving()) {
		motor5.update();
	}
	
	motor1.goTo(M1_ARRIBA);
	while(motor1.getIsMoving()){ //Mientras no esté arriba
		motor1.update();
	}

	motor1.goTo(M1_ABAJO);
	while(motor1.getIsMoving()){ //Mientras no esté abajo
		motor1.update();
	}
	
	while(1) {
		PORTD |= (1 << PD7);
		_delay_ms(150);
		PORTD &= ~(1 << PD7);
		_delay_ms(150);
	}
	return 0;
}