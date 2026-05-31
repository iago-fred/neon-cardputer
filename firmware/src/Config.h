#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

// ============================================================
// Estruturas
// ============================================================
struct WiFiCredential {
    char ssid[32];
    char password[64];
};

// ============================================================
// ConfigManager
// ============================================================
class ConfigManager {
private:
    std::vector<WiFiCredential> _wifiNetworks;
    char _serverHost[64] = "il-separacao-python.onrender.com";
    int  _serverPort = 443;
    char _audioEndpoint[128] = "/api/neon/audio";
    char _pollEndpoint[128] = "/api/neon/poll";
    bool _soundEnabled = true;
    uint8_t _brightness = 100;
    
public:
    ConfigManager() {}
    
    void setDefaults() {
        _wifiNetworks.clear();
        strcpy(_serverHost, "il-separacao-python.onrender.com");
        _serverPort = 443;
        strcpy(_audioEndpoint, "/api/neon/audio");
        strcpy(_pollEndpoint, "/api/neon/poll");
        _soundEnabled = true;
        _brightness = 100;
    }
    
    void load(JsonDocument& doc) {
        _wifiNetworks.clear();
        
        if (doc["wifi"].is<JsonArray>()) {
            for (auto& net : doc["wifi"].as<JsonArray>()) {
                WiFiCredential cred;
                strlcpy(cred.ssid, net["ssid"] | "", sizeof(cred.ssid));
                strlcpy(cred.password, net["password"] | "", sizeof(cred.password));
                _wifiNetworks.push_back(cred);
            }
        }
        
        strlcpy(_serverHost, doc["server_host"] | _serverHost, sizeof(_serverHost));
        _serverPort = doc["server_port"] | 443;
        strlcpy(_audioEndpoint, doc["audio_endpoint"] | _audioEndpoint, sizeof(_audioEndpoint));
        strlcpy(_pollEndpoint, doc["poll_endpoint"] | _pollEndpoint, sizeof(_pollEndpoint));
        _soundEnabled = doc["sound_enabled"] | true;
        _brightness = doc["brightness"] | 100;
    }
    
    void save(JsonDocument& doc) {
        JsonArray wifi = doc["wifi"].to<JsonArray>();
        for (auto& net : _wifiNetworks) {
            JsonObject obj = wifi.add<JsonObject>();
            obj["ssid"] = net.ssid;
            obj["password"] = net.password;
        }
        
        doc["server_host"] = _serverHost;
        doc["server_port"] = _serverPort;
        doc["audio_endpoint"] = _audioEndpoint;
        doc["poll_endpoint"] = _pollEndpoint;
        doc["sound_enabled"] = _soundEnabled;
        doc["brightness"] = _brightness;
    }
    
    void addWiFi(const char* ssid, const char* password) {
        // Não duplicar
        for (auto& net : _wifiNetworks) {
            if (strcmp(net.ssid, ssid) == 0) return;
        }
        WiFiCredential cred;
        strlcpy(cred.ssid, ssid, sizeof(cred.ssid));
        strlcpy(cred.password, password, sizeof(cred.password));
        _wifiNetworks.push_back(cred);
    }
    
    std::vector<WiFiCredential>& getWiFiNetworks() { return _wifiNetworks; }
    
    const char* getServerHost() { return _serverHost; }
    int getServerPort() { return _serverPort; }
    const char* getAudioEndpoint() { return _audioEndpoint; }
    const char* getPollEndpoint() { return _pollEndpoint; }
    bool isSoundEnabled() { return _soundEnabled; }
    void setSoundEnabled(bool e) { _soundEnabled = e; }
    uint8_t getBrightness() { return _brightness; }
    void setBrightness(uint8_t b) { _brightness = b; }
};

#endif // CONFIG_H
