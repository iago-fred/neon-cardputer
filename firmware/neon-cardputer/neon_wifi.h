#ifndef NEON_WIFI_H
#define NEON_WIFI_H

#include <M5Cardputer.h>
#include <WiFi.h>

// ── Animação de loading ────────────────────────────────────────────────────
void animacaoLoading(int x, int y, int frame) {
    const char* spin = "|/-\\";
    M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5Cardputer.Display.setCursor(x, y);
    M5Cardputer.Display.print(spin[frame % 4]);
}

// ── Scan inteligente e conexão ─────────────────────────────────────────────
void otimizarConexaoComScan(String conteudoJson) {
    if (conteudoJson == "") return;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);

    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5Cardputer.Display.setCursor(30, 20);
    M5Cardputer.Display.print("Neon");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(20, 50);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.print("Escaneando redes...");

    int n_redes_no_ar = WiFi.scanNetworks();
    if (n_redes_no_ar == 0) {
        M5Cardputer.Display.setCursor(10, 70);
        M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5Cardputer.Display.print("Nenhuma rede WiFi");
        M5Cardputer.Display.setCursor(10, 85);
        M5Cardputer.Display.print("encontrada!");
        delay(3000);
        return;
    }

    int index = 0;
    int spinFrame = 0;

    while ((index = conteudoJson.indexOf("\"ssid\"", index)) != -1) {
        int ssidInicio = conteudoJson.indexOf("\"", index + 7) + 1;
        int ssidFim = conteudoJson.indexOf("\"", ssidInicio);
        if (ssidFim == -1) break;
        String ssidSalvo = conteudoJson.substring(ssidInicio, ssidFim);

        int passStart = conteudoJson.indexOf("\"password\"", ssidFim);
        if (passStart == -1) break;
        int passInicio = conteudoJson.indexOf("\"", passStart + 11) + 1;
        int passFim = conteudoJson.indexOf("\"", passInicio);
        if (passFim == -1) break;
        String senhaSalva = conteudoJson.substring(passInicio, passFim);

        ssidSalvo.trim();
        senhaSalva.trim();

        // Procura se a rede está disponível
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

        // Mostra "Conectando..." com animação
        M5Cardputer.Display.fillRect(0, 65, 240, 50, TFT_BLACK);
        M5Cardputer.Display.setCursor(10, 68);
        M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        M5Cardputer.Display.printf("Rede: %s", ssidSalvo.c_str());
        M5Cardputer.Display.setCursor(10, 83);
        M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.print("Conectando");

        WiFi.begin(ssidSalvo.c_str(), senhaSalva.c_str());

        // Aguarda até 15s com spinner
        unsigned long start = millis();
        while (millis() - start < 15000) {
            if (WiFi.status() == WL_CONNECTED) {
                WiFi.scanDelete();
                
                M5Cardputer.Display.fillScreen(TFT_BLACK);
                M5Cardputer.Display.setTextSize(2);
                M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
                M5Cardputer.Display.setCursor(30, 30);
                M5Cardputer.Display.println("Conectado!");
                M5Cardputer.Display.setTextSize(1);
                M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
                M5Cardputer.Display.setCursor(10, 60);
                M5Cardputer.Display.printf("IP: %s", WiFi.localIP().toString().c_str());
                M5Cardputer.Display.setCursor(10, 80);
                M5Cardputer.Display.printf("SSID: %s", ssidSalvo.c_str());
                delay(1500);
                return;
            }
            
            // Animação de loading
            M5Cardputer.Display.setCursor(120, 83);
            const char* spin = "|/-\\";
            M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
            M5Cardputer.Display.print(spin[spinFrame % 4]);
            spinFrame++;
            
            delay(150);
        }
        index = passFim;
    }

    WiFi.scanDelete();

    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
    M5Cardputer.Display.setCursor(20, 40);
    M5Cardputer.Display.println("Sem WiFi :(");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setCursor(10, 65);
    M5Cardputer.Display.println("Redes salvas nao");
    M5Cardputer.Display.setCursor(10, 80);
    M5Cardputer.Display.println("estao disponiveis.");
    delay(3000);
}

#endif
