#ifndef AVATAR_H
#define AVATAR_H

#include <M5Cardputer.h>
#include "Display.h"

// ============================================================
// Emoções do Avatar Neon
// ============================================================
enum AvatarEmotion {
    AVATAR_IDLE,
    AVATAR_HAPPY,
    AVATAR_SAD,
    AVATAR_SURPRISED,
    AVATAR_THINKING,
    AVATAR_LISTENING,
    AVATAR_SLEEP,
    AVATAR_ERROR,
    AVATAR_COUNT
};

const char* EMOTION_NAMES[AVATAR_COUNT] = {
    "idle",
    "happy",
    "sad",
    "surprised",
    "thinking",
    "listening",
    "sleep",
    "error"
};

// ============================================================
// Avatar — Desenha a Neon na tela
// ============================================================
class Avatar {
private:
    DisplayManager* _display;
    AvatarEmotion _currentEmotion = AVATAR_IDLE;
    bool _eyesOpen = true;
    float _breathePhase = 0.0f;
    
    // Posição do avatar na tela
    int _x = 70;
    int _y = 40;
    
    // Timing
    uint32_t _emotionStart = 0;
    uint32_t _emotionDuration = 8000; // Volta ao idle após 8s
    
public:
    Avatar(DisplayManager* display) : _display(display) {}
    
    void setEmotion(AvatarEmotion emotion) {
        _currentEmotion = emotion;
        _emotionStart = millis();
    }
    
    void setEmotionByName(const char* name) {
        for (int i = 0; i < AVATAR_COUNT; i++) {
            if (strcmp(EMOTION_NAMES[i], name) == 0) {
                setEmotion((AvatarEmotion)i);
                return;
            }
        }
        setEmotion(AVATAR_IDLE);
    }
    
    void update() {
        // Volta ao idle depois de um tempo
        if (_currentEmotion != AVATAR_IDLE && 
            millis() - _emotionStart > _emotionDuration) {
            _currentEmotion = AVATAR_IDLE;
        }
        
        // Piscada natural sem delay() bloqueante
        // 4s aberto + 150ms fechado = ciclo de 4150ms
        uint32_t blinkCycle = millis() % 4150;
        _eyesOpen = blinkCycle < 4000;
        
        // Respiração (suave)
        if (_currentEmotion == AVATAR_IDLE || _currentEmotion == AVATAR_SLEEP) {
            _breathePhase += 0.02f;
        }
        
        draw();
    }
    
    void draw() {
        switch (_currentEmotion) {
            case AVATAR_HAPPY:      drawHappy();      break;
            case AVATAR_SAD:        drawSad();        break;
            case AVATAR_SURPRISED:  drawSurprised();   break;
            case AVATAR_THINKING:   drawThinking();    break;
            case AVATAR_LISTENING:  drawListening();   break;
            case AVATAR_SLEEP:      drawSleep();       break;
            case AVATAR_ERROR:      drawError();       break;
            case AVATAR_IDLE:
            default:                drawIdle();        break;
        }
    }
    
private:
    void drawBaseGhost(int eyeY = 22, bool openEyes = true) {
        auto& gfx = M5Cardputer.Display;
        int cx = _x, cy = _y;
        
        // Corpo fantasma (formato arredondado)
        gfx.fillCircle(cx, cy + 15, 20, TFT_CYAN);
        gfx.fillRect(cx - 20, cy + 15, 40, 18, TFT_CYAN);
        gfx.fillTriangle(cx - 10, cy + 33, cx, cy + 42, cx + 10, cy + 33, TFT_CYAN);
        gfx.fillTriangle(cx - 15, cy + 33, cx - 5, cy + 42, cx + 5, cy + 33, TFT_CYAN);
        gfx.fillTriangle(cx + 5, cy + 33, cx + 15, cy + 42, cx + 20, cy + 33, TFT_CYAN);
        
        // Olhos
        if (openEyes) {
            // Brilho nos olhos
            gfx.fillCircle(cx - 7, eyeY, 3, TFT_WHITE);
            gfx.fillCircle(cx + 7, eyeY, 3, TFT_WHITE);
            gfx.fillCircle(cx - 7, eyeY, 1, TFT_BLACK);
            gfx.fillCircle(cx + 7, eyeY, 1, TFT_BLACK);
        } else {
            gfx.drawLine(cx - 10, eyeY, cx - 4, eyeY, TFT_WHITE);
            gfx.drawLine(cx + 4, eyeY, cx + 10, eyeY, TFT_WHITE);
        }
        
        // Touca (beanie)
        gfx.fillRect(cx - 14, cy - 5, 28, 8, TFT_BLACK);
        gfx.fillCircle(cx, cy - 1, 15, TFT_BLACK);
        gfx.fillRect(cx - 15, cy - 5, 30, 3, TFT_DARKGREY);
    }
    
