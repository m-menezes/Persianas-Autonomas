#include <FS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <Servo.h>

#ifdef ESP32
#include <SPIFFS.h>
#endif

#ifdef __AVR__
#include <avr/power.h>
#endif
/*
  INSTANCIAS
*/
// Instancia Motor - Micro Servo
Servo servo;
// Instancia WiFiClient
WiFiClient espClient;
// Instancia PubSubClient - MQTT Client
PubSubClient client(espClient);
// Instancia Anel led RGB
Adafruit_NeoPixel pixels(16, 05, NEO_GRB + NEO_KHZ800);

/*
  ID DISPOSITIVO
*/
#define id_mqtt  "ESP8266_1"

/*
   CONFIGURAÇÃO BROKER
*/
char mqtt_server[40] = "192.168.100.2";
int  mqtt_port = 1883;

/*
   CONFIGURAÇÃO IP
*/
char static_ip[16] = "192.168.100.200";
char static_gw[16] = "192.168.100.1";
char static_sn[16] = "255.255.255.0";

/*
  PUBLISHER/SUBSCRIBER MQTT
*/
#define PUB_ANGULO          "ANGULO"
#define PUB_LUZ_PORCENT     "LUZ_PORCENT"
#define PUB_LDR             "LDR"
#define PUB_LUMENS          "LUMENS"
#define SUB_CONFIG          "CONFIG"

/*
  VARIAVEIS
*/
int LDRInterno = 0;
int COLOR = 0;
int ANGULO = 0;
int LUMENS = 0;
int CONFIG = 30;
char buffer[5];

/*
   FLAGS
*/
bool SaveConfig = false;
bool SendCONFIG = true;



/***************************************************************
   SALVA VALOR RECEBIDO DO MQTT NO ARQUIVO
***************************************************************/
void saveJson() {
  Serial.println("Salvando definição do usuário");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  //Copiando dado para um JSON
  json["CONFIG"] = CONFIG;
  //Abre arquivo para escrita
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Falha ao salvar JSON");
  } else {
    // json.prettyPrintTo(Serial);
    json.printTo(configFile);
  }
  //Fecha arquivo
  configFile.close();
}

/***************************************************************
  CONECTA MQTT
***************************************************************/
void initMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT: ");
    Serial.println(mqtt_server);
    if (client.connect(id_mqtt)) {
      Serial.println("Conectado com sucesso!");
      client.subscribe(SUB_CONFIG);
    }
    else {
      Serial.println("Erro ao conectar!!!");
      Serial.println("Nova tentativa de em 10s");
      delay(10000);
    }
  }
}

/***************************************************************
    CALLBACK PARA RECEBER VALOR DO CONFIG DO MQTT
***************************************************************/
void mqtt_callback(char* topic, byte* payload, unsigned int length) {

  String msg;
  /* MONTA STRING RECEBIDA NO PAYLOAD */
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }
  Serial.print("Chegou o seguinte valor via MQTT: ");
  Serial.println(msg);
  CONFIG = msg.toInt();
  saveJson();
}

/***************************************************************
  LEITURA ARQUIVO
***************************************************************/
void setupSpiffs() {
  // SPIFFS.format();
  Serial.println("Montando leitura do JSON...");
  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
        //SPIFFS.remove("/config.json");
        Serial.println("Lendo arquivo");
        File configFile = SPIFFS.open("/config.json", "r");
        if (configFile) {
            //Alocando buffer
            size_t size = configFile.size();
            std::unique_ptr<char[]> buf(new char[size]);
            configFile.readBytes(buf.get(), size);
            DynamicJsonBuffer jsonBuffer;
            JsonObject& json = jsonBuffer.parseObject(buf.get());
            json.printTo(Serial);
            if (json.success()) {
                /*
                LIGHT
                */
                if (json["CONFIG"]) {
                    Serial.println("Lendo Config do arquivo");
                    CONFIG = json["CONFIG"];
                    Serial.println(CONFIG);
                } else {
                    Serial.println("Sem Config no arquivo");
                }
            } else {
                Serial.println("Falha ao carregar JSON");
            }
        } else {
            Serial.println("Arquivo Config n]ao existe arquivo");
        }
    }
    //Arquivo Network
    if (SPIFFS.exists("/network.json")) {
      Serial.println("Lendo arquivo");
      File networkFile = SPIFFS.open("/network.json", "r");
      if (networkFile) {
        //Alocando buffer
        size_t size = networkFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        networkFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          /*
            MQTT
          */
          if (json["mqtt_server"]) {
            Serial.println("Lendo MQTT do arquivo");
            strcpy(mqtt_server, json["mqtt_server"]);
            mqtt_port = json["mqtt_port"];
          } else {
            Serial.println("Sem MQTT no arquivo");
          }
          /*
            IP
          */
          if (json["ip"]) {
            Serial.println("Lendo IP do arquivo");
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            Serial.println(static_ip);
          } else {
            Serial.println("Sem IP no arquivo");
          }
          
        } else {
          Serial.println("Falha ao carregar JSON");
        }
      }
    }
  } else {
    Serial.println("Falha na leitura do JSON");
  }
}


/***************************************************************
  ALTERA VALORES DOS LEDS
***************************************************************/
void light(String str) {
  String cmp = "Aumenta";
  if (cmp.equals(str)) {
    if (LUMENS < 254) {
      for (int i = 0; i < 16; i++) {
        pixels.setPixelColor(i, LUMENS, LUMENS, LUMENS);
        pixels.show();
      }
    }
    return;
  } else {
    if (LUMENS > 0) {
      for (int i = 0; i < 16; i++) {
        pixels.setPixelColor(i, LUMENS, LUMENS, LUMENS);
        pixels.show();
      }
    }
    if (LUMENS == 0) {
      for (int i = 0; i < 16; i++) {
        pixels.setPixelColor(i, 0, 0, 0);
        pixels.show();
      }
    }
    return;
  }
  return;
}

