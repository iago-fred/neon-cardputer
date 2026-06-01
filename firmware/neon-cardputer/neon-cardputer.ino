#include <M5Cardputer.h>
#include "config.h"
#include "neon_wifi.h"
#include "telegram.h" // Inclui o novo módulo

void setup() {
    Serial.begin(115200);
    
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    
    if (inicializarSD()) {
        String dadosJson = lerConteudoConfig();
        
        // 1. Tenta conectar ao Wi-Fi usando o Scan Inteligente
        otimizarConexaoComScan(dadosJson);
        
        // 2. Se a conexão acima foi bem-sucedida, inicia o chat do Telegram
        if (WiFi.status() == WL_CONNECTED) {
            iniciarChatTelegram(dadosJson);
        }
    }
}

void loop() {
    // Como o 'iniciarChatTelegram' possui seu próprio loop infinito controlado,
    // o código só chegará aqui se o Wi-Fi falhar por completo.
    M5Cardputer.update();
    delay(100);
}