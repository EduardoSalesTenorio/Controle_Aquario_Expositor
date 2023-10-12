#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <TimeLib.h>

const char* ssid = "INACESSIVEL";  // Seu SSID da rede WiFi
const char* pass = "coisasboas";   // Sua senha da rede WiFi

static const char ntpServerName[] = "pool.ntp.org";  // Servidor NTP
const int timeZone = -3;                             // Fuso horário de Brasília

const char* host = "192.168.1.120";  // IP fixo

WiFiUDP Udp;
unsigned int localPort = 8888;  // Porta local para ouvir pacotes UDP

ESP8266WebServer server(80);

time_t getNtpTime();
void handleGetHora();


int ledAquarioPin = D2;    // Pino para o controle da iluminação do aquário
int bombaPin = D3;         // Pino para o controle da bomba do aquário
int bombaInternaPin = D4;  // Pino para o controle da bomba interna do aquário
int fitaLed = D5;          // Pino para controlar fita de LED que estará no expositor
int expositorLed1 = D6;
int expositorLed2 = D7;
int expositorLed3 = D8;
int expositorLed4 = D9;

bool offLine = true;
bool bombaInternaOnOf = true;

unsigned long lastActivationTime = 0;     // Variável para rastrear o tempo da última ativação
unsigned long activationInterval = 6000;  // Intervalo de ativação (3 minutos em milissegundos)

