#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
uint8_t marcador = 0;

void mostrar_numero(uint8_t num) {
	PORTL &= ~((1 << PL0) | (1 << PL1) | (1 << PL2) | (1 << PL4) | (1 << PL5) | (1 << PL6));
	PORTD &= ~(1 << PD6);
	uint8_t ref;
	if(num >= 10){
		ref = num % 10;
		} else {
		ref = num;
	}
	switch(ref) {
		case 0: // Segmentos a,b,c,d,e,f
		PORTL |= (1 << PL0) | (1 << PL1) | (1 << PL2) | (1 << PL4) | (1 << PL5);
		PORTD |= (1 << PD6);
		break;
		case 1: // Segmentos b,c
		PORTL |= (1 << PL1) | (1 << PL2);
		break;
		case 2: // Segmentos a,b,d,e,g
		PORTL |= (1 << PL0) | (1 << PL1) | (1 << PL4) | (1 << PL6);
		PORTD |= (1 << PD6);
		break;
		case 3: // Segmentos a,b,c,d,g
		PORTL |= (1 << PL0) | (1 << PL1) | (1 << PL2) | (1 << PL6);
		PORTD |= (1 << PD6);
		break;
		case 4: // Segmentos b,c,f,g
		PORTL |= (1 << PL1) | (1 << PL2) | (1 << PL5) | (1 << PL6);
		break;
		case 5: // Segmentos a,c,d,f,g
		PORTL |= (1 << PL0) | (1 << PL2) | (1 << PL5) | (1 << PL6);
		PORTD |= (1 << PD6);
		break;
		case 6: //Segmentos a,c,d,e,f,g
		PORTL |= (1 << PL0) | (1 << PL2) | (1 << PL5) | (1 << PL6) | (1 << PL4);
		PORTD |= (1 << PD6);
		break;
		case 7:
		PORTL |= (1 << PL0) | (1 << PL1) | (1 << PL2) ;
		break;
		case 8:
		PORTL |= ((1 << PL0) | (1 << PL1) | (1 << PL2) | (1 << PL4) | (1 << PL5) | (1 << PL6));
		PORTD |= (1 << PD6);
		break;
		case 9:
		PORTL |= ((1 << PL0) | (1 << PL1) | (1 << PL2) | (1 << PL5) | (1 << PL6));
		PORTD |= (1 << PD6);
		break;
	}
}

void espera(uint8_t num) {
	PORTL &= ~((1 << PL0) | (1 << PL1) | (1 << PL2) | (1 << PL4) | (1 << PL5) | (1 << PL6));
	PORTD &= ~(1 << PD6);

	switch(num) {
		case 0:
		PORTL |= (1 << PL0);
		break;
		case 1:
		PORTL |= (1 << PL1);
		break;
		case 2:
		PORTL |= (1 << PL2);
		break;
		case 3:
		PORTD |= (1 << PD6);
		break;
		case 4:
		PORTL |= (1 << PL4);
		break;
		case 5:
		PORTL |= (1 << PL5);
		break;
	}
}


int main(void) {
	DDRL |= (1 << PL0) | (1 << PL1) | (1 << PL2) | (1 << PL4) | (1 << PL5) | (1 << PL6);
	DDRD |= (1 << PD6) | (1 << PD7);
	
	DDRD &= ~(1 << PD3);
	PORTD |= (1 << PD3);
	EICRA |= (1 << ISC31);
	EICRA &= ~(1 << ISC30);
	EIMSK |= (1 << INT3);
	
	DDRK &= ~0x3F;
	PORTK |= 0x3F;

	sei();

	while (1) {
	}
	return 0;
}

ISR(INT3_vect) {
	uint8_t estado_bolos;
	uint8_t mascara_bolos = 0b00111111;
	uint8_t frame_espera = 0;
	uint16_t millis_local = 0;

	PORTD |= (1 << PD7);
	while (millis_local <= 5000) {
		estado_bolos = (PINK & 0x3F);
		for(int i=0; i < 6; i++) {
			if (!(estado_bolos & (1 << i)) && (mascara_bolos & (1 << i))) {
				marcador++;
				mascara_bolos &= ~(1 << i);
			}
		}
		if(millis_local % 100 == 0) {
			espera(frame_espera);
			frame_espera++;
			if(frame_espera > 5) frame_espera = 0;
		}
		_delay_ms(1);
		millis_local++;
	}
	mostrar_numero(marcador);
	PORTD &= ~(1 << PD7);
	EIFR |= (1 << INTF3);
}