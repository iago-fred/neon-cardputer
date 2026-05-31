#ifndef AVATAR_H
#define AVATAR_H

#include <M5Cardputer.h>
#include "Display.h"
#include "SpriteManager.h"

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
    "idle", "happy", "sad", "surprised",
    "thinking", "listening", "sleep", "error"
};

// ============================================================
// Modos de layout
// ============================================================
enum AvatarLayout {
    AVATAR_LAYOUT_FULL,   // Centro da tela, tamanho grande
    AVATAR_LAYOUT_SPLIT   // Metade esquerda, tamanho reduzido
};

// ============================================================
// Avatar — Desenha a Neon na tela (com suporte a sprites PNG)
// ============================================================
class Avatar {
private:
    DisplayManager* _display;
    SpriteManager*  _sprites;
    AvatarEmotion _currentEmotion = AVATAR_IDLE;
    AvatarLayout  _layout = AVATAR_LAYOUT_FULL;
    bool _eyesOpen = true;
    float _breathePhase = 0.0f;

    // Posição e tamanho (calculados dinamicamente)
    int _cx = 70;   // centro x
    int _cy = 40;   // centro y
    float _scale = 1.0f;

    // Timing
    uint32_t _emotionStart = 0;
    uint32_t _emotionDuration = 8000;

public:
    Avatar(DisplayManager* display, SpriteManager* sprites)
        : _display(display), _sprites(sprites) {}

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

    void setLayout(AvatarLayout layout) {
        _layout = layout;
        if (layout == AVATAR_LAYOUT_FULL) {
            _cx = 70;   // centro da tela
            _cy = 45;
            _scale = 1.0f;
        } else {
            _cx = 40;   // metade esquerda
            _cy = 40;
            _scale = 0.65f;
        }
    }

    AvatarLayout getLayout() { return _layout; }

    void update() {
        if (_currentEmotion != AVATAR_IDLE &&
            millis() - _emotionStart > _emotionDuration) {
            _currentEmotion = AVATAR_IDLE;
        }

        uint32_t blinkCycle = millis() % 4150;
        _eyesOpen = blinkCycle < 4000;

        if (_currentEmotion == AVATAR_IDLE || _currentEmotion == AVATAR_SLEEP) {
            _breathePhase += 0.02f;
        }

        draw();
    }

