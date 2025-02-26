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

void i2c_adc_gpio_init(); // Função para inicializar ADC, I2C e GPIO

void gpio_irq_handler(uint gpio, uint32_t events); // Função de interrupção para os botões

uint32_t last_print_time = 0;    // Controle de tempo para impressão (1 segundo)
uint32_t last_button_b_time = 0; // Controle de debounce para o botão B
uint32_t last_button_a_time = 0; // Controle de debounce para o botão A
uint32_t current_time = 0;
uint32_t last_frame = 0;
int fuso_atual = 12, fuso_externo = 12;
ssd1306_t ssd; // Estrutura do display SSD1306
uint tela = 0; // Variável para controle de tela
void logo();
void tela_0();
void tela_1();
void tela_2();
void tela_3();
void tela_4();

// Função de callback chamada pelo alarme
int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    uint32_t start_ms = to_ms_since_boot(get_absolute_time());
    minutos_num2++;
    if (minutos_num2 >= 60)
    {
        hora_num2++;
        minutos_num2 -= 60;
        if (hora_num2 >= 24)
            hora_num2 -= 24;
    }
    printf("Alarme disparado em: %d ms\n", start_ms);

    // Retorna um valor em microssegundos para reativar o alarme no futuro (60s)
    return 60 * 1000 * 1000;
}

int main()
{
    stdio_init_all();    // Inicializa a comunicação serial (para debug)
    i2c_adc_gpio_init(); // Inicializa ADC, I2C e GPIO
    // Habilita interrupções para os botões com função de callback
    gpio_set_irq_enabled_with_callback(PB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_b, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    // Configura o alarme para disparar após 60000 ms (1 minuto)
    alarm_id_t alarm = add_alarm_in_ms(60000, alarm_callback, NULL, false); // Alarme para atualizar os minutos

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
        case 2:
            tela_2();
            break;
        case 3:
            tela_3();
            break;
        case 4:
            tela_4();
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
    sleep_ms(2000);                                   // Aguarda 2s
}

void tela_0() // Recebe a hora atual
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
        }
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

void tela_1() // Recebe o fuso horário atual
{
    bool cor = true; // Define a cor da animação
    ssd1306_fill(&ssd, !cor);
    ssd1306_draw_string(&ssd, "Marque o seu", 4, 2); // Escreve uma string no display
    ssd1306_draw_string(&ssd, "fuso atual", 4, 14);  // Escreve uma string no display
    ssd1306_draw_string(&ssd, "E aperte A", 4, 26);  // Escreve uma string no display
    ssd1306_send_data(&ssd);
    sleep_ms(2000);

    while (tela == 1)
    {
        adc_select_input(0);             // Seleciona o canal do eixo Y
        uint16_t vry_value = adc_read(); // Lê o valor do eixo Y

        adc_select_input(1);             // Seleciona o canal do eixo X
        uint16_t vrx_value = adc_read(); // Lê o valor do eixo X

        // Converte os valores de ADC para duty cycle do PWM (intensidade dos LEDs)
        float duty_cycle_red = get_duty_cycle_red(vry_value);   // Cálculo para o LED vermelho (maior brilho no centro)
        float duty_cycle_blue = get_duty_cycle_blue(vry_value); // Cálculo para o LED azul (maior brilho nas extremidades)

        pwm_set_gpio_level(RED_PIN, duty_cycle_red);
        pwm_set_gpio_level(BLUE_PIN, duty_cycle_blue);

        // Mapeamento dos valores do joystick para as coordenadas do display
        uint8_t x = map_x_to_display(vrx_value); // Converte o valor do eixo X para coordenada X do display
        uint8_t y = map_y_to_display(vry_value); // Converte o valor do eixo Y para coordenada Y do display

        current_time = to_us_since_boot(get_absolute_time());

        if (current_time - last_frame >= 10) // 10us
        {                                    // Atualiza o conteúdo do display com animações
            last_frame = current_time;
            ssd1306_fill(&ssd, !cor);                                                              // Limpa o display
            ssd1306_draw_world_map(&ssd, epd_bitmap_mapa_mundi_pixell);                            // Desenha o mapa-múndi no display
            ssd1306_cross(&ssd, x, y, 15, !ssd1306_get_pixel(epd_bitmap_mapa_mundi_pixell, x, y)); // Desenha uma cruz no centro do display
            ssd1306_draw_string(&ssd, "Marque o Local", 10, 3);                                    // Escreve uma string no display
            fuso_atual = select_fuso(&ssd, x, y);
            ssd1306_send_data(&ssd); // Atualiza o display
        }
    }
    pwm_set_gpio_level(RED_PIN, 0);
    pwm_set_gpio_level(BLUE_PIN, 0);
    printf("botao A pressionado");
}

void tela_2() // Mostra os dados do fuso horário atual
{
    char buffer[10];
    char hora_buffer[10];
    sprintf(hora_buffer, "%s:%s", hora_char2, minutos_char2);

    bool cor = true;
    if (fuso_atual - 12 >= 0)                        // Se o fuso for maior que 12
        sprintf(buffer, "UTC: %d", fuso_atual - 12); // Converte "fuso_atual" para string
    else                                             // Se for menor que 12
        sprintf(buffer, "UTC: -%d", 12 - fuso_atual);
    while (tela == 2)
    {
        current_time = to_us_since_boot(get_absolute_time());
        if (current_time - last_frame >= 100000) // 100ms
        {
            last_frame = current_time;

            ssd1306_fill(&ssd, !cor);
            ssd1306_draw_string(&ssd, buffer, 5, 0);
            ssd1306_draw_string(&ssd, fusos_referencias[fuso_atual], 5, 10);
            ssd1306_draw_string(&ssd, "HORA: ", 5, 20);
            ssd1306_draw_string(&ssd, hora_buffer, 48, 20);
            ssd1306_hline(&ssd, 0, 120, 30, true);
            ssd1306_draw_string(&ssd, "pressione A ", 5, 32);
            ssd1306_draw_string(&ssd, "para checar", 5, 42);
            ssd1306_draw_string(&ssd, "outro fuso", 5, 52);

            ssd1306_send_data(&ssd); // Atualiza o display
        }
    }
    printf("botao A pressionado");
}

