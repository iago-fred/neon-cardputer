#ifndef UI_H
#define UI_H

#include <M5Cardputer.h>
#include "Display.h"
#include "Config.h"

// ============================================================
// Telas
// ============================================================
enum Screen {
    SCREEN_IDLE,        // Avatar + frase
    SCREEN_MENU,        // Menu principal
    SCREEN_DIA,         // Agenda + clima
    SCREEN_CHAT,        // Bate-papo texto
    SCREEN_CONFIG,      // Configurações
    SCREEN_ABOUT,       // Sobre / créditos
    SCREEN_NOTIFICATION // Notificação pop-up
};

// ============================================================
// Itens do Menu
// ============================================================
struct MenuItem {
    const char* label;
    const char* icon;
    Screen target;
};

static const MenuItem MENU_ITEMS[] = {
    {"Dia",       "[*]", SCREEN_DIA},
    {"Chat",      "<~>", SCREEN_CHAT},
    {"Config",    "[*]", SCREEN_CONFIG},
    {"Sobre",     "[i]", SCREEN_ABOUT}
};
static const int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

// ============================================================
// UIManager
// ============================================================
class UIManager {
private:
    DisplayManager* _display;
    ConfigManager*  _config;
    Screen _currentScreen = SCREEN_IDLE;
    Screen _previousScreen = SCREEN_IDLE;
    int _menuSelection = 0;
    
    // Stack de navegação (histórico simples)
    Screen _screenStack[5];
    int _stackDepth = 0;
    
public:
    UIManager(DisplayManager* display, ConfigManager* config) 
        : _display(display), _config(config) {}
    
    Screen getScreen() { return _currentScreen; }
    
    void setScreen(Screen s) {
        _previousScreen = _currentScreen;
        
        if (_currentScreen != SCREEN_IDLE && s != SCREEN_IDLE) {
            // Empilha pra navegação
            if (_stackDepth < 5) {
                _screenStack[_stackDepth++] = _currentScreen;
            }
        }
        
        _currentScreen = s;
        _menuSelection = 0;
        _display->clear();
        render();
    }
    
    void goBack() {
        if (_stackDepth > 0) {
            _currentScreen = _screenStack[--_stackDepth];
            _display->clear();
            render();
        } else if (_currentScreen != SCREEN_IDLE) {
            setScreen(SCREEN_IDLE);
        }
    }
    
    void cycleMenu() {
        if (_currentScreen == SCREEN_MENU) {
            _menuSelection = (_menuSelection + 1) % MENU_COUNT;
            _display->clear();
            renderMenu();
        }
    }
    
    void cycleMenuPrev() {
        if (_currentScreen == SCREEN_MENU) {
            _menuSelection = (_menuSelection - 1 + MENU_COUNT) % MENU_COUNT;
            _display->clear();
            renderMenu();
        }
    }
    
    void selectCurrent() {
        if (_currentScreen == SCREEN_MENU && _menuSelection < MENU_COUNT) {
            setScreen(MENU_ITEMS[_menuSelection].target);
        }
    }
    
    void handleKey(uint8_t key) {
        switch (_currentScreen) {
            case SCREEN_MENU:
                handleMenuKey(key);
                break;
            case SCREEN_DIA:
            case SCREEN_CHAT:
            case SCREEN_CONFIG:
            case SCREEN_ABOUT:
                // ESC já tratado no main, volta ao menu
                break;
            default:
                break;
        }
    }
    
    void update() {
        if (_currentScreen == SCREEN_IDLE) {
            // Mostra frase aleatória de tempos em tempos
            static uint32_t lastPhrase = 0;
            if (millis() - lastPhrase > 15000) {
                showIdlePhrase();
                lastPhrase = millis();
            }
        }
    }
    
    void render() {
        switch (_currentScreen) {
            case SCREEN_IDLE:     renderIdle();     break;
            case SCREEN_MENU:     renderMenu();     break;
            case SCREEN_DIA:      renderDia();      break;
            case SCREEN_CHAT:     renderChat();     break;
            case SCREEN_CONFIG:   renderConfig();   break;
            case SCREEN_ABOUT:    renderAbout();    break;
            default:              renderIdle();     break;
        }
    }
    
private:
    void handleMenuKey(uint8_t key) {
        // Navegação simplificada: o loop principal gerencia
        // O handleKey é mantido para compatibilidade futura
    }
    
