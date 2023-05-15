#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include "ESP8266WiFi.h"
#include "RDM6300.h"

//Parametros de conexão
const char* ssid = "HomeRouter 2G"; // REDE
const char* password = "1234ab1188@@##"; // SENHA

// MQTT Broker
const char *mqtt_broker = "192.168.15.202";       //Host do broket
const char *topic = "RFID";                       //Topico a ser subscrito e publicado
const char *mqtt_username = "";                   //Usuario
const char *mqtt_password = "";                   //Senha
const int mqtt_port = 1883;                       //Porta

//Configura o led na porta digital D2
int Led = D4;

//Variáveis
bool mqttStatus = 0;
String client_id = "ESP";
uint8_t Payload[6]; // used for read comparisons

//Objetos
WiFiClient espClient;
PubSubClient client(espClient);

//Inicializa a serial nos pinos 12 (RX) e 13 (TX) 
SoftwareSerial RFID(D6, D7);

RDM6300 RDM6300(Payload, D6, D7);

//Prototipos
bool connectMQTT();

void setup(void)
{
  Serial.begin(9600);

  // RFID 
  //Define o pino do led como saida
  pinMode(Led, OUTPUT);
  
  //Inicializa a serial para o leitor RDM6300
  RFID.begin(9600);
  
  // Conectar
  WiFi.begin(ssid, password);

  //Aguardando conexão
  Serial.println();
  Serial.print("Conectando");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  //Envia IP através da UART
  Serial.println(WiFi.localIP());

  mqttStatus =  connectMQTT();

  //Informacoes iniciais
  Serial.println("Leitor RFID RDM6300");
}

void loop() {if (mqttStatus){
  //Apaga o led
  delay(100);
  digitalWrite(Led, HIGH);
  
  //Aguarda a aproximacao da tag RFID
  while (RFID.available() > 0) {
    //Aciona o led
    digitalWrite(Led, LOW);
    
    uint8_t c = RFID.read();
    if (RDM6300.decode(c)){
      // Cria mensagem
      String tag_string = "{\"TAG\": \""+RDM6300.result()+"\"}";

      // Passa para const char*
      const char* tag = tag_string.c_str();

      //Publica e imprime
      client.publish(topic, tag);
      Serial.println(tag);
    }
  }
  client.loop();
}}

bool connectMQTT() {
  client.setServer(mqtt_broker, mqtt_port);

  int tentativa = 0;
  do {
    client_id += String(WiFi.macAddress());

    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Exito na conexão:");
      Serial.printf("Cliente %s conectado ao broker\n", client_id.c_str());
      return 1;
    } else {
      Serial.print("Falha ao conectar: ");
      Serial.print(client.state());
      Serial.println();
      Serial.print("Tentativa: ");
      Serial.println(tentativa);
      delay(2000);
    }
    tentativa++;
  } while (!client.connected() && tentativa < 5);

  if(tentativa >= 5)
    Serial.println("Não conectado");    
  return 0;
}
