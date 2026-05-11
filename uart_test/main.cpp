#include <avr/io.h>
#include "UART.h"
#include "Timer.h"

Timer timer;
UART uart;

Timer* timerPtr = 0;
ISR(TIMER4_COMPA_vect) { if(timerPtr) timerPtr->addTick(); }

UART* uartPtr = 0;
ISR(USART0_UDRE_vect) { if(uartPtr) uartPtr->isrUDRE(); }

uint32_t currentTime;
uint32_t timeStamp;
uint8_t throwCount;  // Número de lanzamientos simulados (0–7)

// Datos de simulación: 8 lanzamientos con pines y puntuación
const char* pins_sim[8] = {
	"101010", "110100", "111111", "000000",
	"100001", "010101", "001110", "111000"
};
const uint8_t points_sim[8] = { 3, 3, 6, 0, 2, 3, 3, 3 };

void setup() {
	timerPtr = &timer;
	uartPtr  = &uart;

	timer.init();
	uart.init(9600);

	timeStamp  = 0;
	throwCount = 0;

	uart.print("START_GAME\n");
}

void loop() {
	currentTime = timer.millis();

	if (throwCount < 8 && (currentTime - timeStamp) >= 2000) {
		timeStamp = currentTime;

		uart.print("B:");
		uart.print(pins_sim[throwCount]);
		uart.print("\n");

		uart.print("P:");
		uart.send(points_sim[throwCount] + '0');
		uart.send('\n');

		throwCount++;
	}
}

int main() {
	setup();
	while (1) { loop(); }
}