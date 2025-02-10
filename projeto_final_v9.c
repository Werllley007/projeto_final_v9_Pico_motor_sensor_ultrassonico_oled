#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include "hardware/pwm.h"
#include "lib/oled.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#define LED_PIN 12         // Pino do LED
#define ENA 8  // GP8 - PWM Motor Direito
#define ENB 9  // GP9 - PWM Motor Esquerdo
#define IN1 16 // GP16 - Direita Frente
#define IN2 17 // GP17 - Direita Ré
#define IN3 18 // GP18 - Esquerda Frente
#define IN4 19 // GP19 - Esquerda Ré
#define MAX_PWM 65535 // PWM máximo

//Sensor Ultrassonico
#define TRIG_PIN 4
#define ECHO_PIN 20

//Rede Wifi
#define WIFI_SSID "brisa-2835919"  
#define WIFI_PASS "mgghnxgz" 
//#define WIFI_SSID "robotic lab"  
//#define WIFI_PASS "140/24-1800-k-w" 

// Variáveis globais para PWM
uint slice_num_A, slice_num_B;

void setup_pwm(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, MAX_PWM);
    pwm_set_chan_level(slice, PWM_CHAN_A, 0);
    pwm_set_enabled(slice, true);
}

void motor_setup() {
    gpio_init(IN1);
    gpio_init(IN2);
    gpio_init(IN3);
    gpio_init(IN4);
    gpio_set_dir(IN1, GPIO_OUT);
    gpio_set_dir(IN2, GPIO_OUT);
    gpio_set_dir(IN3, GPIO_OUT);
    gpio_set_dir(IN4, GPIO_OUT);
    setup_pwm(ENA);
    setup_pwm(ENB);
}

void move_forward(int speed) {
    printf("move_forward( %d )\n",speed);
    gpio_put(IN1, 1);
    gpio_put(IN2, 0);
    gpio_put(IN3, 1);
    gpio_put(IN4, 0);
    pwm_set_gpio_level(ENA, speed);
    pwm_set_gpio_level(ENB, speed);
}

void move_backward(int speed) {
    printf("Movendo para move_backward()\n");
    gpio_put(IN1, 0);
    gpio_put(IN2, 1);
    gpio_put(IN3, 0);
    gpio_put(IN4, 1);
    pwm_set_gpio_level(ENA, speed);
    pwm_set_gpio_level(ENB, speed);
}

void turn_left(int speed) {
    printf("turn_left( %d )\n",speed);
    gpio_put(IN1, 0);
    gpio_put(IN2, 1);
    gpio_put(IN3, 1);
    gpio_put(IN4, 0);
    pwm_set_gpio_level(ENA, speed);
    pwm_set_gpio_level(ENB, speed);
}

void turn_right(int speed) {
    printf("turn_right( %d )\n",speed);
    gpio_put(IN1, 1);
    gpio_put(IN2, 0);
    gpio_put(IN3, 0);
    gpio_put(IN4, 1);
    pwm_set_gpio_level(ENA, speed);
    pwm_set_gpio_level(ENB, speed);
}

void stop_motors() {
    printf("stop_motors()\n");
    gpio_put(IN1, 0);
    gpio_put(IN2, 0);
    gpio_put(IN3, 0);
    gpio_put(IN4, 0);
    pwm_set_gpio_level(ENA, 0);
    pwm_set_gpio_level(ENB, 0);
}




char *get_network_ip() {
    static char ip_buffer[16];  // Buffer para armazenar o IP no formato string
    struct netif *netif = netif_list;

    // Certifique-se de pegar o primeiro interface de rede válida
    if (netif != NULL && netif_is_up(netif)) {
        snprintf(ip_buffer, sizeof(ip_buffer), "%s", ip4addr_ntoa(netif_ip4_addr(netif)));
    } else {
        snprintf(ip_buffer, sizeof(ip_buffer), "IP indisponivel");
    }

    return ip_buffer;
}




