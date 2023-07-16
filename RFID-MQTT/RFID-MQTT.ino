#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include "ESP8266WiFi.h"
#include "RDM6300.h"

//Parametros de conexão
const char* ssid = "Arthur"; // REDE
const char* password = "12345678-ar"; // SENHA

// MQTT Broker
const char *mqtt_broker = "192.168.1.132";       //Host do broket
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
SoftwareSerial RFID(D6, D8);

RDM6300 RDM6300(Payload, D6, D8);

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
  
  reconectWiFi();

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
      String tag_string = "{\"tag\": \""+RDM6300.result()+"\"}";

      // Passa para const char*
      const char* tag = tag_string.c_str();

      //Publica e imprime
      client.publish(topic, tag);
      Serial.println(tag);
    }
  }
  //garante funcionamento das conexões WiFi e ao broker MQTT
  VerificaConexoesWiFIEMQTT();

  //keep-alive da comunicação com broker MQTT
  client.loop();
}}

bool connectMQTT() {
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

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

//Função: função de callback 
//        esta função é chamada toda vez que uma informação de 
//        um dos tópicos subescritos chega)
//Parâmetros: nenhum
//Retorno: nenhum
void callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
   
    //toma ação dependendo da string recebida:
    if (msg.equals("OI"))
      Serial.println("OI");
     
}

//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT() 
{
    while (!client.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(mqtt_broker);
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            client.subscribe(topic); 
        } else{
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
  
//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum
void reconectWiFi() 
{
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(ssid, password); // Conecta na rede WI-FI
     
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
   
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(ssid);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}

//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
//        Em caso de desconexão (qualquer uma das duas), a conexão
//        é refeita.
//Parâmetros: nenhum
//Retorno: nenhum
void VerificaConexoesWiFIEMQTT(void)
{
    if (!client.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
     
     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}