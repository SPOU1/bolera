# Proyecto Bolera - Sistemas Microprocesadores


Este repositorio contiene el código fuente en C++ para el sistema de control de una pista de bolera automatizada, diseñado para ejecutarse en un microcontrolador **ATmega640**.

El sistema gestiona la recogida de bolas, el armado del mecanismo de lanzamiento, la detección de bolos derribados mediante sensores ópticos, el control de la puntuación en un display de 7 segmentos y la lógica de juego a través de una máquina de estados finitos (FSM) no bloqueante.

## 📂 Estructura del Repositorio

El proyecto está dividido modularmente para facilitar su mantenimiento y comprensión:

* **/bolera_code**: Es el **código principal y final** del proyecto. Integra la máquina de estados, la instanciación de objetos y la función `main`.
* **/bolera_code/Classes**: Contiene las implementaciones aisladas de todas las clases de hardware y utilidades (`Motor.h`, `Motor2.h`, `SwitchMotor.h`, `PinsManager.h`, `Display.h`, `Led.h`, `Timer.h`).
* **/bolera_code/StateMachine**: Contiene la lógica de los estados del juego (`state_machine.h`, `state_actions.h`, `status.h`).
* **Otras carpetas** (`display_class`, `motor_switch_classes`, `classes_combined`,...): Contienen pruebas unitarias y códigos de ejemplo para probar los módulos por separado.

## ⚙️ Especificaciones de Hardware y Control

El código hace uso avanzado de los periféricos del ATmega640:

### 1. Control de Motores (PWM)
El sistema controla 5 motores de DC usando señales PWM configuradas a ~488 Hz:
* **M1 (Elevador Carga):** Timer 2 (OC2A) - 8 bits Fast PWM.
* **M2 (Lanzador):** Timer 1 (OC1AL) - 16 bits en Modo 5 (Fast PWM 8 bits).
* **M3 (Vástago):** Timer 1 (OC1BL) - 16 bits en Modo 5.
* **M4 (Retención):** Timer 0 (OC0A) - 8 bits Fast PWM.
* **M5 (Recogida bolos):** Timer 5 (OC5AL) - 16 bits en Modo 5.

### 2. Sensores y Entradas
* **Microinterruptores (Fines de carrera):** Gestionados por las clases `SwitchMotor` y `SwitchMotor2` (para 3 posiciones). Todas incluyen **filtro antirrebote por software (debounce)** de 20-50ms para evitar lecturas falsas originadas por la vibración mecánica.
* **Sensores Ópticos de Bolos:** Conectados al Puerto K (PK0-PK5), utilizan la interrupción por cambio de estado (`PCINT2` vector, pines `PCINT16` a `PCINT21`). La clase `PinsManager` asegura que cada bolo solo sume un punto por derribo.

## 🧠 Lógica del Sistema (Máquina de Estados)

El flujo principal está gobernado por un sistema orientado a objetos no bloqueante, estructurado en los siguientes estados:

1.  **Init:** Configuración de puertos, PWM, interrupciones y periféricos.
2.  **Calibration:** Los motores buscan su posición "cero" o punto de referencia usando los fines de carrera.
3.  **Idle:** Espera a que el usuario inicie la partida pulsando el botón SW6.
4.  **Carga:** Eleva la bola hacia la rampa de lanzamiento.
5.  **Armado:** El lanzador oscila. El jugador tiene 30 segundos (o menos si es el último turno, donde el LED parpadeará) para pulsar SW6.
6.  **Disparo:** Se libera el mecanismo de retención (M4). Aprovechando el tiempo de viaje de la bola, el sistema rearma internamente el vástago para agilizar el siguiente turno.
7.  **Retorno:** El sistema vuelve a la posición de espera, gestionando la recogida de bolos.

## 🛠️ Guía de Configuración para Microchip Studio

Para poder compilar este proyecto correctamente en Microchip Studio / Atmel Studio, debes realizar las siguientes configuraciones en tu proyecto (`bolera_code`):

### 1. Activar el estándar C++11
El código utiliza características modernas de C++ (como `enum class`). 
* Ve a **Project -> Properties -> Toolchain -> AVR/GNU C++ Compiler -> Miscellaneous**.
* En *Other flags*, añade (separado por un espacio): `-std=gnu++11`

### 2. Configurar los directorios de inclusión (Include Paths)
Para que los archivos `#include` detecten correctamente las carpetas:
* Ve a **Project -> Properties -> Toolchain -> AVR/GNU C++ Compiler -> Directories**.
* Añade un nuevo elemento (Add Item) marcando la casilla de *Relative Path* y escribe un punto: `.`

### 3. Evitar el error "undefined reference to operator delete"
Los microcontroladores AVR no soportan memoria dinámica (`new`/`delete`) por defecto. Este proyecto no la necesita. Si trasladas el código a otro proyecto y te da este error en el enlazador (Linker), asegúrate de **NO** usar destructores marcados como `virtual` en las clases base (como `StateActionBase`).

## 💡 Notas Adicionales
* **Lógica No Bloqueante:** Ningún estado contiene bucles `while` infinitos ni `_delay_ms()`. Todas las pausas y temporizaciones se manejan leyendo los milisegundos del sistema (`Timer::millis()`) en el método `update()` de cada iteración, permitiendo que el hardware multiplexado (como el Display de 7 segmentos) parpadee sin interrupciones a 200 Hz.