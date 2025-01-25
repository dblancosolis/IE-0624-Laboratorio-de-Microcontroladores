#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

enum Estado { espera, conf, cont, pausa, fin };

volatile enum Estado estadoActual = espera;
volatile uint8_t temporizadorActual = 0; // Índice del temporizador actual
volatile uint8_t contador[4] = {0, 0, 0, 0};
volatile uint8_t tiempos[4] = {0, 0, 0, 0};
const uint8_t pinesLED[4] = {PD1, PD3, PD4, PD5};
const uint8_t pinMotor = PB0;

void configuracion_inicial() {
    // Configurar botones como entrada con pull-up
    DDRD &= ~(1 << PD2); PORTD |= (1 << PD2); 
    DDRB &= ~((1 << PB5) | (1 << PB6) | (1 << PB7));
    PORTB |= ((1 << PB5) | (1 << PB6) | (1 << PB7));

    // Configurar pin del motor y LEDs como salida
    DDRB |= (1 << pinMotor); PORTB &= ~(1 << pinMotor);
    for (uint8_t i = 0; i < 4; i++) {
        DDRD |= (1 << pinesLED[i]);
        PORTD &= ~(1 << pinesLED[i]);
    }

    // Configurar pines BCD como salida
    DDRB |= 0x1E;

    // Configurar interrupción externa INT0
    MCUCR |= (1 << ISC01);
    GIMSK |= (1 << INT0);
    sei();
}

ISR(INT0_vect) {
    _delay_ms(20); 
    estadoActual = (estadoActual == cont) ? pausa : cont;
}

// Leer botones para configurar tiempos
void configurarTiempos() {
    if (!(PINB & (1 << PB5))) { _delay_ms(20); tiempos[0] = 1; tiempos[1] = 3; tiempos[2] = 2; tiempos[3] = 3; }
    else if (!(PINB & (1 << PB6))) { _delay_ms(20); tiempos[0] = 2; tiempos[1] = 5; tiempos[2] = 4; tiempos[3] = 5; }
    else if (!(PINB & (1 << PB7))) { _delay_ms(20); tiempos[0] = 3; tiempos[1] = 7; tiempos[2] = 5; tiempos[3] = 6; }
}

// Actualizar salida BCD
void actualizarBCD(uint8_t valor) {
    PORTB = (PORTB & 0xE1) | (valor << 1);
}

// Controlar LEDs y motor
void controlarHardware(uint8_t led, uint8_t motor) {
    for (uint8_t i = 0; i < 4; i++) PORTD &= ~(1 << pinesLED[i]); // Apagar todos los LEDs
    if (led < 4) PORTD |= (1 << pinesLED[led]); // Encender LED correspondiente
    if (motor) PORTB |= (1 << pinMotor); else PORTB &= ~(1 << pinMotor); // Controlar motor
}

// Manejar el temporizador actual
void manejarTemporizador() {
    if (contador[temporizadorActual] < tiempos[temporizadorActual]) {
        _delay_ms(1000);
        contador[temporizadorActual]++;
        actualizarBCD(contador[temporizadorActual]);
    } else {
        contador[temporizadorActual] = 0; // Reiniciar contador
        actualizarBCD(0);
        temporizadorActual++; // Pasar al siguiente temporizador
        if (temporizadorActual >= 4) estadoActual = fin;
    }
}

int main(void) {
    configuracion_inicial();

    while (1) {
        if (estadoActual == espera) {
            configurarTiempos();
            if (tiempos[0] > 0) estadoActual = conf;
        }

        switch (estadoActual) {
            case conf:
                actualizarBCD(0);
                break;

            case cont:
                controlarHardware(temporizadorActual, (temporizadorActual > 0)); // Motor activo en temporizadores 2, 3 y 4
                manejarTemporizador();
                break;

            case pausa:
                controlarHardware(4, 0); // Apagar LEDs y motor
                break;

            case fin:
                controlarHardware(4, 0);
                _delay_ms(1000);
                for (uint8_t i = 0; i < 4; i++) contador[i] = tiempos[i] = 0; // Reiniciar variables
                temporizadorActual = 0;
                estadoActual = espera;
                break;
            default:
                break;
        }
    }
    return 0;
}