    void draw() {
        // Tenta sprite PNG primeiro
        const char* emoName = EMOTION_NAMES[_currentEmotion];
        int spriteSize = (_layout == AVATAR_LAYOUT_FULL) ? 80 : 55;
        if (_sprites->draw(emoName, _cx - spriteSize/2, _cy - spriteSize/2,
                           spriteSize, spriteSize)) {
            return;  // Sprite desenhado com sucesso
        }

        // Fallback: desenho programático
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
    int sx(int v) { return _cx + (int)((v - 70) * _scale); }
    int sy(int v) { return _cy + (int)((v - 40) * _scale); }
    int sr(int v) { return (int)(v * _scale); }

    void drawBaseGhost(int eyeY = 22, bool openEyes = true) {
        auto& gfx = M5Cardputer.Display;
        int cx = _cx, cy = _cy;
        float s = _scale;

        int r = sr(20);
        gfx.fillCircle(cx, cy + sr(15), r, TFT_CYAN);
        gfx.fillRect(cx - r, cy + sr(15), r * 2, sr(18), TFT_CYAN);

        int triY = cy + sr(33);
        int triB = cy + sr(42);
        gfx.fillTriangle(cx - sr(10), triY, cx, triB, cx + sr(10), triY, TFT_CYAN);
        gfx.fillTriangle(cx - sr(15), triY, cx - sr(5), triB, cx + sr(5), triY, TFT_CYAN);
        gfx.fillTriangle(cx + sr(5), triY, cx + sr(15), triB, cx + sr(20), triY, TFT_CYAN);

        int eY = cy + sr(eyeY - 22);
        if (openEyes) {
            int er = sr(3);
            gfx.fillCircle(cx - sr(7), eY, max(er, 1), TFT_WHITE);
            gfx.fillCircle(cx + sr(7), eY, max(er, 1), TFT_WHITE);
            gfx.fillCircle(cx - sr(7), eY, max(sr(1), 1), TFT_BLACK);
            gfx.fillCircle(cx + sr(7), eY, max(sr(1), 1), TFT_BLACK);
        } else {
            gfx.drawLine(cx - sr(10), eY, cx - sr(4), eY, TFT_WHITE);
            gfx.drawLine(cx + sr(4), eY, cx + sr(10), eY, TFT_WHITE);
        }

        // Touca proporcional
        int tw = sr(28), th = sr(8);
        gfx.fillRect(cx - tw/2, cy - sr(5), tw, th, TFT_BLACK);
        gfx.fillCircle(cx, cy - sr(1), sr(15), TFT_BLACK);
        gfx.fillRect(cx - sr(15), cy - sr(5), sr(30), sr(3), TFT_DARKGREY);
    }

    void drawIdle() {
        auto& gfx = M5Cardputer.Display;
        int r = (_layout == AVATAR_LAYOUT_FULL) ? 60 : 50;
        gfx.fillRect(_cx - r, _cy - r, r * 2, r * 2, _display->getBgColor());

        uint8_t glow = 30 + (sin(_breathePhase) * 20);
        gfx.drawCircle(_cx, _cy + sr(15), sr(22), gfx.color565(0, glow, glow));
        drawBaseGhost(22, _eyesOpen);
    }

    void drawHappy() {
        auto& gfx = M5Cardputer.Display;
        int r = (_layout == AVATAR_LAYOUT_FULL) ? 60 : 50;
        gfx.fillRect(_cx - r, _cy - r, r * 2, r * 2, _display->getBgColor());
        gfx.drawCircle(_cx, _cy + sr(15), sr(25), TFT_YELLOW);
        drawBaseGhost(20, true);
        gfx.drawCircle(_cx, _cy + sr(15), sr(5), TFT_WHITE);
        gfx.fillCircle(_cx, _cy + sr(16), sr(4), TFT_CYAN);
    }

    void drawSad() {
        auto& gfx = M5Cardputer.Display;
        int r = (_layout == AVATAR_LAYOUT_FULL) ? 60 : 50;
        gfx.fillRect(_cx - r, _cy - r, r * 2, r * 2, _display->getBgColor());
        drawBaseGhost(24, true);
        int my = _cy + sr(18);
        gfx.drawLine(_cx - sr(4), my, _cx + sr(4), my, TFT_WHITE);
        gfx.drawLine(_cx - sr(4), my, _cx - sr(2), my + sr(2), TFT_WHITE);
        gfx.drawLine(_cx + sr(4), my, _cx + sr(2), my + sr(2), TFT_WHITE);
    }

    void drawSurprised() {
        auto& gfx = M5Cardputer.Display;
        int r = (_layout == AVATAR_LAYOUT_FULL) ? 60 : 50;
        gfx.fillRect(_cx - r, _cy - r, r * 2, r * 2, _display->getBgColor());
        drawBaseGhost(18, true);
        int er = sr(5);
        gfx.fillCircle(_cx - sr(7), _cy + sr(2), max(er, 2), TFT_WHITE);
        gfx.fillCircle(_cx + sr(7), _cy + sr(2), max(er, 2), TFT_WHITE);
        gfx.fillCircle(_cx - sr(7), _cy + sr(2), max(sr(2), 1), TFT_BLACK);
        gfx.fillCircle(_cx + sr(7), _cy + sr(2), max(sr(2), 1), TFT_BLACK);
        gfx.drawCircle(_cx, _cy + sr(18), sr(4), TFT_WHITE);
    }

    void drawThinking() {
        auto& gfx = M5Cardputer.Display;
        int r = (_layout == AVATAR_LAYOUT_FULL) ? 60 : 50;
        gfx.fillRect(_cx - r, _cy - r, r * 2, r * 2, _display->getBgColor());
        drawBaseGhost(22, true);
        int bx = _cx + sr(22), by = _cy - sr(10);
        gfx.drawCircle(bx, by, sr(3), TFT_WHITE);
        gfx.drawCircle(bx + sr(6), by - sr(6), sr(5), TFT_WHITE);
        gfx.drawCircle(bx + sr(13), by - sr(12), sr(10), TFT_WHITE);
        gfx.fillCircle(bx + sr(13), by - sr(12), sr(9), _display->getBgColor());
    }

    void drawListening() {
        auto& gfx = M5Cardputer.Display;
        int r = (_layout == AVATAR_LAYOUT_FULL) ? 60 : 50;
        gfx.fillRect(_cx - r, _cy - r, r * 2, r * 2, _display->getBgColor());
        drawBaseGhost(22, true);
        int mx = _cx + sr(25), my = _cy + sr(10);
        gfx.drawArc(mx, my, sr(5), sr(3), -45, 45, TFT_GREEN);
        gfx.drawArc(mx, my, sr(9), sr(7), -45, 45, TFT_GREEN);
        gfx.drawArc(mx, my, sr(13), sr(11), -45, 45, TFT_GREEN);
    }

    void drawSleep() {
        auto& gfx = M5Cardputer.Display;
        int r = (_layout == AVATAR_LAYOUT_FULL) ? 60 : 50;
        gfx.fillRect(_cx - r, _cy - r, r * 2, r * 2, _display->getBgColor());
        uint8_t dim = 10 + (sin(_breathePhase * 0.5f) * 5);
        gfx.drawCircle(_cx, _cy + sr(15), sr(22), gfx.color565(0, dim, dim));
        drawBaseGhost(22, false);
        int zx = _cx + sr(22), zy = _cy - sr(8);
        gfx.drawChar('Z', zx, zy, 1);
        gfx.drawChar('z', zx + sr(6), zy + sr(6), 1);
        gfx.drawChar('z', zx + sr(12), zy + sr(12), 1);
    }

    void drawError() {
        auto& gfx = M5Cardputer.Display;
        int r = (_layout == AVATAR_LAYOUT_FULL) ? 60 : 50;
        gfx.fillRect(_cx - r, _cy - r, r * 2, r * 2, _display->getBgColor());
        gfx.drawCircle(_cx, _cy + sr(15), sr(23), TFT_RED);
        gfx.drawCircle(_cx, _cy + sr(15), sr(24), TFT_RED);
        int ex1 = _cy + sr(19), ex2 = _cy + sr(27);
        gfx.drawLine(_cx - sr(11), ex1, _cx - sr(3), ex2, TFT_RED);
        gfx.drawLine(_cx - sr(3), ex1, _cx - sr(11), ex2, TFT_RED);
        gfx.drawLine(_cx + sr(3), ex1, _cx + sr(11), ex2, TFT_RED);
        gfx.drawLine(_cx + sr(11), ex1, _cx + sr(3), ex2, TFT_RED);
    }
};

#endif // AVATAR_H