void setup() {

  pinMode(ledAquarioPin, OUTPUT);
  pinMode(bombaPin, OUTPUT);
  pinMode(bombaInternaPin, OUTPUT);
  pinMode(fitaLed, OUTPUT);
  pinMode(expositorLed1, OUTPUT);
  pinMode(expositorLed2, OUTPUT);
  pinMode(expositorLed3, OUTPUT);
  pinMode(expositorLed4, OUTPUT);

  randomSeed(analogRead(0));  // Inicializa a semente do gerador de números aleatórios

  Serial.begin(9600);
  while (!Serial)
    ;  // Aguarda a inicialização do monitor serial

  Serial.println("TimeNTP Example");
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configurar o IP fixo
  IPAddress staticIP(192, 168, 1, 120);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(staticIP, gateway, subnet);

  Serial.print("IP atribuído: ");
  Serial.println(WiFi.localIP());

  Udp.begin(localPort);
  Serial.print("Porta local: ");
  Serial.println(Udp.localPort());
  Serial.println("Aguardando sincronização");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);  // Intervalo de sincronização de 5 minutos

  server.on("/statusLedAquario", HTTP_GET, statusLedAquario);
  server.on("/statusBomba", HTTP_GET, statusBomba);
  server.on("/statusBombaInterna", HTTP_GET, statusBombaInterna);
  server.on("/statusFitaLed", HTTP_GET, statusFitaLed);
  server.on("/statusExpositorLed1", HTTP_GET, statusExpositorLed1);
  server.on("/statusExpositorLed2", HTTP_GET, statusExpositorLed2);
  server.on("/statusExpositorLed3", HTTP_GET, statusExpositorLed3);
  server.on("/statusExpositorLed4", HTTP_GET, statusExpositorLed4);
  server.on("/statusExpositorLed4", HTTP_GET, statusBombaInternaOnOf);

  server.on("/expositorLed4", HTTP_GET, []() {
    digitalWrite(expositorLed4, !digitalRead(expositorLed4));
    statusExpositorLed4();
  });

  server.on("/offLine", HTTP_GET, []() {
    digitalWrite(ledAquarioPin, LOW);
    digitalWrite(bombaPin, LOW);
    digitalWrite(bombaInternaPin, LOW);
    offLine = false;
    server.send(200, "text/html", "offLine");
  });

  server.on("/bombaInternaOff", HTTP_GET, []() {
    bombaInternaOnOf = !bombaInternaOnOf;
    statusBombaInternaOnOf();
  });

  server.on("/onLine", HTTP_GET, []() {
    offLine = true;
    server.send(200, "text/html", "onLine");
  });


  server.on("/ligarTodosLeds", HTTP_GET, []() {
    digitalWrite(expositorLed1, HIGH);
    digitalWrite(expositorLed2, HIGH);
    digitalWrite(expositorLed3, HIGH);
    digitalWrite(expositorLed4, HIGH);
    server.send(200, "text/html", "todosLedsLigados");
  });

  server.on("/DesligarTodosLeds", HTTP_GET, []() {
    digitalWrite(expositorLed1, LOW);
    digitalWrite(expositorLed2, LOW);
    digitalWrite(expositorLed3, LOW);
    digitalWrite(expositorLed4, LOW);
    server.send(200, "text/html", "todosLedsDesligados");
  });

  server.on("/ligarTodosLedseFitaLed", HTTP_GET, []() {
    digitalWrite(expositorLed1, HIGH);
    digitalWrite(expositorLed2, HIGH);
    digitalWrite(expositorLed3, HIGH);
    digitalWrite(expositorLed4, HIGH);
    digitalWrite(fitaLed, HIGH);
    server.send(200, "text/html", "todosLedsLigadosFitaLed");
  });

  server.on("/DesligarTodosLedseFitaLed", HTTP_GET, []() {
    digitalWrite(expositorLed1, LOW);
    digitalWrite(expositorLed2, LOW);
    digitalWrite(expositorLed3, LOW);
    digitalWrite(expositorLed4, LOW);
    digitalWrite(fitaLed, LOW);
    server.send(200, "text/html", "todosLedsDesligadosFitaLed");
  });

  server.on("/expositorLed3", HTTP_GET, []() {
    digitalWrite(expositorLed3, !digitalRead(expositorLed3));
    statusExpositorLed3();
  });

  server.on("/expositorLed2", HTTP_GET, []() {
    digitalWrite(expositorLed2, !digitalRead(expositorLed2));
    statusExpositorLed2();
  });

  server.on("/statusOnOff", HTTP_GET, []() {
    if (offLine) {
      server.send(200, "text/html", "onLine");
    } else {
      server.send(200, "text/html", "offLine");
    }
  });

  server.on("/expositorLed1", HTTP_GET, []() {
    digitalWrite(expositorLed1, !digitalRead(expositorLed1));
    statusExpositorLed1();
  });

  server.on("/fitaLed", HTTP_GET, []() {
    digitalWrite(fitaLed, !digitalRead(fitaLed));
    statusFitaLed();
  });

  server.on("/ledAquario", HTTP_GET, []() {
    digitalWrite(ledAquarioPin, !digitalRead(ledAquarioPin));
    activationInterval = 1200000;
    statusLedAquario();
  });

  server.on("/ligarBomba", HTTP_GET, []() {
    digitalWrite(bombaPin, !digitalRead(bombaPin));
    activationInterval = 1200000;
    statusBomba();
  });

  server.on("/ligarBombaInterna", HTTP_GET, []() {
    digitalWrite(bombaInternaPin, !digitalRead(bombaInternaPin));
    activationInterval = 1200000;
    statusBombaInterna();
  });

  // Codigos divertidos com led

  // Padrão 1: Todos os LEDs piscam juntos

  server.on("/piscamJuntos", HTTP_GET, []() {
    for (int i = 0; i < 5; i++) {

      digitalWrite(expositorLed1, HIGH);
      digitalWrite(expositorLed2, HIGH);
      digitalWrite(expositorLed3, HIGH);
      digitalWrite(expositorLed4, HIGH);

      delay(500);

      digitalWrite(expositorLed1, LOW);
      digitalWrite(expositorLed2, LOW);
      digitalWrite(expositorLed3, LOW);
      digitalWrite(expositorLed4, LOW);

      delay(500);
    }
    server.send(200, "text/html", "LesdPiscando");
  });

  // Padrão 2: LEDs piscam em sequência
  server.on("/piscamSequencia", HTTP_GET, []() {
    for (int i = 0; i < 5; i++) {

      digitalWrite(expositorLed1, HIGH);
      delay(200);
      digitalWrite(expositorLed1, LOW);
      delay(200);

      digitalWrite(expositorLed2, HIGH);
      delay(200);
      digitalWrite(expositorLed2, LOW);
      delay(200);

      digitalWrite(expositorLed3, HIGH);
      delay(200);
      digitalWrite(expositorLed3, LOW);
      delay(200);

      digitalWrite(expositorLed4, HIGH);
      delay(200);
      digitalWrite(expositorLed4, LOW);
      delay(200);
    }
    server.send(200, "text/html", "LesdPiscando");
  });

  // Padrão 3: LEDs piscam em grupos

  server.on("/piscamGrupo", HTTP_GET, []() {
    for (int i = 0; i < 5; i++) {

      digitalWrite(expositorLed1, HIGH);
      digitalWrite(expositorLed3, HIGH);
      delay(300);
      digitalWrite(expositorLed1, LOW);
      digitalWrite(expositorLed3, LOW);
      delay(300);

      digitalWrite(expositorLed2, HIGH);
      digitalWrite(expositorLed4, HIGH);
      delay(300);
      digitalWrite(expositorLed2, LOW);
      digitalWrite(expositorLed4, LOW);
      delay(300);
    }
    server.send(200, "text/html", "LesdPiscando");
  });


  server.on("/piscamDescendo", HTTP_GET, []() {
    for (int i = 0; i < 5; i++) {

      delay(200);
      digitalWrite(expositorLed1, HIGH);
      digitalWrite(expositorLed4, LOW);

      delay(200);
      digitalWrite(expositorLed2, HIGH);

      delay(200);
      digitalWrite(expositorLed3, HIGH);
      digitalWrite(expositorLed1, LOW);

      delay(200);
      digitalWrite(expositorLed4, HIGH);
      digitalWrite(expositorLed2, LOW);

      delay(200);
      digitalWrite(expositorLed3, LOW);
    }
    server.send(200, "text/html", "LesdPiscando");
  });

  server.on("/idaVolta", HTTP_GET, []() {
    for (int i = 0; i < 5; i++) {

      digitalWrite(expositorLed1, HIGH);
      delay(200);
      digitalWrite(expositorLed2, HIGH);
      delay(200);
      digitalWrite(expositorLed3, HIGH);
      delay(200);
      digitalWrite(expositorLed4, HIGH);
      delay(800);

      digitalWrite(expositorLed4, LOW);
      delay(200);
      digitalWrite(expositorLed3, LOW);
      delay(200);
      digitalWrite(expositorLed2, LOW);
      delay(200);
      digitalWrite(expositorLed1, LOW);
      delay(200);
    }
    server.send(200, "text/html", "LesdPiscando");
  });


  server.on("/vaiVem", HTTP_GET, []() {
    for (int i = 0; i < 5; i++) {

      digitalWrite(expositorLed1, HIGH);
      delay(200);
      digitalWrite(expositorLed2, HIGH);
      delay(200);
      digitalWrite(expositorLed3, HIGH);
      delay(200);
      digitalWrite(expositorLed4, HIGH);
      delay(800);

      digitalWrite(expositorLed1, LOW);
      delay(200);
      digitalWrite(expositorLed2, LOW);
      delay(200);
      digitalWrite(expositorLed3, LOW);
      delay(200);
      digitalWrite(expositorLed4, LOW);
      delay(600);
    }
    server.send(200, "text/html", "LesdPiscando");
  });






  // Configurar rota HTTP para obter a hora
  server.on("/getHora", HTTP_GET, handleGetHora);
  server.begin();
}

