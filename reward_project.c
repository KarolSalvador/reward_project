#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
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
        sm = pio_claim_unused_sm(np_pio, true); // tenta alocar uma máquina até que uma esteja livre
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

int getIndex(int x, int y) { //cria variáveis x para horizontal e y para vertical
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.

    if (y % 2 == 0) { // verificação se a linha é par ou ímpar
        return 24-(y * 5 + x); // Linha par (calcula a posição do LED na linha, levando em consideração que   cada linha possui 5 LEDs.).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

//função para emitir som
void emit_sound(int duration_ms, int frequency_hz, int duty_cycle) { //parâmetros de duração, freq e duty
    //configuração do pino do Buzzer na função para que funcione como um pino PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    //obtenção do número do slice do PWM para poder controlar a modulação do sinal
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    //definição do divisor do relógio para o slice, oq ue irá afetar a frequência do som emitido
    pwm_set_clkdiv(slice_num, 125.0f);
    //definir o número máximo de ciclos para o PWM
    pwm_set_wrap(slice_num, 1250000 / frequency_hz);
    //definir o nível de sáida do PWM 
    pwm_set_gpio_level(BUZZER_PIN, (1250000 / frequency_hz) * duty_cycle / 100);
    //habilita o slice para o sinal gerado ser emitido no buzzer
    pwm_set_enabled(slice_num, true);
    //aguarda durante o período de duração do som
    sleep_ms(duration_ms);
    //desabilita o a saída PWM
    pwm_set_enabled(slice_num, false);
}


int main() {
    // Inicializa entradas e saídas.
    stdio_init_all();

    // Inicializa matriz de LEDs com LEDs apagados.
    npInit(LED_PIN);

    //Desenho dos LEDs
    //matriz 5x5 onde cada elemento é um LED que é especificado por três valores(representando as cores, RGB)
    int matriz[5][5][3] = {
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}},
        {{0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}
    };
    
    //loop de atrivuição dos LEDs
    for(int linha = 0; linha < 5; linha++) {
        for(int coluna = 0; coluna < 5; coluna++) {
            //calcula o índice do LED na sequência
            int posicao = getIndex(linha, coluna);

            //Define a cor do LED na posição calculada
            npSetLED(posicao, matriz[coluna][linha][0], matriz[coluna][linha][1], matriz[coluna][linha][2]);
        }
    }

     //inializa o i2c com taxa de 400kHz
    i2c_init(i2c0, 400000);

    //configura o SCL no pino 15
    gpio_set_function(15, GPIO_FUNC_I2C); //SCL
    //configura o SDA no pino 14
    gpio_set_function(14, GPIO_FUNC_I2C); //SDA
    gpio_pull_up(15); //PULL-UP EM SCL
    gpio_pull_up(14); //PULL-UP EM SDA

    //declaração do buffer e da renderização
    uint8_t buf[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
    struct render_area area = {0, 0, SSD1306_WIDTH, SSD1306_HEIGHT};

    //inicializa o display OLED
    SSD1306_init(i2c0);

    //configuração do botão
    gpio_init(BUTTON_PIN);
    //define como entrada
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    //configuração do Buzzer
    gpio_init(BUZZER_PIN);
    //define como saída
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    //MONITORAR TEMPO QUE O BOTÃO ESTÁ PRESSIONADO
    bool button_pressed = false; //determinad que o botão não está pressionado inicialmente
    uint32_t press_start_time = 0; // armazena o tempo que o botão está pressionado iniciado com o valor 0

    //configurar renderização
    struct render_area frame_area = {
        .start_col = 0, //primeira coluna do eixo X, que começa na primeira coluna
        .end_col = SSD1306_WIDTH - 1, //ultima coluna do eixo X, que vai até a última coluna
        .start_page = 0, //primeira página do eixo Y, que começa na primeira linha
        .end_page = (SSD1306_HEIGHT / 8) - 1 //última página do eixo Y, que vai até a última linha
    };
    calc_render_area_buflen(&frame_area); //calcula a memória necessária para armazenar a área de renderização


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