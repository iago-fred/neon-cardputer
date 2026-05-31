#ifndef OTAUPDATE_H
#define OTAUPDATE_H

#include <M5Cardputer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include "Version.h"

// ============================================================
// OTAUpdateManager
//
// Gerencia atualização automática via GitHub Releases.
// No boot, verifica se há versão mais nova; se sim, baixa
// o firmware.bin e aplica OTA.
// ============================================================
class OTAUpdateManager {
private:
    bool _updateAvailable = false;
    String _latestTag;
    String _downloadUrl;
    String _releaseNotes;
    int _totalFirmwareSize = 0;
    
    // Callback para progresso
    using ProgressCallback = void (*)(int progress, int total);
    ProgressCallback _progressCallback = nullptr;
    
public:
    OTAUpdateManager() {}
    
    void setProgressCallback(ProgressCallback cb) {
        _progressCallback = cb;
    }
    
    bool isUpdateAvailable() { return _updateAvailable; }
    String getLatestTag() { return _latestTag; }
    String getReleaseNotes() { return _releaseNotes; }
    
    // ============================================================
    // Verifica GitHub Releases
    // ============================================================
    bool checkForUpdate() {
        _updateAvailable = false;
        
        if (!WiFi.isConnected()) {
            Serial.println("[OTA] ❌ Sem WiFi, pulando verificação");
            return false;
        }
        
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure(); // Para HTTPS sem certificado
        
        http.begin(client, GITHUB_API_URL);
        http.addHeader("Accept", "application/json");
        http.addHeader("User-Agent", "NeonWidget/" FIRMWARE_VERSION_STR);
        
        Serial.printf("[OTA] Verificando GitHub API: %s\n", GITHUB_API_URL);
        
        int httpCode = http.GET();
        
        if (httpCode != 200) {
            Serial.printf("[OTA] ❌ HTTP %d\n", httpCode);
            http.end();
            return false;
        }
        
        String payload = http.getString();
        http.end();
        
        // Parse JSON manualmente (sem depender do ArduinoJson pra não aumentar o tamanho)
        _latestTag = extractJsonString(payload, "\"tag_name\"");
        _releaseNotes = extractJsonString(payload, "\"body\"");
        
        // Encontra URL do asset firmware.bin
        _downloadUrl = findAssetUrl(payload, "firmware.bin");
        
        if (_latestTag.isEmpty()) {
            Serial.println("[OTA] ❌ tag_name não encontrado");
            return false;
        }
        
        // Parse versão remota
        FirmwareVersion remote = parseVersion(_latestTag.c_str());
        FirmwareVersion current = getCurrentVersion();
        
        Serial.printf("[OTA] Atual: %s | Remoto: %s\n", 
                      getCurrentVersionStr(), _latestTag.c_str());
        
        _updateAvailable = isNewerVersion(current, remote);
        
        if (_updateAvailable) {
            Serial.printf("[OTA] 🆕 Atualização disponível: %s\n", _latestTag.c_str());
            Serial.printf("[OTA] URL: %s\n", _downloadUrl.c_str());
            _totalFirmwareSize = getAssetSize(payload, "firmware.bin");
        } else {
            Serial.println("[OTA] ✅ Firmware atualizado");
        }
        
        return _updateAvailable;
    }
    
