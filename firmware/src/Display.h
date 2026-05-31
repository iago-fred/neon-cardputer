#ifndef DISPLAY_H
#define DISPLAY_H

#include <M5Cardputer.h>
#include "Config.h"

// ============================================================
// DisplayManager
// ============================================================
class DisplayManager {
private:
    ConfigManager* _config;
    bool _asleep = false;
    uint16_t _bgColor = TFT_BLACK;
    
public:
    DisplayManager(ConfigManager* config) : _config(config) {}
    
    void init() {
        M5Cardputer.Display.setRotation(1);
        M5Cardputer.Display.fillScreen(_bgColor);
        M5Cardputer.Display.setTextColor(TFT_WHITE, _bgColor);
    }
    
    void clear() {
        M5Cardputer.Display.fillScreen(_bgColor);
    }
    
    uint16_t getBgColor() { return _bgColor; }
    
    void showSplash() {
        auto& gfx = M5Cardputer.Display;
        gfx.fillScreen(TFT_BLACK);
        
        // Título
        gfx.setTextSize(2);
        gfx.setTextColor(TFT_CYAN, TFT_BLACK);
        gfx.setCursor(40, 40);
        gfx.println("Neon");
        
        gfx.setTextSize(1);
        gfx.setTextColor(TFT_WHITE, TFT_BLACK);
        gfx.setCursor(30, 65);
        gfx.println("Carregando...");
        
        // Barra de progresso
        gfx.drawRect(20, 85, 200, 10, TFT_DARKGREY);
        for (int i = 0; i < 30; i++) {
            gfx.fillRect(22 + i * 6, 87, 4, 6, TFT_CYAN);
            delay(30);
        }
    }
    
    void showStatus(const char* msg) {
        auto& gfx = M5Cardputer.Display;
        gfx.setTextSize(1);
        gfx.setTextColor(TFT_WHITE, _bgColor);
        gfx.setCursor(5, 110);
        gfx.fillRect(0, 105, 240, 30, _bgColor);
        gfx.println(msg);
    }
    
    void showText(const char* text) {
        auto& gfx = M5Cardputer.Display;
        // Limpa área de texto (metade inferior da tela)
        gfx.fillRect(0, 55, 240, 80, _bgColor);
        
        gfx.setTextSize(1);
        gfx.setTextColor(TFT_WHITE, _bgColor);
        gfx.setCursor(5, 60);
        
        // Quebra linha automaticamente
        String t = text;
        int lineWidth = 0;
        int lineY = 60;
        int maxWidth = 230;
        
        for (int i = 0; i < t.length(); i++) {
            char c = t.charAt(i);
            int cw = gfx.textWidth(String(c));
            
            if (lineWidth + cw > maxWidth || c == '\n') {
                lineY += 12;
                lineWidth = 0;
                if (c == '\n') continue;
            }
            
            if (lineY > 100) {
                gfx.print("...");
                break;
            }
            
            gfx.print(String(c));
            lineWidth += cw;
        }
    }
    
    void showFooter(const char* text) {
        auto& gfx = M5Cardputer.Display;
        gfx.setTextSize(1);
        gfx.setTextColor(TFT_DARKGREY, _bgColor);
        gfx.fillRect(0, 120, 240, 15, _bgColor);
        gfx.setCursor(5, 122);
        gfx.println(text);
    }
    
    void showRecording(bool active) {
        auto& gfx = M5Cardputer.Display;
        if (active) {
            gfx.fillRect(0, 100, 240, 20, _bgColor);
            gfx.setTextColor(TFT_RED, _bgColor);
            gfx.setCursor(5, 102);
            gfx.println("🔴 GRAVANDO...");
        } else {
            gfx.fillRect(0, 100, 240, 20, _bgColor);
        }
    }
    
    void showNotification(const char* text) {
        // Pop-up de notificação
        auto& gfx = M5Cardputer.Display;
        gfx.fillRect(10, 30, 220, 50, TFT_DARKGREY);
        gfx.drawRect(10, 30, 220, 50, TFT_CYAN);
        gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
        gfx.setCursor(20, 42);
        gfx.setTextSize(1);
        gfx.println(text);
        gfx.fillRect(20, 65, 200, 3, TFT_CYAN); // Timer visual
    }
    
    void sleep() {
        if (_asleep) return;
        _asleep = true;
        M5Cardputer.Display.sleep();
    }
    
    void wake() {
        if (!_asleep) return;
        _asleep = false;
        M5Cardputer.Display.wakeup();
        delay(100);
        M5Cardputer.Display.setRotation(1);
        clear();
    }
};

#endif // DISPLAY_H
