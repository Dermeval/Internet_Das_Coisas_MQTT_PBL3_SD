// Bibliotecas necessárias
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <lcd.h>

// Constantes
#define LCD_RS 3
#define LCD_E  0
#define LCD_D4 6
#define LCD_D5 1
#define LCD_D6 5
#define LCD_D7 4
#define LCD_COLS 16
#define LCD_ROWS 2

// Variáveis globais
static struct mosquitto *mqtt_client;
static int lcd_handle;

// Função de callback para receber mensagens MQTT
void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
    if (strcmp(message->topic, "sensor/temperatura") == 0) {
        float temperatura;
        memcpy(&temperatura, message->payload, sizeof(float));
        
        // Atualizar a IHM remota
        // Código para enviar a temperatura para a IHM remota
    } else if (strcmp(message->topic, "sensor/umidade") == 0) {
        float umidade;
        memcpy(&umidade, message->payload, sizeof(float));
        
        // Atualizar a IHM remota
        // Código para enviar a umidade para a IHM remota
    }
}

// Função para atualizar o display LCD local
void update_lcd(float temperatura, float umidade) {
    char lcd_text[32];
    snprintf(lcd_text, sizeof(lcd_text), "Temperatura: %.2f C", temperatura);
    lcdPosition(lcd_handle, 0, 0);
    lcdPuts(lcd_handle, lcd_text);
    
    snprintf(lcd_text, sizeof(lcd_text), "Umidade: %.2f %%", umidade);
    lcdPosition(lcd_handle, 0, 1);
    lcdPuts(lcd_handle, lcd_text);
}

// Função para inicializar o display LCD local
void init_lcd() {
    lcd_handle = lcdInit(LCD_ROWS, LCD_COLS, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0);
    lcdClear(lcd_handle);
}

// Função principal
int main(int argc, char *argv[]) {
    // Inicialização do sistema
    wiringPiSetup();
    init_lcd();
    
    // Inicialização do MQTT
    mosquitto_lib_init();
    mqtt_client = mosquitto_new(NULL, true, NULL);
    
    // Configuração do MQTT
    mosquitto_connect(mqtt_client, "localhost", 1883, 0);
    mosquitto_subscribe(mqtt_client, NULL, "sensor/temperatura", 0);
    mosquitto_subscribe(mqtt_client, NULL, "sensor/umidade", 0);
    mosquitto_message_callback_set(mqtt_client, mqtt_message_callback);
    
    // Loop principal
    while (1) {
        // Leitura dos sensores locais
        float temperatura_local = 0.0;  // Código para ler a temperatura local
        float umidade_local = 0.0;     // Código para ler a umidade local
        
        // Atualização do display LCD local
        update_lcd(temperatura_local, umidade_local);
        
        // Envio dos dados para a IHM remota
        // Código para publicar a temperatura e umidade no MQTT
        
        // Espera de 1 segundo antes da próxima leitura
        delay(1000);
    }
    
    // Encerramento do programa
    mosquitto_destroy(mqtt_client);
    mosquitto_lib_cleanup();
    lcdClear(lcd_handle);
    return 0;
}