    // ============================================================
    // Aplica OTA
    // ============================================================
    bool applyUpdate() {
        if (!_updateAvailable || _downloadUrl.isEmpty()) {
            Serial.println("[OTA] ❌ Nada para atualizar");
            return false;
        }
        
        if (!WiFi.isConnected()) {
            Serial.println("[OTA] ❌ Sem WiFi");
            return false;
        }
        
        Serial.printf("[OTA] ⬇️ Baixando firmware de: %s\n", _downloadUrl.c_str());
        
        WiFiClientSecure client;
        client.setInsecure();
        
        HTTPClient http;
        http.begin(client, _downloadUrl);
        http.addHeader("Accept", "application/octet-stream");
        http.addHeader("User-Agent", "NeonWidget/" FIRMWARE_VERSION_STR);
        
        int httpCode = http.GET();
        if (httpCode != 200) {
            Serial.printf("[OTA] ❌ HTTP %d ao baixar firmware\n", httpCode);
            http.end();
            return false;
        }
        
        int contentLength = http.getSize();
        if (contentLength <= 0) {
            Serial.println("[OTA] ❌ Content-Length inválido");
            http.end();
            return false;
        }
        
        Serial.printf("[OTA] Tamanho: %d bytes\n", contentLength);
        
        if (!Update.begin(contentLength)) {
            Serial.printf("[OTA] ❌ Update.begin() falhou: %s\n", Update.errorString());
            http.end();
            return false;
        }
        
        WiFiClient* stream = http.getStreamPtr();
        uint8_t buffer[1024];
        int written = 0;
        
        while (http.connected() && written < contentLength) {
            int available = stream->available();
            if (available > 0) {
                int bytesRead = stream->readBytes(
                    buffer, 
                    min((size_t)available, sizeof(buffer))
                );
                
                if (Update.write(buffer, bytesRead) != bytesRead) {
                    Serial.printf("[OTA] ❌ Erro na escrita: %s\n", Update.errorString());
                    http.end();
                    return false;
                }
                
                written += bytesRead;
                
                if (_progressCallback) {
                    _progressCallback(written * 100 / contentLength, 100);
                }
                
                Serial.printf("[OTA] ⏳ %d / %d bytes (%d%%)\n", 
                              written, contentLength, written * 100 / contentLength);
            }
            delay(1);
        }
        
        http.end();
        
        if (!Update.end()) {
            Serial.printf("[OTA] ❌ Update.end() falhou: %s\n", Update.errorString());
            return false;
        }
        
        if (!Update.isFinished()) {
            Serial.println("[OTA] ❌ Update não finalizado");
            return false;
        }
        
        Serial.println("[OTA] ✅ Atualização concluída! Reiniciando...");
        delay(500);
        ESP.restart();
        
        return true;
    }
    
private:
    // ============================================================
    // Parse JSON simples (sem dependências)
    // ============================================================
    String extractJsonString(const String& json, const String& key) {
        int keyPos = json.indexOf(key);
        if (keyPos < 0) return "";
        
        int valStart = json.indexOf('"', keyPos + key.length() + 1);
        if (valStart < 0) return "";
        
        int valEnd = json.indexOf('"', valStart + 1);
        if (valEnd < 0) return "";
        
        return json.substring(valStart + 1, valEnd);
    }
    
    String findAssetUrl(const String& json, const String& assetName) {
        // Procura no array "assets" por um objeto com "name" = assetName
        // e extrai "browser_download_url"
        int assetsPos = json.indexOf("\"assets\"");
        if (assetsPos < 0) return "";
        
        String assetsSection = json.substring(assetsPos);
        int namePos = 0;
        
        while (true) {
            namePos = assetsSection.indexOf("\"name\"", namePos);
            if (namePos < 0) break;
            
            int valStart = assetsSection.indexOf('"', namePos + 7);
            if (valStart < 0) break;
            int valEnd = assetsSection.indexOf('"', valStart + 1);
            if (valEnd < 0) break;
            
            String name = assetsSection.substring(valStart + 1, valEnd);
            
            if (name == assetName) {
                int urlPos = assetsSection.indexOf("\"browser_download_url\"", valEnd);
                if (urlPos < 0) break;
                
                int urlStart = assetsSection.indexOf('"', urlPos + 21);
                if (urlStart < 0) break;
                int urlEnd = assetsSection.indexOf('"', urlStart + 1);
                if (urlEnd < 0) break;
                
                return assetsSection.substring(urlStart + 1, urlEnd);
            }
            
            namePos = valEnd + 1;
        }
        
        return "";
    }
    
    int getAssetSize(const String& json, const String& assetName) {
        int assetsPos = json.indexOf("\"assets\"");
        if (assetsPos < 0) return 0;
        
        String assetsSection = json.substring(assetsPos);
        int namePos = 0;
        
        while (true) {
            namePos = assetsSection.indexOf("\"name\"", namePos);
            if (namePos < 0) break;
            
            int valStart = assetsSection.indexOf('"', namePos + 7);
            if (valStart < 0) break;
            int valEnd = assetsSection.indexOf('"', valStart + 1);
            if (valEnd < 0) break;
            
            String name = assetsSection.substring(valStart + 1, valEnd);
            
            if (name == assetName) {
                int sizePos = assetsSection.indexOf("\"size\"", valEnd);
                if (sizePos < 0) return 0;
                
                int numStart = assetsSection.indexOf(':', sizePos + 6);
                if (numStart < 0) return 0;
                numStart++;
                while (numStart < (int)assetsSection.length() && 
                       (assetsSection[numStart] == ' ' || assetsSection[numStart] == '\t')) {
                    numStart++;
                }
                
                int numEnd = numStart;
                while (numEnd < (int)assetsSection.length() && 
                       isDigit(assetsSection[numEnd])) {
                    numEnd++;
                }
                
                return assetsSection.substring(numStart, numEnd).toInt();
            }
            
            namePos = valEnd + 1;
        }
        
        return 0;
    }
    
    bool isDigit(char c) {
        return c >= '0' && c <= '9';
    }
};

#endif // OTAUPDATE_H
