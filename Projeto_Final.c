#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "pico/bootrom.h"

// Outras bibliotecas necessárias para o funcionamento do display e ADC
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/secundary_functions.h"

void i2c_adc_gpio_init(); // Função de inicialização do ADC e I2C

void gpio_irq_handler(uint gpio, uint32_t events); // Função de interrupção do GPIO (botões)

uint32_t last_print_time = 0;           // Variável para controle de tempo de impressão (1 segundo)
uint32_t last_joystick_button_time = 0; // Controle de debounce do botão do joystick
uint32_t last_button_a_time = 0;        // Controle de debounce do botão A
uint32_t current_time = 0;
ssd1306_t ssd;                          // Estrutura do display SSD1306
uint tela = 1;                          // Variável para controle de tela
void logo();
void tela_0();
void tela_1();
void tela_2();

int main()
{
    stdio_init_all();    // Inicializa a comunicação serial (para debug)
    i2c_adc_gpio_init(); // Inicializa o ADC e o display SSD1306
    // Habilita interrupções para os botões com função de callback//lembrar de colocar interrupções dentro das respectivas telas
    gpio_set_irq_enabled_with_callback(PB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_b, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true)
    {
        switch (tela)
        {
        case 0:
        {
            logo();
            tela_0();
        }
        break;

        case 1:
            tela_1();
            break;
        }
    }
}
void logo()
{
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    ssd1306_draw_string(&ssd, "OMNI CLOCK", 22, 3);   // Escreve uma string no display
    ssd1306_draw_centered_image(&ssd, clock, 39, 14); // Desenha a imagem do logo no display
    ssd1306_send_data(&ssd);                          // Atualiza o display
    sleep_ms(1000);                                   // Aguarda 3s
}

void tela_0() // recebe a hora atual
{
    while (true)
    {
        if (stdio_usb_connected())
        {
            // Limpa o display (apaga todos os pixels)
            ssd1306_fill(&ssd, false);
            ssd1306_send_data(&ssd);
            ssd1306_draw_string(&ssd, "Envie a hora", 4, 2);    // Escreve uma string no display
            ssd1306_draw_string(&ssd, "atual", 4, 14);          // Escreve uma string no display
            ssd1306_draw_string(&ssd, "pelo monitor: ", 4, 26); // Escreve uma string no display
            ssd1306_draw_string(&ssd, "HH :MM", 4, 38);
            ssd1306_send_data(&ssd); // Atualiza o display
            get_hora();
            sleep_ms(1000);
            tela++;
            break;
        } // Aguarda 1s
        else
        {
            // Limpa o display (apaga todos os pixels)
            ssd1306_fill(&ssd, false);
            ssd1306_send_data(&ssd);
            ssd1306_draw_string(&ssd, "Conecte o cabo USB para continuar", 10, 3); // Escreve uma string no display
            ssd1306_send_data(&ssd);                                               // Atualiza o display
            sleep_ms(1000);                                                        // Aguarda 1s
            printf("Conecte o cabo USB para continuar\n");
        }
    }
}

void tela_1() // recebe o fuso atual
{
    bool cor = true; // Define a cor da animação
    ssd1306_fill(&ssd, !cor);
    ssd1306_draw_string(&ssd, "Marque o seu", 4, 2); // Escreve uma string no display
    ssd1306_draw_string(&ssd, "fuso atual", 4, 14);  // Escreve uma string no display
    ssd1306_draw_string(&ssd, "E aperte A", 4, 26);  // Escreve uma string no display
    ssd1306_send_data(&ssd);
    sleep_ms(2000);

    while (gpio_get(button_a))
    {
        adc_select_input(0);             // Seleciona o canal do eixo y
        uint16_t vry_value = adc_read(); // Lê o valor do eixo y
        
        adc_select_input(1);             // Seleciona o canal do eixo x
        uint16_t vrx_value = adc_read(); // Lê o valor do eixo x
        
        
        // Converte os valores de ADC para duty cycle do PWM (intensidade dos LEDs)
        float duty_cycle_red = get_duty_cycle(vrx_value);  // Cálculo para o LED vermelho
        float duty_cycle_blue = get_duty_cycle(vry_value); // Cálculo para o LED azul
        
        // Mapeamento dos valores do joystick para as coordenadas do display
        uint8_t x = map_x_to_display(vrx_value); // Converte o valor do eixo X para coordenada X do display
        uint8_t y = map_y_to_display(vry_value); // Converte o valor do eixo Y para coordenada Y do display
        current_time = to_us_since_boot(get_absolute_time());
        if(current_time - last_print_time >=3000000)
        {
            last_print_time = current_time;
            printf("valor de y: %d, valor de x: %d", y, x);
        }

        // pwm_set_gpio_level(RED_PIN, duty_cycle_red);   // Atualiza o PWM do LED vermelho
        // pwm_set_gpio_level(BLUE_PIN, duty_cycle_blue); // Atualiza o PWM do LED azul

        uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual

        // Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor);                                                              // Limpa o display
        ssd1306_draw_world_map(&ssd, epd_bitmap_mapa_mundi_pixell);                            // Desenha o mapa-múndi no display
        ssd1306_cross(&ssd, x, y, 15, !ssd1306_get_pixel(epd_bitmap_mapa_mundi_pixell, x, y)); // Desenha uma cruz no centro do display
        ssd1306_draw_string(&ssd, "Marque o Local", 10, 3);                                    // Escreve uma string no display
        select_fuso(&ssd, x, y);
        ssd1306_send_data(&ssd);                                                               // Atualiza o display
        sleep_ms(1);
    }
    printf("botao a pressionado");
}

