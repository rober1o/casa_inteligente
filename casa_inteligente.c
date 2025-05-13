
#include "casa_inteligente.h"

int main()
{
    inicializar_perifericos();
    desenha_fig(matriz_apagada, BRILHO_PADRAO, pio, sm);

    while (true)
    {
        cyw43_arch_poll();

        monitorar(); // Verifica constantimente se deve disparar o alarme
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
}

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void inicializar_leds(void)
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
{ // Função para inicializar o pwm do Buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    pwm_set_wrap(slice_num, 25000);                                       // Frequência: 500 Hz (125 MHz / 25000 = 500 Hz)
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), 6250); // 50% duty cycle
    pwm_set_enabled(slice_num, false);                                    // Começa desligado
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

// Tratamento do request do usuário - digite aqui
void user_request(char **request)
{

    if (strstr(*request, "GET /luz_1") != NULL)
    {
        alternar_luz_1();
    }
    else if (strstr(*request, "GET /luz_2") != NULL)
    {
        alternar_luz_2();
    }
    else if (strstr(*request, "GET /porta") != NULL)
    {
        alternar_porta();
    }
    else if (strstr(*request, "GET /alarme") != NULL)
    {
        alternar_alarme();
    }
    else if (strstr(*request, "GET /modo_viagem") != NULL)
    {
        alternar_modo_viagem();
    }
    else if (strstr(*request, "GET /silenciar_alarme") != NULL)
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
             "<html><head>"
             "<style>"
             "body{background-color:#333;color:#fff;text-align:center}"
             "h1{margin:10px}"
             "button{width:150px;padding:10px;margin:5px;border:none;border-radius:6px;font-size:16px;color:#fff;cursor:pointer}"
             ".jumbo{padding:15px;background-color:#add8e6;border-radius:10px;margin:0 60%}"
             "p{background-color:#FF8C00; width: 300px; padding: 10px; border-radius: 10px;}"
             "</style>"
             "</head><body>"
             "<h1>Casa Inteligente</h1><div class='jumbo'>"

             // Luzes
             "<form action='/luz_1'><button style='background:#1E90FF'>&#x23FB; Luz 1</button></form>"
             "<form action='/luz_2'><button style='background:#FF4500'>&#x23FB; Luz 2</button></form>"

             // Porta
             "<form action='/porta'><button style='background:#666'>&#x23FB; Porta</button></form>"

             // Alarme
             "<form action='/alarme'><button style='background:#228B22'>&#x23FB; Alarme</button></form>"
             "<form action='/silenciar_alarme'><button style='background:#B22222'>Silenciar Alarme</button></form>"

             // Modo Viagem
             "<form action='/modo_viagem'><button style='background:#800080'>&#x23FB; Modo Viagem</button></form>"

             // Temperatura
             "<p>%.1f &deg;C</p>"

             "</div></body></html>",
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

void draw_ssd1306(uint32_t *_matriz)
{ // FUNÇÃO PARA DESENHAR NO DISPLAY COM O CÓDIGO EXPORTADO DO PISKEL
    for (int i = 0; i < 8192; i++)
    {
        int x = i % 128; // coluna
        int y = i / 128; // linha

        if (_matriz[i] > 0x00000000)
        {
            ssd1306_pixel(&ssd, x, y, true);
        }
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
    ssd1306_fill(&ssd, !cor);
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);

    // Desenhar imagem de porta conforme o estado
    if (porta_open)
    {
        draw_ssd1306(porta_aberta);
    }
    else
    {
        draw_ssd1306(porta_fechada);
    }

    // PRIORIDADE 1: Alarme disparado (sobrepõe tudo, inclusive modo viagem)
    if (alarme_disparado)
    {
        ssd1306_draw_string(&ssd, "ALERTA!", 56, 30);
    }
    // PRIORIDADE 2: Modo viagem (somente aparece se o alarme NÃO estiver disparado)
    else if (modo_viagem)
    {
        ssd1306_draw_string(&ssd, "MODO", 65, 25);
        ssd1306_draw_string(&ssd, "VIAGEM", 60, 35);
    }
    // PRIORIDADE 3: Estado normal do alarme ON/OFF
    else
    {
        ssd1306_draw_string(&ssd, "ALARME", 65, 30);
        ssd1306_draw_string(&ssd, alarme ? "ON" : "OFF", 75, 40);
    }

    ssd1306_send_data(&ssd);
}

// Leitura da temperatura interna
float verificar_temperatura(void)
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

void alternar_porta() // Função para abrir e fechar a porta
{
    porta_open = !porta_open;
    atualizar_display();
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
