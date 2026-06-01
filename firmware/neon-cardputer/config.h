#ifndef CONFIG_H
#define CONFIG_H

#include <M5Cardputer.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// Pinos do SD do M5CardPuter
#define SD_CS   12
#define SD_SCK  40
#define SD_MISO 39
#define SD_MOSI 14

const char* CONFIG_PATH = "/neon/config.json";

// Inicializa o barramento SPI e o Cartão SD
bool inicializarSD() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, SPI)) {
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.setCursor(0, 20);
        M5Cardputer.Display.println("Erro: SD nao encontrado!");
        return false;
    }
    return true;
}

// Lê o arquivo JSON do SD e retorna como String
String lerConteudoConfig() {
    File arquivo = SD.open(CONFIG_PATH, FILE_READ);
    if (!arquivo) {
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.setCursor(0, 20);
        M5Cardputer.Display.println("Erro: Arquivo JSON nao aberto!");
        return "";
    }
    String conteudo = arquivo.readString();
    arquivo.close();
    return conteudo;
}

#endif