void tela_2()                        //
{                                    // Funções ADC: leitura dos valores do joystick
    adc_select_input(0);             // Seleciona o canal do eixo y
    uint16_t vry_value = adc_read(); // Lê o valor do eixo y

    adc_select_input(1);             // Seleciona o canal do eixo x
    uint16_t vrx_value = adc_read(); // Lê o valor do eixo x

    // Converte os valores de ADC para duty cycle do PWM (intensidade dos LEDs)
    float duty_cycle_red = get_duty_cycle(vrx_value);  // Cálculo para o LED vermelho
    float duty_cycle_blue = get_duty_cycle(vry_value); // Cálculo para o LED azul

    // Mapeamento dos valores do joystick para as coordenadas do display
    uint8_t x = map_x_to_display(vrx_value); // Converte o valor do eixo X para coordenada X do display
    uint8_t y = map_y_to_display(vry_value); // Converte o valor do eixo Y para coordenada Y do display

    // pwm_set_gpio_level(RED_PIN, duty_cycle_red);   // Atualiza o PWM do LED vermelho
    // pwm_set_gpio_level(BLUE_PIN, duty_cycle_blue); // Atualiza o PWM do LED azul

    uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual

    // Atualiza o conteúdo do display com animações
    bool cor = true;                                            // Define a cor da animação
    ssd1306_fill(&ssd, !cor);                                   // Limpa o display
    ssd1306_draw_world_map(&ssd, epd_bitmap_mapa_mundi_pixell); // Desenha o mapa-múndi no display

    ssd1306_cross(&ssd, x, y, 15, !ssd1306_get_pixel(epd_bitmap_mapa_mundi_pixell, x, y)); // Desenha uma cruz no centro do display
    ssd1306_draw_string(&ssd, "Marque o Local", 10, 3);                                    // Escreve uma string no display
    ssd1306_send_data(&ssd);                                                               // Atualiza o display
    sleep_ms(1);
} // Aguarda 1 ms para criar o efeito de animação

// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual

    if (gpio == PB) // Se o botão do joystick foi pressionado
    {
        if (current_time - last_joystick_button_time >= 200) // Debounce (evita múltiplas leituras)
        {
            gpio_put(GREEN_PIN, !gpio_get(GREEN_PIN)); // Alterna o estado do LED verde
            last_joystick_button_time = current_time;  // Atualiza o tempo de debounce
        }
    }
    else if (gpio == button_a) // Se o botão A foi pressionado
    {
        if (current_time - last_button_a_time >= 200) // Debounce
        {
            last_button_a_time = current_time; // Atualiza o tempo de debounce
        }
    }
}

void i2c_adc_gpio_init()
{
    adc_init();             // Inicializa o ADC
    adc_gpio_init(VRX_PIN); // Inicializa o pino do eixo X do joystick
    adc_gpio_init(VRY_PIN); // Inicializa o pino do eixo Y do joystick

    uint pwm_wrap = 4096;                                    // Valor de wrap do PWM (resolução de 12 bits)
    uint pwm_slice_red = pwm_init_gpio(RED_PIN, pwm_wrap);   // Inicializa o PWM do LED vermelho
    uint pwm_slice_blue = pwm_init_gpio(BLUE_PIN, pwm_wrap); // Inicializa o PWM do LED azul

    buttons_init(); // Inicializa os botões (joystick, botão A e botão B)

    // Inicialização do I2C (usado para comunicação com o display)
    i2c_init(I2C_PORT, 400 * 1000);            // Define a velocidade do I2C para 400 KHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configura o pino SDA para função I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configura o pino SCL para função I2C
    gpio_pull_up(I2C_SDA);                     // Habilita pull-up no pino SDA
    gpio_pull_up(I2C_SCL);                     // Habilita pull-up no pino SCL

    // Inicializa o display SSD1306
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display com o endereço I2C
    ssd1306_config(&ssd);                                         // Configura o display (opções como inversão de cores)
    ssd1306_send_data(&ssd);                                      // Envia as configurações para o display
    // Limpa o display (apaga todos os pixels)
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}