time_t prevDisplay = 0;

void loop() {
  server.handleClient();

  if (offLine && millis() - lastActivationTime >= activationInterval) {
    ligarDesligarAquario();
    activationInterval = 60000;
    lastActivationTime = millis();
  }
}

void handleGetHora() {
  // Obtém a hora atual
  int currentHour = hour();

  if (currentHour >= 14 && currentHour < 20) {
    server.send(200, "text/plain", "Hora certa muito certa");
  } else {
    server.send(200, "text/plain", "Hora errada e muito");
  }
}
/*-------- Código NTP ----------*/

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

time_t getNtpTime() {
  IPAddress ntpServerIP;

  while (Udp.parsePacket() > 0)
    ;

  Serial.println("Enviando Requisição NTP");
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();

  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Recebendo Resposta NTP");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long secsSince1900;
      secsSince1900 = (unsigned long)packetBuffer[43] | ((unsigned long)packetBuffer[42] << 8) | ((unsigned long)packetBuffer[41] << 16) | ((unsigned long)packetBuffer[40] << 24);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("Nenhuma Resposta NTP :-(");
  return 0;
}

void sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

//configurando rotas para status

void statusExpositorLed1() {
  if (digitalRead(expositorLed1) == 1) {
    server.send(200, "text/html", "expositorLed1Ligado");
  } else {
    server.send(200, "text/html", "expositorLed1Desligado");
  }
}
void statusExpositorLed2() {
  if (digitalRead(expositorLed2) == 1) {
    server.send(200, "text/html", "expositorLed2Ligado");
  } else {
    server.send(200, "text/html", "expositorLed2Desligado");
  }
}

