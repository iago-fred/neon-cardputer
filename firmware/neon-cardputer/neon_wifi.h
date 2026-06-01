#ifndef NEON_WIFI_H
#define NEON_WIFI_H

#include <M5Cardputer.h>
#include <WiFi.h> // Agora o compilador vai achar a biblioteca certa!

// Executa o scan e tenta conectar nas redes enviadas via parâmetro (JSON)
void otimizarConexaoComScan(String conteudoJson) {
    if (conteudoJson == "") return;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);

    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 20);
    M5Cardputer.Display.setTextColor(TFT_CYAN);
    M5Cardputer.Display.println("Escaneando redes...");

    int n_redes_no_ar = WiFi.scanNetworks();
    if (n_redes_no_ar == 0) {
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.println("Nenhuma rede no ar!");
        return;
    }

    int index = 0;

    while ((index = conteudoJson.indexOf("\"ssid\"", index)) != -1) {
        int ssidInicio = conteudoJson.indexOf("\"", index + 7) + 1;
        int ssidFim = conteudoJson.indexOf("\"", ssidInicio);
        String ssidSalvo = conteudoJson.substring(ssidInicio, ssidFim);

        int passStart = conteudoJson.indexOf("\"password\"", ssidFim);
        int passInicio = conteudoJson.indexOf("\"", passStart + 11) + 1;
        int passFim = conteudoJson.indexOf("\"", passInicio);
        String senhaSalva = conteudoJson.substring(passInicio, passFim);

        ssidSalvo.trim();
        senhaSalva.trim();

        bool redeDisponivel = false;
        for (int i = 0; i < n_redes_no_ar; ++i) {
            if (WiFi.SSID(i) == ssidSalvo) {
                redeDisponivel = true;
                break;
            }
        }

        if (!redeDisponivel) {
            index = passFim;
            continue; 
        }

        M5Cardputer.Display.fillScreen(TFT_BLACK);
        M5Cardputer.Display.setCursor(0, 20);
        M5Cardputer.Display.setTextColor(TFT_GREEN);
        M5Cardputer.Display.printf("Rede encontrada!\n");
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.printf("Conectando em:\n%s\n", ssidSalvo.c_str());

        WiFi.begin(ssidSalvo.c_str(), senhaSalva.c_str());

        unsigned long start = millis();
        while (millis() - start < 10000) {
            if (WiFi.status() == WL_CONNECTED) {
                M5Cardputer.Display.fillScreen(TFT_BLACK);
                M5Cardputer.Display.setCursor(0, 20);
                M5Cardputer.Display.setTextColor(TFT_GREEN);
                M5Cardputer.Display.println("CONECTADO!");
                M5Cardputer.Display.setTextColor(TFT_WHITE);
                M5Cardputer.Display.printf("IP: %s", WiFi.localIP().toString().c_str());
                
                WiFi.scanDelete(); 
                return; 
            }
            delay(100);
        }
        index = passFim;
    }

    WiFi.scanDelete(); 

    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 20);
    M5Cardputer.Display.setTextColor(TFT_RED);
    M5Cardputer.Display.println("Redes salvas nao\nestao ao alcance.");
}

#endif