void tela_3() // Seleciona outro fuso horário
{
    bool cor = true; // Define a cor da animação
    ssd1306_fill(&ssd, !cor);
    ssd1306_draw_string(&ssd, "Marque o", 4, 2);       // Escreve uma string no display
    ssd1306_draw_string(&ssd, "fuso desejado", 4, 14); // Escreve uma string no display
    ssd1306_draw_string(&ssd, "E aperte A", 4, 26);    // Escreve uma string no display
    ssd1306_send_data(&ssd);
    sleep_ms(2000);

    while (tela == 3)
    {
        adc_select_input(0);             // Seleciona o canal do eixo Y
        uint16_t vry_value = adc_read(); // Lê o valor do eixo Y

        adc_select_input(1);             // Seleciona o canal do eixo X
        uint16_t vrx_value = adc_read(); // Lê o valor do eixo X
        // Converte os valores de ADC para duty cycle do PWM (intensidade dos LEDs)
        float duty_cycle_red = get_duty_cycle_red(vry_value);   // Cálculo para o LED vermelho (maior brilho no centro)
        float duty_cycle_blue = get_duty_cycle_blue(vry_value); // Cálculo para o LED azul (maior brilho nas extremidades)

        pwm_set_gpio_level(RED_PIN, duty_cycle_red);
        pwm_set_gpio_level(BLUE_PIN, duty_cycle_blue);

        // Mapeamento dos valores do joystick para as coordenadas do display
        uint8_t x = map_x_to_display(vrx_value); // Converte o valor do eixo X para coordenada X do display
        uint8_t y = map_y_to_display(vry_value); // Converte o valor do eixo Y para coordenada Y do display

        current_time = to_us_since_boot(get_absolute_time());

        if (current_time - last_frame >= 10) // 10us
        {                                    // Atualiza o conteúdo do display com animações
            last_frame = current_time;
            ssd1306_fill(&ssd, !cor);                                                              // Limpa o display
            ssd1306_draw_world_map(&ssd, epd_bitmap_mapa_mundi_pixell);                            // Desenha o mapa-múndi no display
            ssd1306_cross(&ssd, x, y, 15, !ssd1306_get_pixel(epd_bitmap_mapa_mundi_pixell, x, y)); // Desenha uma cruz no centro do display
            ssd1306_draw_string(&ssd, "Marque o Local", 10, 3);                                    // Escreve uma string no display
            fuso_externo = select_fuso(&ssd, x, y);
            ssd1306_send_data(&ssd); // Atualiza o display
        }
    }
    printf("botao A pressionado");
    pwm_set_gpio_level(RED_PIN, 0);
    pwm_set_gpio_level(BLUE_PIN, 0);
}

void tela_4() // Mostra o novo fuso horário
{
    char buffer[10];
    char hora_buffer[10];
    char hora_char3[3] = {};
    int hora_num3; // Hora do fuso externo
    hora_num3 = hora_num2 + (fuso_externo - fuso_atual);
    if (hora_num3 >= 24)
    {
        hora_num3 -= 24;
    }
    else if (hora_num3 < 0)
    {
        hora_num3 += 24;
    }
    sprintf(hora_char3, "%02d", hora_num3);

    sprintf(hora_buffer, "%s:%s", hora_char3, minutos_char2); // Os minutos permanecem os mesmos

    bool cor = true;
    if (fuso_externo - 12 >= 0)                        // Se o fuso for maior que 12
        sprintf(buffer, "UTC: %d", fuso_externo - 12); // Converte "fuso_externo" para string
    else                                               // Se for menor que 12
        sprintf(buffer, "UTC: -%d", 12 - fuso_externo);
    while (tela == 4)
    {
        current_time = to_us_since_boot(get_absolute_time());
        if (current_time - last_frame >= 100000) // 100ms
        {
            last_frame = current_time;

            ssd1306_fill(&ssd, !cor);
            ssd1306_draw_string(&ssd, buffer, 5, 0);
            ssd1306_draw_string(&ssd, fusos_referencias[fuso_externo], 5, 10);
            ssd1306_draw_string(&ssd, "HORA: ", 5, 20);
            ssd1306_draw_string(&ssd, hora_buffer, 48, 20);
            ssd1306_hline(&ssd, 0, 120, 30, true);
            ssd1306_draw_string(&ssd, "pressione: ", 5, 32);
            ssd1306_draw_string(&ssd, "A -Outro fuso", 5, 52);
            ssd1306_draw_string(&ssd, "B -Fuso inicial", 5, 42);

            ssd1306_send_data(&ssd); // Atualiza o display
        }
    }
    printf("botao A pressionado\n");
}

// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual

    if (gpio == button_b) // Se o botão B foi pressionado
    {
        if (current_time - last_button_b_time >= 500) // Debounce (evita múltiplas leituras)
        {
            last_button_b_time = current_time; // Atualiza o tempo de debounce
            if (tela == 4)
                tela = 2;
        }
    }
    else if (gpio == button_a) // Se o botão A foi pressionado
    {
        if (current_time - last_button_a_time >= 500) // Debounce
        {
            last_button_a_time = current_time; // Atualiza o tempo de debounce
            tela++;
            if (tela > 4)
                tela = 3;
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
