// Bibliotecas usadas
#include <wiringPi.h> // Biblioteca para manipular a GPIO (Entrada/Saída de Propósito Geral)
#include <wiringSerial.h> //Permite a comunicação serial fornecida pela biblioteca WiringPi
#include <lcd.h> // Biblioteca que contém definições e declarações relacionadas ao controle do display LCD
#include <stdio.h>  // Biblioteca que fornece funções para entrada e saída de dados. 
#include <errno.h> //Fornece acesso à variável errno e declarações de macros que são usadas para verificar e manipular erros durante a execução
#include <string.h> // Biblioteca que permite a manipulação de Strings
#include <MQTTClient.h>
#include <pthread.h>

// Pinagem do display
#define LCD_RS 13             // Pino de seleção de registro
#define LCD_E 18              // Pino de habilitação
#define LCD_D4 21             // Pino de dados 4
#define LCD_D5 24             // Pino de dados 5
#define LCD_D6 26             // Pino de dados 6
#define LCD_D7 27             // Pino de dados 7

// Pinagem dos botões
#define BOTAO1 23             // PA07
#define BOTAO2 25             // PA10
#define BOTAO3 19             // PA20

// Configuração da porta serial UART
#define PORTA_SERIAL_UART "/dev/ttyS3"
#define BAUD_RATE 115200



//IP do broker do laboratório
#define MQTT_ADDRESS   "tcp://10.0.0.101"

#define CLIENTID       "2022136"

//tópico de envio de requisições da SBC para a NodeMCU: 
//ligar e desligar o led, obter situação da nodeMCU, obter a medição do sensor analogico
#define MQTT_PUBLISH_TOPIC_ESP_REQUEST    "SBCESP/REQUEST"

//tópico de request da medição de um sensor digital
#define MQTT_PUBLISH_TOPIC_ESP_REQUEST_SENSOR_DIGITAL    "SBCESP/REQUEST/DIGITAL"

//tópico para enviar o valor do novo intervalo de atualização das medições dos sensores
#define MQTT_PUBLISH_TOPIC_ESP_TIMER_INTERVAL "SBC/TIMERINTERVAL"

//tópico de resposta das requisições da SBC para a NodeMCU: 
//ligar e desligar o led, obter situação da nodeMCU
#define MQTT_SUBSCRIBE_TOPIC_ESP_RESPONSE  "ESPSBC/RESPONSE"

//Tópico para receber a medição do sensor analogico
#define MQTT_SUBSCRIBE_TOPIC_ESP_RESPONSE_SENSOR_ANALOGICO  "ESPSBC/RESPONSE/ANALOGICO"

//Tópico para receber a medição do sensor digital selecionado pelo usuario
#define MQTT_SUBSCRIBE_TOPIC_ESP_RESPONSE_SENSOR_DIGITAL  "ESPSBC/RESPONSE/DIGITAL"

//tópico para receber a medição dos sensores digitais dos 8 pinos da NodeMCU e atualizar no histórico
#define MQTT_SUBSCRIBE_TOPIC_ESP_RESPONSE_HISTORY_SENSOR_DIGITAL  "ESPSBC/RESPONSE/HISTORY/DIGITAL"

typedef struct{
  char values[16];
  char time[10];
} History;

MQTTClient client;
int lcd;



int lcd = 0;  // Variavel do LCD inicializada com 0
int uartfd = 0; // Variável responsavel pela transmissão serial de dados pela uart
int dadoRecebido = 0; // Dado Recebido
int dado = 254; // Variável de dados, que serve como uma chave que 
                //determina qual condição da NodeMCU será exibida
                // 254 -> Exibe:  D0
                // 255 -> Exibe:  D1
                // 155 -> Acende: Led
                // 100 -> Exibe:  Analógico

// Configuração dos pinos dos botões
const int botao1 = 23;
const int botao2 = 25;
const int botao3 = 19;

int valorAnalogico = 0; // Variável que guarda o valor analógico
char d[4]; // Variável d é um vetor de caracteres (char) com tamanho 4
           // que recebe os valores do dados analógico em bits separados 



