#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/binary_info.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"

//definir botão
#define BUTTON_PIN 5
//definir buzzer
#define BUZZER_PIN 10

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

// Definição de pixel GRB
struct pixel_t {
 uint8_t G, R, B; // Três valores de 8-bits, cada um, compõem um pixel.
};

typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t", por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

//Inicializa a máquina PIO para controle da matriz de LEDs.
void npInit(uint pin) {
    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
    }

     // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

//Atribui uma cor RGB a um LED.
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

//Limpa o buffer de pixels.
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

//Escreve os dados do buffer nos LEDs.
void npWrite() {
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

//função para emitir som
void emit_sound(int duration_ms, int frequency_hz, int duty_cycle) {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    pwm_set_clkdiv(slice_num, 125.0f);
    pwm_set_wrap(slice_num, 1250000 / frequency_hz);
    pwm_set_gpio_level(BUZZER_PIN, (1250000 / frequency_hz) * duty_cycle / 100);

    pwm_set_enabled(slice_num, true);

    sleep_ms(duration_ms);
    pwm_set_enabled(slice_num, false);
}


int main() {
    // Inicializa entradas e saídas.
    stdio_init_all();

    // Inicializa matriz de LEDs com LEDs apagados.
    npInit(LED_PIN);

    //Desenho dos LEDs
    int matriz[5][5][3] = {
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}},
        {{0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}
    };
    
    for(int linha = 0; linha < 5; linha++) {
        for(int coluna = 0; coluna < 5; coluna++) {
            int posicao = getIndex(linha, coluna);
            npSetLED(posicao, matriz[coluna][linha][0], matriz[coluna][linha][1], matriz[coluna][linha][2]);
        }
    }

    i2c_init(i2c0, 400000); //inializa o i2c com taxa de 400kHz
    gpio_set_function(15, GPIO_FUNC_I2C); //SCL
    gpio_set_function(14, GPIO_FUNC_I2C); //SDA
    gpio_pull_up(15); //PULL-UP EM SCL
    gpio_pull_up(14); //PULL-UP EM SDA

    uint8_t buf[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
    struct render_area area = {0, 0, SSD1306_WIDTH, SSD1306_HEIGHT};

    SSD1306_init(i2c0);

    //configuração dos pinos
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    //MONITORAR TEMPO QUE O BOTÃO ESTÁ PRESSIONADO
    bool button_pressed = false;
    uint32_t press_start_time = 0;

    //configurar renderização
    struct render_area frame_area = {
        .start_col = 0,
        .end_col = SSD1306_WIDTH - 1,
        .start_page = 0,
        .end_page = (SSD1306_HEIGHT / 8) - 1
    };
    calc_render_area_buflen(&frame_area);

    //zerar display
    /*
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);
    */

    while(true) {
        //verificar se botão foi pressionado
        if(!gpio_get(BUTTON_PIN)) {
            if(!button_pressed) {
                button_pressed = true;
                press_start_time = to_ms_since_boot(get_absolute_time());
                printf("Botão pressionado. Tempo inicial: %u ms\n", press_start_time); 
            }

            //verificar o tempo que o botão está sendo pressionado
            uint32_t press_duration = to_ms_since_boot(get_absolute_time()) - press_start_time;
            printf("Botão pressionado por: %u ms\n", press_duration); // Depuração

            //caso botão pressionado por 5 segundos, liberar recompensa
            if(press_duration >= 5000) {
                //limpar o display
                memset(buf, 0, SSD1306_BUF_LEN);  // Preenche o buffer com zeros (limpando a tela), garantir que não vai sobreescrever
                render(buf, &frame_area);            // Atualiza o display com o buffer limpo
                printf("Recompensa liberada!\n");
                emit_sound(2000, 1000, 50);

                npWrite(); // Escreve os dados nos LEDs.
                sleep_ms(2000);
                npClear();
                npWrite();//atualizar matriz com leds apagados.
                

                //exibir no display
                /*
                char *text[] = {
                    "~~Recompensa~~",
                    "~~liberada!~~",
                    "~Bom garoto!!~"
                };

                int y = 0;
                for(uint i = 0; i < count_of(text); i++) {
                    WriteString(buf, 5, y, text[i]); //posicionar texto no buffer
                    y += 8; //avançar linha
                }

                render(buf, &frame_area); // atualiza o display com o novo conteudo do buffer
                */
                WriteString(buf, 5, 0, "Recompensa liberada");
                render(buf, &frame_area);
                printf("Mensagem exibida no display.\n");

                //aguardar botão liberar
                while(!gpio_get(BUTTON_PIN)) {
                    tight_loop_contents(); // Economiza energia enquanto espera
                }
                button_pressed = false; //resetar estado do botão
                printf("Botão solto. Aguardando nova pressão.\n"); // Depuração
            }

        } else {
            button_pressed = false;
        }
        sleep_ms(50);
    }
    return 0;

}