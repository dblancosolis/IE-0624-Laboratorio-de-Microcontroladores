#include "battery.h"
#include <libopencm3/stm32/rcc.h>

void battery_init(void)
{
	rcc_periph_clock_enable(RCC_ADC1);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);
	adc_power_off(ADC1);
	adc_disable_scan_mode(ADC1); 
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);
	adc_power_on(ADC1);
}

uint16_t battery_read(uint8_t channel)
{
	uint8_t channel_array[16];
	channel_array[0] = channel;
	adc_disable_external_trigger_regular(ADC1);
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_start_conversion_regular(ADC1);
	while (!adc_eoc(ADC1));
	uint16_t reg16 = adc_read_regular(ADC1);
	return reg16;
}

double battery_get(uint8_t channel)
{
    return (double)battery_read(channel) * (9.0/4096.0);
}

void alert_battery(double battery)
{
        if (battery < 7.0)
        {
		    gpio_toggle(GPIOG, GPIO14);
        }
        else {
            gpio_clear(GPIOG, GPIO14);
        }
}
