#include <avr/io.h>
#define F_CPU 8000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "Display.h"

// Define the hardware mapping for the 7 segments (a, b, c, d, e, f, g)
volatile uint8_t* segPorts[7] = {&PORTL, &PORTL, &PORTL, &PORTD, &PORTL, &PORTL, &PORTL};
volatile uint8_t* segDdrs[7]  = {&DDRL,  &DDRL,  &DDRL,  &DDRD,  &DDRL,  &DDRL,  &DDRL};
uint8_t segMasks[7]           = {(1<<PL0), (1<<PL1), (1<<PL2), (1<<PD6), (1<<PL4), (1<<PL5), (1<<PL6)};

// Create the Display object
Display display(
    segPorts, 
    segDdrs, 
    segMasks, 
    &PORTL, &DDRL, (1<<PL7) // The DS pin parameters
);

Display* displayPtr = 0;

ISR(TIMER3_COMPA_vect) {
    if (displayPtr) {
        displayPtr->refresh();
    }
}

void initTimer3_1ms() {
    TCCR3B |= (1 << WGM32); // CTC mode
    TCCR3B |= (1 << CS31) | (1 << CS30); // Prescaler 64
    OCR3A = 124; // 8MHz / (64 * 1000) - 1 = 124
    TIMSK3 |= (1 << OCIE3A); // Enable Timer3 Compare Match A interrupt
}

int main() {
    displayPtr = &display;
    cli();
    display.init();
    initTimer3_1ms();
    sei();

    while (1)
    {
        _delay_ms(1000);
        display.addPoint();
    }
    return 0;
    
}