    void renderIdle() {
        // Avatar é renderizado pelo Avatar::update() separadamente
    }
    
    void showIdlePhrase() {
        const char* phrases[] = {
            "Só passando pra dar um oi...",
            "Boooo! (sou um fantasma, lembra?)",
            "Bora um cafe?",
            "Pop punk nunca eh demais",
            "Tudo calmo por aqui...",
            "A patroa mandou lembrancas"
        };
        int idx = random(0, sizeof(phrases)/sizeof(phrases[0]));
        _display->showFooter(phrases[idx]);
    }
    
    void renderMenu() {
        auto& gfx = M5Cardputer.Display;
        gfx.setTextSize(1);
        
        int y = 5;
        for (int i = 0; i < MENU_COUNT; i++) {
            if (i == _menuSelection) {
                gfx.fillRect(0, y, 240, 25, TFT_DARKGREY);
                gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
            } else {
                gfx.setTextColor(TFT_LIGHTGREY, _display->getBgColor());
            }
            
            gfx.setCursor(10, y + 5);
            gfx.printf("%s  %s", MENU_ITEMS[i].icon, MENU_ITEMS[i].label);
            y += 30;
        }
        
        // Instruções no rodapé
        _display->showFooter("tecla = seleciona  longa = volta");
    }
    
    void renderDia() {
        auto& gfx = M5Cardputer.Display;
        gfx.setTextSize(1);
        gfx.setTextColor(TFT_CYAN, _display->getBgColor());
        gfx.setCursor(5, 5);
        gfx.println("== HOJE ==");
        
        gfx.setTextColor(TFT_WHITE, _display->getBgColor());
        gfx.setCursor(5, 20);
        gfx.println("Carregando...");
        
        // Aqui entraria dados do Calendar + clima via polling
        _display->showFooter("ESC volta");
    }
    
    void renderChat() {
        auto& gfx = M5Cardputer.Display;
        gfx.setTextSize(1);
        gfx.setTextColor(TFT_CYAN, _display->getBgColor());
        gfx.setCursor(5, 5);
        gfx.println("== CHAT ==");
        
        gfx.setTextColor(TFT_WHITE, _display->getBgColor());
        gfx.setCursor(5, 20);
        gfx.println("Pressione o botao");
        gfx.setCursor(5, 32);
        gfx.println("para falar comigo!");
        
        _display->showFooter("Botao A = gravar  ESC volta");
    }
    
    void renderConfig() {
        auto& gfx = M5Cardputer.Display;
        gfx.setTextSize(1);
        gfx.setTextColor(TFT_CYAN, _display->getBgColor());
        gfx.setCursor(5, 5);
        gfx.println("== CONFIG ==");
        
        gfx.setTextColor(TFT_WHITE, _display->getBgColor());
        gfx.setCursor(5, 22);
        gfx.printf("Som: %s\n", _config->isSoundEnabled() ? "ON" : "OFF");
        gfx.setCursor(5, 34);
        gfx.printf("Brilho: %d%%\n", _config->getBrightness());
        gfx.setCursor(5, 46);
        gfx.printf("WiFi: %s\n", WiFi.isConnected() ? WiFi.SSID().c_str() : "desconectado");
        gfx.setCursor(5, 58);
        gfx.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        
        _display->showFooter("ESC volta");
    }
    
    void renderAbout() {
        auto& gfx = M5Cardputer.Display;
        gfx.setTextSize(1);
        gfx.setTextColor(TFT_CYAN, _display->getBgColor());
        gfx.setCursor(5, 5);
        gfx.println("== SOBRE ==");
        
        gfx.setTextColor(TFT_WHITE, _display->getBgColor());
        gfx.setCursor(5, 20);
        gfx.println("Neon Widget v0.1");
        gfx.setCursor(5, 32);
        gfx.println("Feito com <3 por Iago");
        gfx.setCursor(5, 44);
        gfx.println("& Neon");
        gfx.setCursor(5, 56);
        gfx.println("✨ M5CardPuter + OpenClaw");
        
        _display->showFooter("ESC volta");
    }
};

#endif // UI_H