/***************************************************************
  FUNÇÃO CHECK, VERIFICA SE O VALOR DO LDR ESTÁ COMO DEVERIA
***************************************************************/
boolean check(String str) {
  String cmp = "Igual";
  int ATUAL = int((valorLDR() * 100) / 1024);
  if (cmp.equals(str)) {
    if (CONFIG == ATUAL || CONFIG == (ATUAL + 1) || CONFIG == (ATUAL - 1)) {
      return true;
    } else {
      return false;
    }
  } else {
    if (ATUAL < CONFIG) {
      return true;
    } else {
      return false;
    }
  }
}

/***************************************************************
  RETORNA O VALOR ANALOGICO INTERCALANDO O SINAL DAS PORTAS
***************************************************************/
int valorLDR() {
  return analogRead(A0);
}

/***************************************************************
  SETUP
***************************************************************/
void setup() {
  Serial.begin(115200);

  //Reset servo
  servo.attach(D7);
  servo.write(0);
  //Reset Leds
    #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
    #endif
  pixels.begin();
  for (int i = 0; i < 16; i++) {
    pixels.setPixelColor(i, 0, 0, 0);
    pixels.show();
  }
  setupSpiffs();

  WiFiManager wm;

  // Set IP
  IPAddress _ip, _gw, _sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  wm.setSTAStaticIPConfig(_ip, _gw, _sn);

  //Reset configs
  //wm.resetSettings();

  /*
      Auto conecta, se falhar inicia uma rede wifi para que possa ser configurado pelo smartphone
      Rede: AutoConnectAP
      Password: password
  */
  if (!wm.autoConnect("AutoConnectAP", "password")) {
    Serial.println("Falha ao conectar a Wi-Fi");
    delay(4000);
    ESP.restart();
    delay(4000);
  }

  Serial.println("Conectado");

  //Salva as configurações no arquivo
  if (SaveConfig) {
    Serial.println("Salvando configuração");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& network = jsonBuffer.createObject();
    JsonObject& json = jsonBuffer.createObject();
    //Salva no arquivo os dados do MQTT
    network["mqtt_server"] = mqtt_server;
    network["mqtt_port"]   = mqtt_port;

    //Salva no arquivo os dados de rede, IP, GT, SUBNET
    network["ip"]          = WiFi.localIP().toString();
    network["gateway"]     = WiFi.gatewayIP().toString();
    network["subnet"]      = WiFi.subnetMask().toString();

    File networkFile = SPIFFS.open("/network.json", "w");
    if (!networkFile) {
        Serial.println("Falha ao abrir arquivo");
    } else {
        network.printTo(networkFile);
    }
    networkFile.close();

    json["CONFIG"] = CONFIG;
    File configFile = SPIFFS.open("/config.json", "w");

    if (!configFile) {
        Serial.println("Falha ao abrir arquivo");
    } else {
        network.printTo(configFile);
    }
    configFile.close();

    SaveConfig = false;
  }

  //Print valores da rede
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());

  client.setServer(mqtt_server, mqtt_port);
  client.setKeepAlive(360);
  client.setCallback(mqtt_callback);
}
/***************************************************************
  LOOP
***************************************************************/
void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    initMQTT();
  }
  client.loop();
  if (SendCONFIG) {
    //Envia valor do CONFIG para o MQTT
    snprintf(buffer, 5, "%d", CONFIG);
    client.publish(SUB_CONFIG, buffer);
    SendCONFIG = false;
  }
  //FORÇA VALOR 0, ZERA LUZ E ANGULO
  if (CONFIG == 0) {
    servo.write(0);
    LUMENS = 0;
    light("Diminui");
  }
  if (!check("Igual")) {
    if ( check("Menor") ) {
      // Pouca luz internamente
      while (ANGULO < 180 && check("Menor")) {
        servo.write(ANGULO);
        ANGULO++;
        delay(50);
      }
      while (check("Menor") && ANGULO == 180 && LUMENS < 255) {
        LUMENS++;
        light("Aumenta");
        delay(50);
      }
    } else {
      //Muita luz internamente
      while (LUMENS > 0 && !check("Menor")) {
        LUMENS--;
        light("Diminui");
        delay(50);
      }
      while (!check("Menor") && ANGULO > 0 && LUMENS == 0) {
        servo.write(ANGULO);
        ANGULO--;
        delay(50);
      }
      return;
    }
    delay(1000);
  }

  /*
     ENCAMINHA PACOTES PARA O MQTT
  */
  //Envia angulo para o MQTT
  snprintf(buffer, 5, "%d", ANGULO / 2);
  client.publish(PUB_ANGULO, buffer);

  //Envia valor do Lumens para o MQTT
  snprintf(buffer, 5, "%d", map(LUMENS, 0, 255, 0, 100));
  client.publish(PUB_LUMENS, buffer);

  //Envia % de luz para o MQTT
  snprintf(buffer, 5, "%d", (int(((valorLDR()) * 100) / 1024)));
  client.publish(PUB_LUZ_PORCENT, buffer);

  //Envia valor do LDR para o MQTT
  snprintf(buffer, 5, "%d", valorLDR());
  client.publish(PUB_LDR, buffer);


  /*
    DELAY FINAL
  */
  Serial.println((String)"Delay 10s");
  delay(10000);
}
