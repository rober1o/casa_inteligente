#ifndef CASA_INTELIGENTE
#define CASA_INTELIGENTE

// ------------------------------
// Bibliotecas padrão C
// ------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// ------------------------------
// SDK do Raspberry Pi Pico
// ------------------------------
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// ------------------------------
// Conectividade Wi-Fi (CYW43)
// ------------------------------
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

// ------------------------------
// Bibliotecas externas do projeto
// ------------------------------
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/matriz_5X5.h"
#include "pio_wave.pio.h"

// ------------------------------
// Definições e constantes
// ------------------------------

// Credenciais da rede Wi-Fi
#define WIFI_SSID "SUA_REDE"
#define WIFI_PASSWORD "SUA_SENHA"

// Pinos de LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN
#define LED_BLUE_PIN 12
#define LED_GREEN_PIN 11
#define LED_RED_PIN 13

// Buzzer
#define BUZZER_PIN 10
#define SERVO_MOTOR_PIN 20
#define SERVO_MIN 1250        // pulso para 0 graus (exemplo: 1ms)
#define SERVO_90_DEGREES 2500 // pulso para 90 graus (exemplo: 1.5ms)

// Botões
#define botaoB 6

// I2C - Display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Matriz de LEDs
#define MATRIZ_PIN 7
#define NUM_PIXELS 25
#define BRILHO_PADRAO 50

// ------------------------------
// Variáveis globais
// ------------------------------
PIO pio;       // Instância do PIO
int sm;        // Máquina de estado
ssd1306_t ssd; // Instância do display OLED
bool luz_1 = false;
bool luz_2 = false;
bool alarme = false;
bool modo_viagem = false;
bool porta_open = false;
bool alarme_disparado = false;
uint slice_num;

// ------------------------------
// Protótipos de funções
// ------------------------------


//Funções de inicialização de periferico
void inicializar_perifericos();
void inicializar_leds(void);
void incializar_servo_motor();
void configurar_matriz_leds();
void inicializar_display_i2c();
void inicializar_pwm_buzzer();
void inicializar_sensor_temperatura();
void conectar_wifi();
void configurar_servidor_tcp();
void configurar_botao_bootsel();



//Buzzer
void tocar_pwm_buzzer(uint duracao_ms);

// Wi-Fi / TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Sensor de temperatura interno
float verificar_temperatura();

// Interface do usuário
void user_request(char **request);

// Display OLED

void atualizar_display();

// Matriz de LEDs

void desenha_fig(uint32_t *_matriz, uint8_t _intensidade, PIO pio, uint sm);


// funções de requisição do sistema

void alternar_luz_1();
void alternar_luz_2();
void alternar_porta();
void alternar_alarme();
void monitorar();
void silenciar_alarme();
void alternar_modo_viagem();

// GPIO / Interrupções
void gpio_irq_handler(uint gpio, uint32_t events);





#endif // CASA_INTELIGENTE