void statusExpositorLed3() {
  if (digitalRead(expositorLed3) == 1) {
    server.send(200, "text/html", "expositorLed3Ligado");
  } else {
    server.send(200, "text/html", "expositorLed3Desligado");
  }
}

void statusExpositorLed4() {
  if (digitalRead(expositorLed4) == 1) {
    server.send(200, "text/html", "expositorLed4Ligado");
  } else {
    server.send(200, "text/html", "expositorLed4Desligado");
  }
}

void statusFitaLed() {
  if (digitalRead(fitaLed) == 1) {
    server.send(200, "text/html", "fitaLedLigada");
  } else {
    server.send(200, "text/html", "fitaLedDesligada");
  }
}

void statusLedAquario() {
  if (digitalRead(ledAquarioPin) == 1) {
    server.send(200, "text/html", "ledAquarioLigado");
  } else {
    server.send(200, "text/html", "ledAquarioDesligado");
  }
}

void statusBombaInternaOnOf() {
  if (bombaInternaOnOf) {
    server.send(200, "text/html", "bombaInternaOffLine");
  } else {
    server.send(200, "text/html", "bombaInternaOnLine");
  }
}

void statusBomba() {
  if (digitalRead(bombaPin) == 1) {
    server.send(200, "text/html", "bombaLigada");
  } else {
    server.send(200, "text/html", "bombaDesligada");
  }
}

void statusBombaInterna() {

  if (bombaInternaOnOf) {
    if (digitalRead(bombaInternaPin) == 1) {
      server.send(200, "text/html", "bombainternaLigada");
    } else {
      server.send(200, "text/html", "bombaInternaDesligada");
    }
  } else {
    server.send(200, "text/html", "bombaInternaDesligadaPausa");
  }
}

//Verificar hora
void ligarDesligarAquario() {

  int currentHour = hour();

  if (currentHour >= 13 && currentHour < 19) {
    digitalWrite(ledAquarioPin, HIGH);
    digitalWrite(bombaPin, HIGH);

    if (bombaInternaOnOf) {
      digitalWrite(bombaInternaPin, HIGH);
    }
  } else {
    digitalWrite(ledAquarioPin, LOW);
    digitalWrite(bombaPin, LOW);
    digitalWrite(bombaInternaPin, LOW);
    bombaInternaOnOf = true;
  }
}