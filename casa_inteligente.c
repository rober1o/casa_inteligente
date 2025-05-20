
#include "casa_inteligente.h"

int main()
{   // inicializa os perifericos, iniciando primeiro o wifi e posteriormente o restante
    inicializar_perifericos();
    desenha_fig(matriz_apagada, BRILHO_PADRAO, pio, sm);
    while (true)
    {
        cyw43_arch_poll();

        monitorar(); // Verifica constantimente se deve ou não disparar o alarme
        sleep_ms(200);
    }

    cyw43_arch_deinit();
    return 0;
}

// ********************************************* Funções *********************************************

// -------------------------------------- INICIALIZAR PERIFERICOS --------------------------------------

void inicializar_perifericos()
{
    // Inicializa a comunicação serial para depuração
    stdio_init_all();

    // Conecta ao Wi-Fi antes de inicializar os periféricos
    conectar_wifi(); // Agora, a conexão Wi-Fi é feita aqui

    // Inicializa os periféricos agora que o Wi-Fi foi estabelecido
    inicializar_display_i2c();        // Inicializa o display I2C
    configurar_matriz_leds();         // Configura a matriz de LEDs
    inicializar_leds();               // Configura o LED (se necessário)
    inicializar_pwm_buzzer();         // Inicializa o PWM para o buzzer
    atualizar_display();              // Atualiza o conteúdo do display
    configurar_botao_bootsel();       // Configura o botão BOOTSEL
    configurar_servidor_tcp();        // Configura o servidor TCP
    inicializar_sensor_temperatura(); // Inicializa o sensor de temperatura
    incializar_servo_motor();
}

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void inicializar_leds()
{
    // Configuração dos LEDs como saída
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, false);

    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);

    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);
}

void incializar_servo_motor()
{
    gpio_set_function(SERVO_MOTOR_PIN, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(SERVO_MOTOR_PIN); // Agora está correto!

    pwm_set_wrap(slice_num, 20000);    // 20 ms período (50Hz)
    pwm_set_clkdiv(slice_num, 125.0f); // Clock de 1MHz
    pwm_set_enabled(slice_num, true);
}

void configurar_matriz_leds() // FUNÇÃO PARA CONFIGURAR O PIO PARA USAR NA MATRIZ DE LEDS
{
    bool clock_setado = set_sys_clock_khz(133000, false);
    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &Matriz_5x5_program);
    Matriz_5x5_program_init(pio, sm, offset, MATRIZ_PIN);
}

void inicializar_display_i2c()
{ // FUNÇÃO PARA INICIALIZAR O I2C DO DISPLAY
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void inicializar_pwm_buzzer()
{
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    pwm_set_clkdiv(slice_num, 10.0f);                                      // Reduz clock base para 12.5 MHz
    pwm_set_wrap(slice_num, 31250);                                        // 12.5 MHz / 31250 = 400 Hz
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), 15625); // 50% duty cycle
    pwm_set_enabled(slice_num, false);                                     // Começa desligado
}

void inicializar_sensor_temperatura() // função para inicializar o ADC do sensor de temperatura interno
{
    adc_init();
    adc_set_temp_sensor_enabled(true);
}

void conectar_wifi() // Função para conectar ao WIFI
{
    if (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        exit(-1);
    }

    cyw43_arch_gpio_put(LED_PIN, 0);
    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        exit(-1);
    }

    printf("Conectado ao Wi-Fi\n");

    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }
}

void configurar_servidor_tcp() // Função para configurar servidor TCP
{
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        exit(-1);
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        exit(-1);
    }

    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");
}

