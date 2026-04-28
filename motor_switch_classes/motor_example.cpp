#include <avr/io.h>
#include <avr/interrupt.h>
#include "Timer.h"
#include "SwitchUser.h"
#include "SwitchMotor.h"
#include "Motor.h"

Timer systemTimer;
Timer* timerPtr = 0;
ISR(TIMER4_COMPA_vect) {
    if (timerPtr) timerPtr->addTick();
}

SwitchUser sw6(&PIND, &PORTD, &DDRD, (1<<PD3));
SwitchMotor sw3(&PIND, &PORTD, &DDRD, (1<<PD0));

Motor m3(&PORTB, &DDRB, (1<<PB2),
         &PORTB, &DDRB, (1<<PB6),
         &OCR1BL, &sw3);

int main(void) {
    timerPtr = &systemTimer;

    cli();
    
    // Timer 1 (M3 - OCR1B)
    TCCR1A |= (1 << COM1B1) | (1 << WGM10);
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);
    OCR1BH = 0;
    OCR1BL = 0;
    
    systemTimer.init();
    sw6.init();
    sw3.init();
    m3.init();
    
    sei();

    // Arranca calibración automáticamente hacia SIDE_1
    uint32_t currentMillis = systemTimer.millis();
    m3.calibrate(true, currentMillis);

    while (1) {
        currentMillis = systemTimer.millis();

        sw6.update(currentMillis);
        sw3.update(currentMillis);
        m3.update(currentMillis);

        // SW6 relanza la calibración manualmente si se quiere repetir
        if (sw6.consumeClick()) {
            m3.calibrate(true, currentMillis);
        }
    }

    return 0;
}