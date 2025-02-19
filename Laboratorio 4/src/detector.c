#include "labo.h"

int main(void)
{   
    uint8_t temperature;
    double battery;
    bool USART_enable = true;

    data xyzData;
    INIT_ANGLE(xyzData.angle);
    INIT_SAMPLE_TIME(xyzData.lastTime);

    // Configuración de periféricos 
	clock_setup();
	console_setup(115200); 
    spi_init();
	sdram_init();
	lcd_spi_init();
    button_init();
    battery_init();
    gfx_init(lcd_draw_pixel, 240, 320);


	while (1) 
    {   
        // Toma de datos 
        temperature = mems_temp();
        xyzData.reading = read_xyz();
        xyzData = integrate_xyz(xyzData);
        battery = battery_get(5); // Use channel 5

        // Alertas 
        five_degree_alert(xyzData);
        alert_battery(battery);

        // Envío de datos 
        USART_enable = console_usart_enable(xyzData, temperature, battery, USART_enable); 
        lcd_slope(temperature, xyzData.angle, battery, USART_enable); 
	}
}
