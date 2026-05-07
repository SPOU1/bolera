#pragma once
#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#define TX_BUFFER_SIZE 64

class UART {
    private:
    char txBuffer[TX_BUFFER_SIZE];
    uint8_t txHead;
    uint8_t txTail;

    public:
    UART() {
        txHead = 0;
        txTail = 0;
    }

    void init(uint32_t baud) {
		uint32_t ubrr = F_CPU / 16 / baud - 1;
		UBRR0H = (unsigned char) (ubrr >> 8);
		UBRR0L = (unsigned char) ubrr;
		UCSR0B = (1 << TXEN0);
		UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

        txHead = 0;
        txTail = 0;
    }

    void send(char c) {
        uint8_t nextHead = (txHead + 1) % TX_BUFFER_SIZE; 
        
        if (nextHead == txTail) {
            // Buffer lleno. Descartar caracter.
            return;
        }

        txBuffer[txHead] = c;
        txHead = nextHead;

        UCSR0B |= (1 << UDRIE0); // Habilitar interrupción "Data Register Empty"
	//	while (!(UCSR0A & (1 << UDRE0)));
	//	UDR0 = c;
    }

    void print(const char* s) {
        while (*s) send(*s++);
    }

    void isrUDRE() {
        if (txHead != txTail) {
            UDR0 = txBuffer[txTail];
            txTail = (txTail + 1) % TX_BUFFER_SIZE;
        } else {
            UCSR0B &= ~(1 << UDRIE0); // Deshabilitar interrupción si no hay más datos
        }
    }
};