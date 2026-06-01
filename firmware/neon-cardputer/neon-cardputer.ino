//=============================================================================
// NEON WIDGET — Firmware para M5CardPuter (versao minimalista)
//
// Hardware: M5CardPuter (ESP32-S3)
// Board Manager: M5Stack 3.2.2+
// Placa: M5Cardputer
//
// O que faz:
//   1. Liga a tela
//   2. Conecta no WiFi (redes configuradas no SD card)
//   3. Mostra "Neon ativa!" na tela
//=============================================================================

// Biblioteca oficial do M5CardPuter (ja inclui WiFi, tela, etc)
#include <M5Cardputer.h>
// Biblioteca pra ler o SD card
#include <SD.h>
#include <SPI.h>

// Pinos do SD card no M5CardPuter
#define SD_CS   GPIO_NUM_12
#define SD_MOSI GPIO_NUM_14
#define SD_MISO GPIO_NUM_40
#define SD_SCK  GPIO_NUM_39

// Caminho da config no SD
#define CONFIG_PATH "/neon/config.json"

//=============================================================================
// SETUP — roda uma vez quando liga
//=============================================================================
void setup() {
    // Inicia comunicacao serial (debug)
    Serial.begin(115200);
    Serial.println("\n=== Neon Widget ===");

    // Inicia o hardware do CardPuter
    M5Cardputer.begin();
    // Gira a tela pra posicao correta
    M5Cardputer.Display.setRotation(1);
    // Fundo preto
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    // Mostra texto inicial na tela
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5Cardputer.Display.setCursor(30, 50);
    M5Cardputer.Display.println("Neon");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(20, 75);
    M5Cardputer.Display.println("Conectando WiFi...");
    
    // Tenta montar o SD card
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    SD.begin(SD_CS, SPI);

    // Tenta conectar no WiFi
    conectarWiFi();

    // Se conseguiu conectar, mostra mensagem
    if (WiFi.isConnected()) {
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        M5Cardputer.Display.setCursor(30, 40);
        M5Cardputer.Display.println("Neon");
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Cardputer.Display.setCursor(20, 65);
        M5Cardputer.Display.println("Ativa!");
        M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.setCursor(10, 85);
        M5Cardputer.Display.println(WiFi.localIP().toString());
        
        Serial.println("WiFi conectado! IP: " + WiFi.localIP().toString());
    } else {
        M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5Cardputer.Display.setCursor(15, 100);
        M5Cardputer.Display.println("Sem WiFi");
    }

    Serial.println("Pronto!");
}

//=============================================================================
// LOOP — roda pra sempre
//=============================================================================
void loop() {
    // So precisa atualizar o hardware
    M5Cardputer.update();
    delay(100);
}

//=============================================================================
// CONECTAR WIFI
//=============================================================================
void conectarWiFi() {
    // 1. Tenta ler config do SD card
    if (SD.cardType() != CARD_NONE && SD.exists(CONFIG_PATH)) {
        lerConfigDoSD();
    }

    // 2. Se nao tem rede configurada, para por aqui
    if (WiFi.SSID().length() == 0) {
        Serial.println("Nenhuma rede configurada");
        return;
    }

    // 3. Tenta conectar (ate 15 segundos)
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
        delay(500);
        tentativas++;
        Serial.print(".");
    }
    Serial.println();
}

//=============================================================================
// LER CONFIG DO SD CARD
//=============================================================================
void lerConfigDoSD() {
    // Abre o arquivo
    File arquivo = SD.open(CONFIG_PATH, FILE_READ);
    if (!arquivo) return;

    // Le o conteudo
    String json = arquivo.readString();
    arquivo.close();

    // Procura o primeiro SSID e senha (formato simples)
    int ssidStart = json.indexOf("\"ssid\"");
    int passStart = json.indexOf("\"password\"");
    
    if (ssidStart > 0 && passStart > 0) {
        // Extrai SSID
        int inicio = json.indexOf("\"", ssidStart + 7) + 1;
        int fim = json.indexOf("\"", inicio);
        String ssid = json.substring(inicio, fim);
        
        // Extrai senha
        inicio = json.indexOf("\"", passStart + 10) + 1;
        fim = json.indexOf("\"", inicio);
        String senha = json.substring(inicio, fim);
        
        // Conecta
        Serial.print("Conectando em ");
        Serial.println(ssid);
        WiFi.begin(ssid.c_str(), senha.c_str());
    }
}