/    //Abertura do arquivo da UART
  int mqtt_connect = -1;
  
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  conn_opts.keepAliveInterval = 1500;
  conn_opts.cleansession = 1;
  conn_opts.username = USERNAME;
  conn_opts.password = PASSWORD;
  
  printf("Iniciando MQTT\n");
  
  /* Inicializacao do MQTT (conexao & subscribe) */
    MQTTClient_create(&client, MQTT_ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(client, NULL, connlost, on_message, NULL);

  mqtt_connect = MQTTClient_connect(client, &conn_opts);
  
  if (mqtt_connect == -1){
    printf("\n\rFalha na conexao ao broker MQTT. Erro: %d\n", mqtt_connect);
          exit(-1);
  }
  //definindo em quais tópicos a SBC estará inscrita
  MQTTClient_subscribe(client, MQTT_SUBSCRIBE_TOPIC_ESP_RESPONSE, 2);
    MQTTClient_subscribe(client, MQTT_SUBSCRIBE_TOPIC_ESP_RESPONSE_SENSOR_DIGITAL, 2);
    MQTTClient_subscribe(client, MQTT_SUBSCRIBE_TOPIC_ESP_RESPONSE_SENSOR_ANALOGICO, 2);
    MQTTClient_subscribe(client, MQTT_SUBSCRIBE_TOPIC_ESP_RESPONSE_HISTORY_SENSOR_DIGITAL, 2);


// Opções do menu
enum OpcaoMenu { SelecionarNode, DadosAnalogicos, DadosDigitais, LigarLed, 
DadosAnalogicosMQTT, DadosDigitaisMQTT, Situacao, LigarLedMQTT, DesligarLedMQTT, HistoricoMQTT};

// Variáveis do menu
enum OpcaoMenu opcaoSelecionada = SelecionarNode;

// Funções do menu
void selecionarNode() {

  // Espaço para incrementar código para selecionar mais de uma Node //
  // Aqui iremos trabalhar apenas com uma Node, então exibiremos isso no display e console
  printf("LEU NODE\n");
  lcdPuts(lcd, "NODE1:");
}
// Função que exibe os dados analógicos
void dadosAnalogicos() {

  // Código para exibir dados analógicos
  dado = 100; // incrementa 100 na variavél 'dado' para poder entrar na condição para exibir o dado analogico
  lcdClear(lcd);
  lcdPuts(lcd, "> Sensor");
  serialPutchar(uartfd, dado);
  delay(2000);

  if (serialDataAvail(uartfd) > 0) {
    printf("CONSEGUIU LER ANALO\n"); // exibe no console que conseguiu ler o dado analogico

    // Ler caracteres (bytes) do valor analógico enviada pela UART e add em d[0] d[1] d[2] //
    d[0] = serialGetchar(uartfd); 
    d[1] = serialGetchar(uartfd);
    d[2] = serialGetchar(uartfd);

    // Foi usado deslocamento lógico à esquerda, uma operação bit a bit que desloca todos os bits de um valor para a esquerda
    valorAnalogico = (d[2] << 16) + (d[1] << 8) + d[0]; // Junta os bits de informações na variável valorAnalogico
    printf("%d\n", valorAnalogico); // Exibe no console o valor que deve ser exibido no display
    lcdPrintf(lcd, "> A0 %d", valorAnalogico); // Exibe no display o valor do Dado Analógico

    delay(900);
  }
}

void dadosDigitais() {
  // Código para exibir os dados digitais

  printf("LEU\n");
  delay(900);

  dado = 254; // incrementa 254 na variavél 'dado' para poder exibir o valor de 'D0'
  lcdClear(lcd); // limpa o display
  lcdPuts(lcd, "> Sensor"); // Exibe no display '> Sensor'
  serialPutchar(uartfd, dado); //Envia dado pra NodeMCU através da porta serial Uart
  delay(1000);

  if (serialDataAvail(uartfd) > 0) {
    printf("CONSEGUIU LER D0\n");
    dadoRecebido = serialGetchar(uartfd); // Recebe o dado do D1 através da UART
    lcdPrintf(lcd, "> D0 %d", dadoRecebido);  // Exibe no display o calor de D0
    delay(900);
  }

  lcdClear(lcd); // limpa o display
  lcdPuts(lcd, "> Sensor"); // Exibe no display '> Sensor'

  dado = 255; // incrementa 25 na variavél 'dado' para poder exibir o valor de 'D1'
  serialPutchar(uartfd, dado); //Envia dado pra NodeMCU através da porta serial Uart
  delay(1500);

  if (serialDataAvail(uartfd) > 0) {
    printf("CONSEGUIU LER D1\n"); // imprimir no console para verificação
    dadoRecebido = serialGetchar(uartfd); // Recebe o dado do D1 através da UART
    lcdPrintf(lcd, "> D1 %d", dadoRecebido); // Exibe no display o calor de D1
    delay(900);
  }
}

// Função que liga a LED
void ligarLed() {
  dado = 155; // incrementa 155 na variavél 'dado' para poder entrar na condição de ligar o led na NodeMCU
  serialPutchar(uartfd, dado); //Envia dado pra NodeMCU através da porta serial Uart
  delay(10);

// Condição que recebe o valor que liga a led
  if (serialDataAvail(uartfd) > 0) {
    printf("CONSEGUIU LER\n");
    dadoRecebido = serialGetchar(uartfd); 
    lcdPrintf(lcd, "> LED LIGADO %d", dadoRecebido); 
  }
}

// Funções do MQTT

void ligarLedMQTT(){

}

void desligarLedMQTT(){

}

void dadosDigitaisMQTT(){

}

void dadosAnalogicosMQTT(){
  enter(BOTAO3, sendRequestAnalogicSensor);


}

void historicoMQTT(){

}
// Funções dos botões //

// Função para quando o botão 1 estiver pressionado
void botao1Pressionado() {

//Percorre as opções do menu da primeira opção até a última
  opcaoSelecionada--;
  if (opcaoSelecionada < SelecionarNode) {
    opcaoSelecionada = HistoricoMQTT;
  }
}

// Função para quando o botão 2 estiver pressionado
void botao2Pressionado() {

  //Seleciona a opção exibida e chama a sua função
  switch (opcaoSelecionada) {
    case SelecionarNode:
      selecionarNode(); // Quando selecionado o menu "Selecionar Node" no display, chama a sua função
      break;

    case DadosAnalogicos:
      dadosAnalogicos(); // Quando selecionado o menu "Dados Analogicos" no display, chama a sua função
      break;

    case DadosDigitais:
      dadosDigitais(); // Quando selecionado o menu "Dados Digitais" no display, chama a sua função
      break;

    case LigarLed: // Quando selecionado o menu "Selecionar Led" no display, chama a sua função
      ligarLed();
      break;

    case DadosDigitaisMQTT:
      dadosDigitaisMQTT();
      break;

    case DadosAnalogicosMQTT:
      dadosAnalogicosMQTT();
        break;

    case LigarLedMQTT:
      ligarLedMQTT();
        break;

    case DesligarLedMQTT:
      desligarLedMQTT();
        break;

    case HistoricoMQTT:
      historicoMQTT();
      break;
  }
}

// Função para quando o botão 3 estiver pressionado
void botao3Pressionado() {

  // Percorre as opções do menu do final até inicio
    opcaoSelecionada++;
  if (opcaoSelecionada > HistoricoMQTT) { 
    opcaoSelecionada = SelecionarNode;
  }
}

// Função de exibição do menu do Display
void exibirMenu() {

  lcdClear(lcd); // Limpa o LCD
  lcdCursor(lcd, 0); // Move o cursor na posição inicial
  lcdPuts(lcd, "Menu:"); // Escreve no display a opção 'Menu'
  lcdCursor(lcd, 1); // Move o cursor para a segunda linha do display (linha 1). Assim, qualquer texto subsequente será exibido na segunda linha
  
  // Menu de ações a serem tomadas 
  switch (opcaoSelecionada) { // O Switch gerencia a opção Selecionada
    case SelecionarNode:
      lcdPuts(lcd, "> Selecionar Node");  // Exibe no display '> Selecionar Node'
      break;
    case DadosAnalogicos:
      lcdPuts(lcd, "> Dados Analogicos"); // Exibe no display '> Dados Analogicos'
      break;
    case DadosDigitais:
      lcdPuts(lcd, "> Dados Digitais"); // Exibe no display '> Dados Digitais'
      break;
    case LigarLed:
      lcdPuts(lcd, "> Ligar Led"); // Exibe no display '> Ligar Led'
      break;
    case DadosDigitaisMQTT:

      lcdPuts(lcd, "> "Dados DIG MQTT); // Exibe no display '> Ligar Led'
      break;
  }
}

// Função principal
int main() {
  // Inicialização do WiringPi
  wiringPiSetup();

  //Inicializa o display LCD de 2 colunas, 16 linhas e 4 bits de dados
  lcd = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0);

//Abrir a porta serial "/dev/ttyS3" com uma taxa de transmissão de 9600 bps. 
  if ((uartfd = serialOpen("/dev/ttyS3", 9600)) < 0) {

    //Ao ocorrer um erro a função pode definir o valor de 'errno' para indicar o tipo específico de erro que ocorreu.
    fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
    return 1;
  }

  //Configuração dos pinos dos botões como entrada e com pull-up interno
  pinMode(botao1, INPUT);
  pullUpDnControl(botao1, PUD_UP);
  pinMode(botao2, INPUT);
  pullUpDnControl(botao2, PUD_UP);
  pinMode(botao3, INPUT);
  pullUpDnControl(botao3, PUD_UP);

  // Loop principal
  while (1) {
    // Verifica se o botão 1 foi pressionado
    if (digitalRead(botao1) == LOW) {
      botao1Pressionado();
      exibirMenu();
      delay(500);
    }

    // Verifica se o botão 2 foi pressionado
    if (digitalRead(botao2) == LOW) {
      botao2Pressionado();
      exibirMenu();
      delay(500);
    }

    // Verifica se o botão 3 foi pressionado
    if (digitalRead(botao3) == LOW) {
      botao3Pressionado();
      exibirMenu();
      delay(500);
    }
  }

  return 0;
}
