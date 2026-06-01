//=============================================================================
// NEON CARD — Firmware para M5CardPuter
//
// Conversa com a Neon via Telegram + Bridge HTTP
//
// Hardware: M5CardPuter (ESP32-S3)
// Board Manager: M5Stack 3.2.2+
// Placa: M5Cardputer
//=============================================================================

#include <M5Cardputer.h>
#include "config.h"
#include "neon_wifi.h"
#include "telegram.h"

// ── Splash de boot ─────────────────────────────────────────────────────────
void mostrarSplash() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    
    // Ícone Neon (carinha no centro)
    M5Cardputer.Display.fillCircle(120, 45, 18, TFT_CYAN);
    M5Cardputer.Display.drawCircle(120, 45, 18, TFT_BLUE);
    M5Cardputer.Display.fillCircle(113, 40, 5, TFT_WHITE);  // olho esquerdo
    M5Cardputer.Display.fillCircle(127, 40, 5, TFT_WHITE);   // olho direito
    M5Cardputer.Display.fillCircle(113, 40, 2, TFT_BLUE);   // pupila
    M5Cardputer.Display.fillCircle(127, 40, 2, TFT_BLUE);
    M5Cardputer.Display.drawLine(115, 50, 120, 53, TFT_BLUE); // sorriso
    M5Cardputer.Display.drawLine(120, 53, 125, 50, TFT_BLUE);
    
    // Nome
    M5Cardputer.Display.setTextSize(3);
    M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5Cardputer.Display.setCursor(55, 70);
    M5Cardputer.Display.print("Neon");
    
    // Status
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.setCursor(40, 100);
    M5Cardputer.Display.print("Inicializando...");
    
    delay(1500);
}

// ── Setup ──────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Neon Card ===");
    
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    
    // Splash de boot
    mostrarSplash();
    
    // Tenta ler o SD
    if (inicializarSD()) {
        String dadosJson = lerConteudoConfig();
        
        // Conecta WiFi com scan inteligente
        otimizarConexaoComScan(dadosJson);
        
        // Inicia o chat (bridge + Telegram)
        if (WiFi.status() == WL_CONNECTED || dadosJson != "") {
            iniciarChatTelegram(dadosJson);
        }
    }
    
    // Se chegou aqui, algo deu errado — tela idle
    // (iniciarChatTelegram tem loop próprio, então isso é fallback)
}

// ── Loop (fallback) ────────────────────────────────────────────────────────
void loop() {
    M5Cardputer.update();
    delay(100);
}
