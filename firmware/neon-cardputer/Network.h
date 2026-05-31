#ifndef NETWORK_H
#define NETWORK_H

#include <M5Cardputer.h>
#include <WiFi.h>
#include "Config.h"

// ============================================================
// NetworkManager — Gerenciamento de conexão WiFi
//
// Gerencia reconexão automática e verificação de status.
// As funções de envio HTTP estão no main.cpp diretamente.
// ============================================================
class NetworkManager {
private:
    ConfigManager* _config;
    uint32_t _lastReconnectAttempt = 0;
    
public:
    NetworkManager(ConfigManager* config) : _config(config) {}
    
    void update() {
        // Tenta reconectar se WiFi caiu
        if (WiFi.status() != WL_CONNECTED && 
            millis() - _lastReconnectAttempt > 30000) {
            _lastReconnectAttempt = millis();
            Serial.println("[Network] WiFi perdido, reconectando...");
            WiFi.reconnect();
        }
    }
    
    bool isConnected() {
        return WiFi.status() == WL_CONNECTED;
    }
};

#endif // NETWORK_H
