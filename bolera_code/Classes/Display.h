/* Display.h
 * Controla el display 7 segmentos de 2 dgitos mediante multiplexación.
 */

#pragma once
#include <avr/io.h>

class Display {
    private:
    volatile uint8_t* portData[7];
    volatile uint8_t* ddrData[7];
    uint8_t maskData[7];
    
    volatile uint8_t* portDS;
    volatile uint8_t* ddrDS;
    uint8_t maskDS;
    
    uint8_t currentScore;
    bool showTens;
    
    const uint8_t digits[10] = {
        0b00111111, // 0
        0b00000110, // 1
        0b01011011, // 2
        0b01001111, // 3
        0b01100110, // 4
        0b01101101, // 5
        0b01111101, // 6
        0b00000111, // 7
        0b01111111, // 8
        0b01101111  // 9
    };
    
    uint32_t lastTime;
    const uint32_t refreshInterval = 5;
    
    public:
    Display(
        volatile uint8_t* pData[7],
        volatile uint8_t* dData[7],
        uint8_t mData[7],
        volatile uint8_t* pDS,
        volatile uint8_t* dDS,
        uint8_t mDS
        
    ) {
        for (uint8_t i = 0; i<7; i++) {
            portData[i] = pData[i];
            ddrData[i]  = dData[i];
            maskData[i] = mData[i];
        }
        
        portDS = pDS;
        ddrDS  = dDS;
        maskDS = mDS;
        
        currentScore = 0;
        showTens = false;
    }
    
    void init() {
        for (uint8_t i = 0; i<7; i++) {
            *ddrData[i] |= maskData[i];
            *portData[i] &= ~maskData[i];
        }
        
        *ddrDS  |=  maskDS;
        *portDS &= ~maskDS;
    }
    
    void clear() {
        // Apagar todos los segmentos
        for (uint8_t i = 0; i<7; i++) {
            *portData[i] &= ~maskData[i];
        }
    }
    
    void printDigit(uint8_t digit) {
        // Mostrar un dígito (0-9) en el display.
        if (digit>9) return;
        uint8_t pattern = digits[digit];
        
        for (uint8_t i=0; i<7; i++) {
            if (pattern & (1<<i)) {
                *portData[i] |= maskData[i];
            } else {
                *portData[i] &= ~maskData[i];
            }
        }
    }
    
    void setScore(uint8_t score) {
        // Establece el puntaje actual (número a mostrar).
        if (score > 99) score = 99;
        currentScore = score; 
    }
    
    void addPoint() {
        if (currentScore < 99) currentScore++;
    }
    
    void resetScore() {
        currentScore=0;
    }
    
    void refresh() {
        // Actualiza el display mostrando el dígito correspondiente.
        clear();
        if (showTens) {
            uint8_t tens = currentScore/10;
            *portDS |= maskDS;
            printDigit(tens);
        } else {
            uint8_t units = currentScore%10;
            *portDS &= ~maskDS;
            printDigit(units);
        }
        
        showTens = !showTens;
    }
    
    void update(uint32_t currentTime, bool endGame) {
        /*
         * Actualiza el display según el tiempo y el estado del juego.
         * Si endGame, parpadea el puntaje.
         */
        if (endGame && ((currentTime%1000) >= 900)) {
            clear(); 
        } else {
            if (currentTime - lastTime >= refreshInterval) {
                refresh();
                lastTime = currentTime;
            }
        }
    }
    
    };