void configurar_botao_bootsel() // Função para Bootsel no botão B
{
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

// -------------------------------------- PROCESSAMENTO TCP --------------------------------------

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Tratamento do request do usuário
void user_request(char **request)
{

    if (strstr(*request, "GET /l1") != NULL)
    {
        alternar_luz_1();
    }
    else if (strstr(*request, "GET /l2") != NULL)
    {
        alternar_luz_2();
    }
    else if (strstr(*request, "GET /pt") != NULL)
    {
        alternar_porta();
    }
    else if (strstr(*request, "GET /al") != NULL)
    {
        alternar_alarme();
    }
    else if (strstr(*request, "GET /mv") != NULL)
    {
        alternar_modo_viagem();
    }
    else if (strstr(*request, "GET /si") != NULL)
    {
        silenciar_alarme();
    }
};

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);

    // Leitura da temperatura interna
    float temperatura = verificar_temperatura();

    // Cria a resposta HTML
    char html[1024];
    snprintf(html, sizeof(html),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n\r\n"
             "<html><head><style>"
             "body{background:#333;color:#f0f0f0;text-align:center;font-family:sans-serif}"
             "a{display:block;width:200px;padding:10px;margin:10px auto;border-radius:8px;text-decoration:none;color:#fff;font-size:16px}"
             ".b{background:#9ac7e0;padding:15px;border-radius:12px;margin:20px auto;width:40vw}"
             ".l1{background:#3a78d0}.l2{background:#d23c1f}.pt{background:#555}"
             ".al{background:#1e6e1e}.si{background:#992222}.mv{background:#663399}"
             ".t{background:#1a1a1a;padding:12px;border-radius:12px;width:180px;margin:20px auto;border:1px solid #e88c00}"
             ".tv{color:#e88c00;font-size:18px;margin-top: 10px;}"
             "</style></head><body>"
             "<h3>Casa Inteligente</h3><div class='b'>"
             "<a href='/l1' class='l1'>Luz 1</a>"
             "<a href='/l2' class='l2'>Luz 2</a>"
             "<a href='/pt' class='pt'>Porta</a>"
             "<a href='/al' class='al'>Alarme</a>"
             "<a href='/si' class='si'>Silenciar</a>"
             "<a href='/mv' class='mv'>Modo viagem</a>"
             "</div>"
             "<div class='t'>Temperatura:<div class='tv'>%.1f&deg;C</div></div>"
             "</body></html>",
             temperatura);

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    // libera memória alocada dinamicamente
    free(request);

    // libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}

// ---*-----------------------------------------------*------------------------------------------*--------

// -------------------------------------- FUNÇÕES AUXILIARES DOS PERIFERICOS --------------------------------------

#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

void desenha_fig(uint32_t *_matriz, uint8_t _intensidade, PIO pio, uint sm) // FUNÇÃO PARA DESENHAR O SEMAFORO NA MATRIZ
{
    uint32_t pixel = 0;
    uint8_t r, g, b;

    for (int i = 24; i > 19; i--) // Linha 1
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF) * (_intensidade / 100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF) * (_intensidade / 100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF) * (_intensidade / 100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel << 8u);
    }

    for (int i = 15; i < 20; i++) // Linha 2
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF) * (_intensidade / 100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF) * (_intensidade / 100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF) * (_intensidade / 100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (b << 16) | (r << 8) | g;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel << 8u);
    }

    for (int i = 14; i > 9; i--) // Linha 3
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF) * (_intensidade / 100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF) * (_intensidade / 100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF) * (_intensidade / 100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel << 8u);
    }

    for (int i = 5; i < 10; i++) // Linha 4
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF) * (_intensidade / 100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF) * (_intensidade / 100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF) * (_intensidade / 100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel << 8u);
    }

    for (int i = 4; i > -1; i--) // Linha 5
    {
        pixel = _matriz[i];
        b = ((pixel >> 16) & 0xFF) * (_intensidade / 100.00); // Isola os 8 bits mais significativos (azul)
        g = ((pixel >> 8) & 0xFF) * (_intensidade / 100.00);  // Isola os 8 bits intermediários (verde)
        r = (pixel & 0xFF) * (_intensidade / 100.00);         // Isola os 8 bits menos significativos (vermelho)
        pixel = 0;
        pixel = (g << 16) | (r << 8) | b;
        pio_sm_put_blocking(pio, sm, pixel << 8u);
    }
}

void tocar_pwm_buzzer(uint duracao_ms) // Função para Tocar o buzzer
{
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, true);
    sleep_ms(duracao_ms);
    pwm_set_enabled(slice_num, false);
}

bool cor = true;

// -------------------------------------- FUNÇÕES AUXILIARES DO SISTEMA --------------------------------------

