#include "labo.h"

// Información en LCD 
void lcd_slope(uint8_t temperature, degree reading, double battery, bool USART_enable)
{
    char buf[21];

    // Interfaz gráfica
    gfx_fillScreen(LCD_WHITE);

    // Encabezado
    gfx_setTextSize(2.5);
    gfx_setCursor(35, 25);
    gfx_puts("MONITOR DE");
    gfx_setCursor(35, 50);
    gfx_puts("PENDIENTES");

    // Batería y temperatura
    gfx_setTextSize(2);
    gfx_setCursor(15, 95);
    gfx_puts("TEMP:");
    gfx_setCursor(85, 95);
    snprintf(buf, sizeof(buf), "%3d C", temperature);
    gfx_puts(buf);

    // Ejes X, Y y Z
    gfx_setCursor(15, 133);
    gfx_puts("X:");
    gfx_setCursor(65, 133);
    snprintf(buf, sizeof(buf), "%3.2f", reading.x);
    gfx_puts(buf);

    gfx_setCursor(15, 171);
    gfx_puts("Y:");
    gfx_setCursor(65, 171);
    snprintf(buf, sizeof(buf), "%3.2f", reading.y);
    gfx_puts(buf);

    gfx_setCursor(15, 209);
    gfx_puts("Z:");
    gfx_setCursor(65, 209);
    snprintf(buf, sizeof(buf), "%3.2f", reading.z);
    gfx_puts(buf);
    
    gfx_setCursor(15, 247);
    gfx_puts("BAT:");
    gfx_setCursor(90, 247);
    snprintf(buf, sizeof(buf), "%3.2fV", battery);
    gfx_puts(buf);

    // Comunicación serial
    gfx_setCursor(15, 285);
    gfx_puts(USART_enable ? "COM SER: ON" : "COMM SER: OFF");

    lcd_show_frame();
}

// Imprimir datos en consola 
void console_puts_all(data xyzData, uint8_t temperature, double battery)
{
    char buf[21];

    snprintf(buf, sizeof(buf), "%3.3f\t%3.3f\t%3.3f\t%3d\t%3.2f\n",
             xyzData.angle.x, xyzData.angle.y, xyzData.angle.z, temperature, battery);
    console_puts(buf);
}

// Comunicación USART 
bool console_usart_enable(data xyzData, uint8_t temperature, double battery, bool USART_enable)
{
    if (gpio_get(GPIOA, GPIO0))
    {
        USART_enable = !USART_enable;
        msleep(10);
    }
    
    if (USART_enable)
    {
        console_puts_all(xyzData, temperature, battery);
    }

    return USART_enable;
}

void delay(void)
{
    for (int i = 0; i < 6000000; i++) 
        __asm__("nop");
}

// Eje individual 
integral integrate_axis(double reading, double angle, double lastSampleTime)
{
    integral speed_integral;

    if (fabs(reading) > 1 && (mtime() - lastSampleTime) > SAMPLE_TIME) 
    {
        double deltaT = ((double)mtime() - lastSampleTime) / 1000.0; 
        angle += fabs(reading) * deltaT;
        lastSampleTime = mtime();
    }
    
    speed_integral.angle = angle;
    speed_integral.lastSampleTime = lastSampleTime;
    return speed_integral;
}

// Los tres ejes 
data integrate_xyz(data xyzData)
{
    xyzData.angle.x = integrate_axis(xyzData.reading.x, xyzData.angle.x, xyzData.lastTime.x).angle;
    xyzData.lastTime.x = integrate_axis(xyzData.reading.x, xyzData.angle.x, xyzData.lastTime.x).lastSampleTime;

    xyzData.angle.y = integrate_axis(xyzData.reading.y, xyzData.angle.y, xyzData.lastTime.y).angle;
    xyzData.lastTime.y = integrate_axis(xyzData.reading.y, xyzData.angle.y, xyzData.lastTime.y).lastSampleTime;

    xyzData.angle.z = integrate_axis(xyzData.reading.z, xyzData.angle.z, xyzData.lastTime.z).angle;
    xyzData.lastTime.z = integrate_axis(xyzData.reading.z, xyzData.angle.z, xyzData.lastTime.z).lastSampleTime;

    return xyzData;
}

// Si algún eje supera 5 grados 
void five_degree_alert(data xyzData)
{
    if (xyzData.angle.x > 5 || xyzData.angle.y > 5 || xyzData.angle.z > 5)
    {
        gpio_toggle(GPIOG, GPIO13);
    }
}
