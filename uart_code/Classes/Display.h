#pragma once
#include <avr/io.h>

class Display {
    /*
     * ==== Display ====
     * Controla el display 7 segmentos de 2 dígitos mediante multiplexación por software.
     * Puede mostrar un score numérico (0-99) o el número de jugador con prefijo "P".
     */
    private:
    volatile uint8_t* portData[7];
    volatile uint8_t* ddrData[7];
    uint8_t maskData[7];

    volatile uint8_t* portDS;
    volatile uint8_t* ddrDS;
    uint8_t maskDS;

    uint8_t currentScore;
    uint8_t currentPlayer;
    bool showPlayerMode;   // true = muestra "P+jugador", false = muestra score
    bool showTens;

    // Segmentos: bit0=a, bit1=b, bit2=c, bit3=d, bit4=e, bit5=f, bit6=g
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

    // P: segmentos a, b, e, f, g  →  0b1110011
    const uint8_t LETTER_P = 0b01110011;

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
        for (uint8_t i = 0; i < 7; i++) {
            portData[i] = pData[i];
            ddrData[i] = dData[i];
            maskData[i] = mData[i];
        }

        portDS = pDS;
        ddrDS  = dDS;
        maskDS = mDS;

        currentScore  = 0;
        currentPlayer = 1;
        showPlayerMode = false;
        showTens = false;
        lastTime = 0;
    }

    void init() {
        for (uint8_t i = 0; i < 7; i++) {
            *ddrData[i]  |=  maskData[i];
            *portData[i] &= ~maskData[i];
        }
        *ddrDS  |=  maskDS;
        *portDS &= ~maskDS;
    }

    void clear() {
        for (uint8_t i = 0; i < 7; i++) {
            *portData[i] &= ~maskData[i];
        }
    }

    void printBinary(uint8_t bin) {
        for (uint8_t i = 0; i < 7; i++) {
            if (bin & (1 << i)) {
                *portData[i] |=  maskData[i];
            } else {
                *portData[i] &= ~maskData[i];
            }
        }
    }

    void printDigit(uint8_t digit) {
        if (digit > 9) return;
        printBinary(digits[digit]);
    }

    // --- Modos ---

    void setScore(uint8_t score) {
        if (score > 99) score = 99;
        currentScore   = score;
        showPlayerMode = false;
    }

    void setPlayer(uint8_t player) {
        if (player > 9) player = 9;   // el dígito de unidades solo aguanta 1 cifra
        currentPlayer  = player;
        showPlayerMode = true;
    }

    // --- Refresco ---

    void refresh() {
        clear();

        if (showTens) {
            // Dígito izquierdo
            *portDS |= maskDS;
            if (showPlayerMode) {
                printBinary(LETTER_P);          // muestra "P"
            } else {
                printDigit(currentScore / 10);  // muestra decena del score
            }
        } else {
            // Dígito derecho
            *portDS &= ~maskDS;
            if (showPlayerMode) {
                printDigit(currentPlayer);      // muestra número de jugador
            } else {
                printDigit(currentScore % 10);  // muestra unidad del score
            }
        }

        showTens = !showTens;
    }

    void update(uint32_t currentTime, bool endGame) {
        if (endGame && ((currentTime % 1000) >= 900)) {
            clear();
        } else {
            if (currentTime - lastTime >= refreshInterval) {
                refresh();
                lastTime = currentTime;
            }
        }
    }
};