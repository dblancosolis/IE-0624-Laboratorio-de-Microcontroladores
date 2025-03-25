#include <pic14/pic12f683.h>
#include <stdint.h>

// Configurar para deshabilitar WDT y MCLR.
typedef unsigned int word;
word __at 0x2007 __CONFIG = (_WDTE_OFF & _MCLRE_OFF);

// Declaración de la función de retraso
void delay(unsigned int tiempo);

// Función para inicializar el PIC12f683
void initPIC12f683() {
    TRISIO = 0x10;  // Se configuran GPIO0-GPIO3 y GPIO5 como salida, GPIO4 como entrada
    ANSEL = 0x00;   // Se configuran todos los pines como digitales
    GPIO = 0x00;    // Se inicializan todos los pines en estado bajo
}

// Se establece el valor inicial de LFSR para generar números pseudoaleatorios.
static uint16_t lfsr = 0xABCDu;

// Función para generar un número pseudoaleatorio entre 1 y 6 usando LFSR
uint8_t generarNumeroAleatorio() {
    unsigned lsb = lfsr & 0x01;  // Se obtiene el bit menos significativo
    lfsr >>= 1;                  // Se desplaza el LFSR a la derecha
    if (lsb) {
        lfsr ^= 0xEF01u;         // Se aplica XOR con la máscara si el bit menos significativo es 1
    }
    return (lfsr % 6) + 1;       // Se retorna un valor entre 1 y 6
}

// Función para  mostrar un número en los LEDs 
void mostrarNumero(int numero) {
    GPIO &= 0b11100000;  // Limpiar los bits de GPIO0-GPIO5, para apagar todos los LEDs antes de mostrar un número

    // Casos para encender los LEDs correspondientes 
    switch (numero) {
        case 1:
            GPIO |= 0b11100001;  // Encender LED para el número 1
            break;
        case 2:
            GPIO |= 0b11100010;  // Encender LEDs para el número 2
            break;
        case 3:
            GPIO |= 0b11100011;  // Encender LEDs para el número 3
            break;
        case 4:
            GPIO |= 0b11100110;  // Encender LEDs para el número 4
            break;
        case 5:
            GPIO |= 0b11100111;  // Encender LEDs para el número 5
            break;
        case 6:
            GPIO |= 0b11101110;  // Encender LEDs para el número 6
            break;
        default:
            break;
    }
}

void main() {
    initPIC12f683();  // Inicializar el microcontrolador

    while (1) {
        // Ciclo para esperar hasta que el botón sea presionado
        while (GP4 == 0) {
            lfsr = lfsr + 1;  // Incrementar LFSR para variar valor inicial 
        }

        int numero = generarNumeroAleatorio(); // Generar un número aleatorio entre 1 y 6
        mostrarNumero(numero);                 // Mostrar el número en los LEDs
        delay(500);                            // Retraso de 500 ms para visualizar el número
        GPIO = 0x00;                           // Apagar los LEDs
    }
}

// Función para aplicar retraso (bloqueante)
void delay(unsigned int tiempo) {
    unsigned int i, j;
    for (i = 0; i < tiempo; i++) {
        for (j = 0; j < 1275; j++);
    }
}