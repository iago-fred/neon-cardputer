#ifndef SPRITEMANAGER_H
#define SPRITEMANAGER_H

#include <M5Cardputer.h>
#include <SD.h>

// ============================================================
// SpriteManager - Carrega PNGs do SD card para o avatar Neon
//
// Estrutura esperada no SD:
//   /neon/sprites/{emotion}.png
//
// Emoções: idle, happy, sad, surprised, thinking,
//          listening, sleep, error
//
// Se o sprite PNG não for encontrado, usa desenho programático.
// ============================================================

#define SPRITES_DIR "/neon/sprites"

class SpriteManager {
private:
    bool _sdAvailable = false;

public:
    SpriteManager() {}

    void init() {
        _sdAvailable = (SD.cardType() != CARD_NONE);
        if (_sdAvailable) {
            // Verifica se a pasta existe
            File dir = SD.open(SPRITES_DIR);
            if (!dir || !dir.isDirectory()) {
                Serial.printf("[Sprites] Pasta %s nao encontrada\n", SPRITES_DIR);
                _sdAvailable = false;
            } else {
                Serial.println("[Sprites] Pasta encontrada no SD");
                dir.close();
            }
        } else {
            Serial.println("[Sprites] SD indisponivel, usando desenho nativo");
        }
    }

    bool isAvailable() { return _sdAvailable; }

    // Tenta desenhar PNG do SD. Retorna true se conseguiu.
    bool draw(const char* emotion, int x, int y, int maxW = 0, int maxH = 0) {
        if (!_sdAvailable) return false;

        char path[64];
        snprintf(path, sizeof(path), "%s/%s.png", SPRITES_DIR, emotion);

        if (!SD.exists(path)) return false;

        File file = SD.open(path, FILE_READ);
        if (!file) return false;

        // Le o arquivo inteiro pra memoria (sprites sao pequenos < 20KB)
        size_t fileSize = file.size();
        uint8_t* buf = (uint8_t*)ps_malloc(fileSize);
        if (!buf) { file.close(); return false; }
        
        file.read(buf, fileSize);
        file.close();

        auto& gfx = M5Cardputer.Display;

        if (maxW > 0 && maxH > 0) {
            // Desenha com redimensionamento
            gfx.drawPng(buf, fileSize, x, y, maxW, maxH);
        } else {
            gfx.drawPng(buf, fileSize, x, y);
        }

        free(buf);
        return true;
    }

    // Nome do arquivo para cada emoção
    static const char* filenameForEmotion(int emotion) {
        static const char* names[] = {
            "idle", "happy", "sad", "surprised",
            "thinking", "listening", "sleep", "error"
        };
        if (emotion >= 0 && emotion < 8) return names[emotion];
        return "idle";
    }
};

#endif // SPRITEMANAGER_H
