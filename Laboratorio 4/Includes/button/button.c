#include "button.h"

void button_init(void)
{
	// Reloj GPIOG.
	rcc_periph_clock_enable(RCC_GPIOG);

	// GPIO13 a output push-pull.
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO14);
	// Habilita reloj GPIOA.
	rcc_periph_clock_enable(RCC_GPIOA);

	// GPIO0 a input open-drain.
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
}
