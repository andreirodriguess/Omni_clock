#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "string.h"
// Definição dos pinos dos LEDs
#define GREEN_PIN 11
#define BLUE_PIN 12
#define RED_PIN 13

// Definição dos pinos do joystick e do botão
#define VRX_PIN 27
#define VRY_PIN 26
#define PB 22      // Pino para o botão do joystick
#define button_a 5 // Pino para o botão A
#define button_b 6 // Pino para o botão B

// Definição dos pinos do display I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C // Endereço do display OLED

float get_duty_cycle(uint16_t value) // função que calcula o duty cycle
{
    if (value > 2150)
    {

        return (value - 2047) * 2;
    }
    else if (value < 1800)
    {
        return (2047 - value) * 2;
    }
    else
    {
        return 0;
    }
}

// Função para inicializar o PWM em um pino GPIO
uint pwm_init_gpio(uint gpio, uint wrap)
{
    gpio_set_function(gpio, GPIO_FUNC_PWM);       // Configura o pino GPIO para função PWM
    uint slice_num = pwm_gpio_to_slice_num(gpio); // Obtem o número do slice PWM associado ao pino
    pwm_set_wrap(slice_num, wrap);                // Define o valor de "wrap", que controla a resolução do PWM
    pwm_set_enabled(slice_num, true);             // Habilita o PWM no slice
    return slice_num;                             // Retorna o número do slice PWM
}

void buttons_init()
{
    gpio_init(PB);                   // Inicializa o pino do botão do joystick
    gpio_set_dir(PB, GPIO_IN);       // Define o pino do botão como entrada
    gpio_pull_up(PB);                // Habilita o resistor pull-up no botão
    gpio_init(button_a);             // Inicializa o pino do botão A
    gpio_set_dir(button_a, GPIO_IN); // Define o pino do botão A como entrada
    gpio_pull_up(button_a);          // Habilita o resistor pull-up no botão A
    gpio_init(button_b);             // Inicializa o pino do botão B
    gpio_set_dir(button_b, GPIO_IN); // Define o pino do botão B como entrada
    gpio_pull_up(button_b);          // Habilita o resistor pull-up no botão B
}

static volatile uint hora_num, minutos_num;
static char hora_char[3], minutos_char[3];

void get_hora()
{
    while (true)
    {
        printf("Digite o valor das horas: \n");
        for (int c = 0; c < 2; c++)
        {
            hora_char[c] = getchar();
            if (hora_char[c] < '0' || hora_char[c] > '9') // se não for um numero
            {
                printf("valor invalido digite novamente");
                c--;
            }
            hora_num = atoi(hora_char);
            printf("%d", hora_num);
        }

        printf("Digite o valor dos minutos: \n");
        for (int c = 0; c < 2; c++)
        {
            minutos_char[c] = getchar();
            if (minutos_char[c] >= 0 && minutos_char[c] <= 9)
                minutos_num = atoi(minutos_char);
        }
        if (hora_num <= 24 && minutos_num <= 60)
            printf("Hora definida: %s:%s", hora_char, minutos_char);
    }
}