/*********************************************************/
void setup_ultrasonic_sensor() {
    gpio_init(TRIG_PIN);
    gpio_init(ECHO_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_put(TRIG_PIN, 0);
}

uint32_t get_pulse_time() {
    uint32_t start_time, end_time;

    // Envia o pulso de Trigger
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);

    // Espera o Echo ir para alto
    while (gpio_get(ECHO_PIN) == 0);

    // Registra o tempo quando o Echo vai para alto
    start_time = time_us_32();

    // Espera o Echo ir para baixo
    while (gpio_get(ECHO_PIN) == 1);

    // Registra o tempo quando o Echo volta para baixo
    end_time = time_us_32();

    // Retorna o tempo do pulso em microssegundos
    return end_time - start_time;
}

float calculate_distance() {
    uint32_t pulse_time = get_pulse_time();

    if (pulse_time == 0) {
        printf("Erro ao obter tempo do pulso\n");
        return -1.0;
    }

    // Converte o tempo em distância (som viaja a 0,034 cm/µs)
    return (pulse_time * 0.034) / 2.0;
}

void check_obstacle() {
    float distance = calculate_distance();

    if (distance > 0 && distance <= 30.0) {
        printf("Obstáculo detectado a %.2f cm. Parando motores.\n", distance);
        stop_motors();
    }
}

/*********************************************************/

static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;
    printf("Recebido: %s\n", request);
    int speed = MAX_PWM / 2;

    if (strstr(request, "GET /forward")) {
        move_forward(speed);
    } else if (strstr(request, "GET /backward")) {
        move_backward(speed);
    } else if (strstr(request, "GET /left")) {
        turn_left(speed);
    } else if (strstr(request, "GET /right")) {
        turn_right(speed);
    } else if (strstr(request, "GET /stop")) {
        stop_motors();
    }

    char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                  "<html><body>"       
                  "<h1 style='font-size: 36px; font-weight: bold; text-align: left;'>"
                  "Sistema de Controle do Robô"
                  "</h1>"          
                  "<p><a href='/forward'>Mover para forward</a></p>"
                  "<p><a href='/backward'>Mover para backward</a></p>"
                  "<p><a href='/left'>Virar para left</a></p>"
                  "<p><a href='/right'>Virar para right</a></p>"
                  "<p><a href='/stop_motors'>Parar</a></p>"
                  "</body></html>\r\n";
    tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar servidor TCP\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP iniciado na porta 80\n");
}

int main() {
    stdio_init_all();

    // Inicializa o OLED
    oled_init();
    oled_clear();

    // Exibe o texto "EmbarcaTech"
    oled_display_text("EmbarcaTech", 0, 0);

    sleep_ms(5000);  // Espera 5 segundos para visualização
    printf("Iniciando sistema...\n");

    motor_setup();
    setup_ultrasonic_sensor();

    if (cyw43_arch_init()) {
        printf("Erro ao inicializar Wi-Fi\n");
        oled_display_text("Erro Wi-Fi", 0, 16);
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        oled_display_text("Erro Conexao", 0, 16);
        return 1;
    }

    printf("Conectado ao Wi-Fi!\n");

    // Exibe o IP no display
    char *ip_address = get_network_ip();
    oled_display_text(ip_address, 0, 16);

    sleep_ms(1000);  // Aguarda um momento antes de prosseguir

    // Inicia o servidor HTTP
    start_http_server();

    // Variáveis para controle de tempo e execução
    uint32_t last_update_time = 0;
    uint32_t last_motor_check_time = 0;

    while (true) {
        // Atualiza o estado dos motores ou verifica obstáculos a cada 100 ms
        if (time_us_32() - last_motor_check_time > 100000) {
            check_obstacle();  // Verifica obstáculos e atualiza o controle dos motores
            last_motor_check_time = time_us_32();
        }

        // Atualiza o OLED a cada 1000 ms
        if (time_us_32() - last_update_time > 1000000) {
            char buffer[32];

            // Calcula e exibe a distância
            float distance = calculate_distance();
            if (distance > 0) {
                snprintf(buffer, sizeof(buffer), "Distancia: %.2f cm", distance);
            } else {
                snprintf(buffer, sizeof(buffer), "Distancia: ---");
            }
            oled_display_text(buffer, 0, 32);

            // Exibe o nome "Werlley" no final
            oled_display_text("Werlley", 0, 48);

            last_update_time = time_us_32();
        }

        // Mantém o polling do Wi-Fi
        cyw43_arch_poll();
    }

    cyw43_arch_deinit();
    return 0;
}