void atualizar_display() // Atualizar as informações do display
{
    // PRIORIDADE 1: Alarme disparado (sobrepõe tudo, inclusive modo viagem)
    if (alarme_disparado)
    {
        ssd1306_draw_string(&ssd, "ALERTA!!", 40, 30);
    }
    // PRIORIDADE 2: Modo viagem (somente aparece se o alarme NÃO estiver disparado)
    else if (modo_viagem)
    {
        ssd1306_fill(&ssd, !cor);                     // limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Moldura
        ssd1306_draw_string(&ssd, "MODO VIAGEM", 20, 30);
        ssd1306_send_data(&ssd); // Envia os dados para o display
    }
    // PRIORIDADE 3: Estado normal do alarme ON/OFF
    else
    {

        if (alarme)
        {
            ssd1306_fill(&ssd, !cor);                     // limpa o display
            ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Moldura
            ssd1306_draw_string(&ssd, "ALARME: ON", 20, 30);
            ssd1306_send_data(&ssd); // Envia os dados para o display
        }
        else
        {
            ssd1306_fill(&ssd, !cor);                     // limpa o display
            ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Moldura
            ssd1306_draw_string(&ssd, "ALARME: OFF", 20, 30);
            ssd1306_send_data(&ssd); // Envia os dados para o display
        }
    }
}

// Leitura da temperatura interna
float verificar_temperatura()
{
    adc_select_input(4);
    uint16_t valor_adc = adc_read();
    const float fator_convercao = 3.3f / (1 << 12);
    float temperatura = 27.0f - ((valor_adc * fator_convercao) - 0.706f) / 0.001721f;
    return temperatura;
}

// -------------------------------------- FUNÇÕES ACIONADA PELOS BOTÕES --------------------------------------

void alternar_luz_1() // Fução para alternar a luz do Led
{
    luz_1 = !luz_1;

    gpio_put(LED_BLUE_PIN, luz_1);
    gpio_put(LED_RED_PIN, luz_1);
    gpio_put(LED_GREEN_PIN, luz_1);
}

void alternar_luz_2() // Função para alternar a luz do display
{
    luz_2 = !luz_2;

    if (luz_2)
    {
        desenha_fig(luz_quarto, BRILHO_PADRAO, pio, sm);
    }
    else
    {
        desenha_fig(matriz_apagada, BRILHO_PADRAO, pio, sm);
    }
}

void alternar_porta()
{
    porta_open = !porta_open; // alterna estado

    uint slice_num = pwm_gpio_to_slice_num(SERVO_MOTOR_PIN);
    uint channel = pwm_gpio_to_channel(SERVO_MOTOR_PIN);

    if (porta_open)
    {
        // 90 graus (posição aberta)
        pwm_set_chan_level(slice_num, channel, SERVO_90_DEGREES);
    }
    else
    {
        // 0 graus (posição fechada)
        pwm_set_chan_level(slice_num, channel, SERVO_MIN);
    }
}

void alternar_alarme() // função para ativar e desativar o alarme
{
    if (!modo_viagem)
    {
        alarme = !alarme;
    }

    atualizar_display();
}

void silenciar_alarme() // função para silenciar o alarme
{
    alarme_disparado = false;
    modo_viagem = false;
    alarme = false;
    atualizar_display();
}

void alternar_modo_viagem() // função para ativar e desativar o modo viagem
{
    modo_viagem = !modo_viagem;

    if (modo_viagem)
    {
        luz_1 = true;
        luz_2 = true;
        porta_open = true;
        alarme = true;
        alternar_luz_1();
        alternar_luz_2();
        sleep_ms(100);
        alternar_porta();
    }
    else
    {
        alarme = false;
        atualizar_display();
    }
}

void monitorar() // função que monitora constantimente se deve tocar o buzzer/alarme
{
    // Verifica se deve disparar o alarme
    if ((alarme && porta_open) ||
        (modo_viagem && (luz_1 || luz_2 || porta_open)))
    {
        alarme_disparado = true;
    }

    // Se o alarme foi silenciado, interrompe a disparação do alarme
    if (alarme_disparado && !alarme && !modo_viagem)
    {
        alarme_disparado = false;
    }
    atualizar_display();
    // Se o alarme foi disparado, mantém o buzzer tocando
    if (alarme_disparado)
    {
        tocar_pwm_buzzer(800);
    }
}