    void drawIdle() {
        auto& gfx = M5Cardputer.Display;
        // Fundo da área
        gfx.fillRect(_x - 30, _y - 20, 60, 70, _display->getBgColor());
        
        // Brilho suave (respiração)
        uint8_t glow = 30 + (sin(_breathePhase) * 20);
        gfx.drawCircle(_x, _y + 15, 22, gfx.color565(0, glow, glow));
        
        drawBaseGhost(22, _eyesOpen);
    }
    
    void drawHappy() {
        auto& gfx = M5Cardputer.Display;
        gfx.fillRect(_x - 30, _y - 20, 60, 70, _display->getBgColor());
        
        // Brilho extra (feliz)
        gfx.drawCircle(_x, _y + 15, 25, TFT_YELLOW);
        
        drawBaseGhost(20, true);
        
        // Sorriso maior
        gfx.drawCircle(_x, 35, 5, TFT_WHITE);
        gfx.fillCircle(_x, 36, 4, TFT_CYAN);
    }
    
    void drawSad() {
        drawBaseGhost(24, true);
        // Boca triste (arco invertido)
        auto& gfx = M5Cardputer.Display;
        gfx.drawLine(_x - 4, 38, _x + 4, 38, TFT_WHITE);
        gfx.drawLine(_x - 4, 38, _x - 2, 40, TFT_WHITE);
        gfx.drawLine(_x + 4, 38, _x + 2, 40, TFT_WHITE);
    }
    
    void drawSurprised() {
        drawBaseGhost(18, true);
        // Olhos maiores
        auto& gfx = M5Cardputer.Display;
        gfx.fillCircle(_x - 7, 22, 5, TFT_WHITE);
        gfx.fillCircle(_x + 7, 22, 5, TFT_WHITE);
        gfx.fillCircle(_x - 7, 22, 2, TFT_BLACK);
        gfx.fillCircle(_x + 7, 22, 2, TFT_BLACK);
        // Boca "O"
        gfx.drawCircle(_x, 38, 4, TFT_WHITE);
    }
    
    void drawThinking() {
        drawBaseGhost(22, true);
        // Bolha de pensamento
        auto& gfx = M5Cardputer.Display;
        gfx.drawCircle(_x + 22, _y - 10, 3, TFT_WHITE);
        gfx.drawCircle(_x + 28, _y - 16, 5, TFT_WHITE);
        gfx.drawCircle(_x + 35, _y - 22, 10, TFT_WHITE);
        gfx.fillCircle(_x + 35, _y - 22, 9, _display->getBgColor());
    }
    
    void drawListening() {
        // Ícone de áudio ao lado
        drawBaseGhost(22, true);
        auto& gfx = M5Cardputer.Display;
        // "Ondas sonoras"
        int mx = _x + 25;
        int my = _y + 10;
        gfx.drawArc(mx, my, 5, 3, -45, 45, TFT_GREEN, TFT_BLACK);
        gfx.drawArc(mx, my, 9, 7, -45, 45, TFT_GREEN, TFT_BLACK);
        gfx.drawArc(mx, my, 13, 11, -45, 45, TFT_GREEN, TFT_BLACK);
    }
    
    void drawSleep() {
        auto& gfx = M5Cardputer.Display;
        gfx.fillRect(_x - 30, _y - 20, 60, 70, _display->getBgColor());
        
        // Brilho fraco
        uint8_t dim = 10 + (sin(_breathePhase * 0.5f) * 5);
        gfx.drawCircle(_x, _y + 15, 22, gfx.color565(0, dim, dim));
        
        drawBaseGhost(22, false);
        // "Z z z"
        gfx.drawChar('Z', _x + 22, _y - 8, 1);
        gfx.drawChar('z', _x + 28, _y - 2, 1);
        gfx.drawChar('z', _x + 34, _y + 4, 1);
    }
    
    void drawError() {
        auto& gfx = M5Cardputer.Display;
        // Tom avermelhado
        gfx.drawCircle(_x, _y + 15, 23, TFT_RED);
        gfx.drawCircle(_x, _y + 15, 24, TFT_RED);
        
        // Olhos "X X"
        gfx.drawLine(_x - 11, 19, _x - 3, 27, TFT_RED);
        gfx.drawLine(_x - 3, 19, _x - 11, 27, TFT_RED);
        gfx.drawLine(_x + 3, 19, _x + 11, 27, TFT_RED);
        gfx.drawLine(_x + 11, 19, _x + 3, 27, TFT_RED);
    }
};

#endif // AVATAR_H
