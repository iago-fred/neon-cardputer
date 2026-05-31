# Neon Widget — M5CardPuter 🖥️👻

## Visão Geral

Widget físico com M5CardPuter que se conecta à VPS para interagir com a Neon (assistente AI). Display colorido + teclado + microfone + speaker.

## Hardware

- **M5CardPuter** (M5Stack)
  - ESP32-S3 (2x Xtensa LX7 @240MHz)
  - 16MB Flash, 8MB PSRAM
  - Tela 1.14" ST7789V (240×135, colorida)
  - Teclado QWERTY integrado
  - Microfone I2S (SPM1423)
  - Speaker I2S (NS4168)
  - Bateria 500mAh
  - WiFi + Bluetooth

## Funcionalidades

| Funcionalidade | Status |
|---|---|
| Avatar Neon com emoções | 🎯 Planejado |
| Menu: Dia (agenda+clima) | 🎯 Planejado |
| Menu: Chat (áudio/texto) | 🎯 Planejado |
| Menu: Config (WiFi, som, brilho) | 🎯 Planejado |
| Push-to-talk (botão ação) | 🎯 Planejado |
| Sleep automático | 🎯 Planejado |
| Feedback sonoro (voz Neon) | 🎯 Planejado |
| WiFi auto-connect (lista) | 🎯 Planejado |

## Arquitetura de Comunicação

```
┌──────────────────────┐          ┌──────────────────────────┐
│    M5CardPuter        │          │    VPS (OpenClaw)        │
│                       │  HTTP/   │                          │
│  [Microfone] ───WAV──►│  WS     │  ┌─ WebSocket Server ──┐  │
│                       │◄──JSON──│  │  neon-ws-bridge     │  │
│  [Display/Tela] ◄─────│         │  └─────────────────────┘  │
│                       │         │         │                 │
│  [Teclado] ────KEY───►│  /api/  │  ┌─ Whisper (TTS) ────┐  │
│                       │◄──AUDIO─│  │  transcribe_local   │  │
│  [Speaker] ◄──────────│         │  │  speak.sh (TTS)     │  │
│                       │         │  └─────────────────────┘  │
│  [WiFi Manager]       │         │         │                 │
│                       │         │  ┌─ Google Calendar ────┐ │
└──────────────────────┘          │  │  Clima / News        │ │
                                   │  ~~~~~~~~~~~~~~~~~~~~~  │
                                   └──────────────────────────┘
```

### Endpoints da VPS (propostos)

Para começo, podemos usar o HTTP simples. Depois evolui pra WebSocket.

```
POST /api/neon/audio        ← CardPuter envia áudio gravado (WAV)
GET  /api/neon/ping          ← CardPuter verifica conexão
POST /api/neon/tts           ← CardPuter solicita TTS (retorna áudio)
GET  /api/neon/messages      ← CardPuter busca últimas mensagens
GET  /api/neon/avatar/:emotion ← PNG do avatar com emoção X
```

### Fluxo push-to-talk

```
1. Usuário aperta botão ação
2. CardPuter: "gravando..." (ícone na tela)
3. Usuário fala
4. Usuário solta botão
5. CardPuter enconde WAV (16kHz, 16bit, mono)
6. HTTP POST /api/neon/audio
7. VPS: Whisper transcreve
8. Neon processa comando
9. VPS retorna JSON com:
   - texto resposta
   - emoção do avatar
   - URL do áudio TTS (opcional)
10. CardPuter renderiza:
    - Avatar com emoção
    - Texto resposta
    - Se tiver áudio, baixa e toca
```

## Sistema de Telas

### Navegação

```
[Boot/Splash]
    │
    ▼
[Modo Idle — Avatar]  ◄──────────────────┐
    │                                      │
    │ (qualquer tecla)                     │
    ▼                                      │
[Menu Principal]                           │
    │                                      │
    ├── 📅 Dia ───── [Agenda + Clima] ─────┤
    │                                      │
    ├── 💬 Chat ──── [Bate-papo] ─────────┤
    │                                      │
    ├── ⚙️ Config ── [WiFi, Som, Brilho] ──┤
    │                                      │
    └── ❤️ Sobre ─── [Info / Créditos] ────┘
                    (ESC volta)
```

### Avatar + Emoções

Banco de emoções (frames PNG ou desenhados programaticamente):

| Emoção | Quando usar |
|---|---|
| `idle` | Tela parada, respiração leve |
| `thinking` | Processando áudio/comando |
| `happy` | Resposta positiva |
| `sad` | Algo deu errado / notícia triste |
| `surprised` | Notificação inesperada |
| `sleep` | Modo economia (antes de desligar tela) |
| `listening` | Gravando áudio |
| `error` | Conexão/WiFi perdido |

## Sketches do Firmware

### main.cpp (esboço)

```cpp
#include <M5Cardputer.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Audio/AudioManager.h"
#include "Display/DisplayManager.h"
#include "Network/NetworkManager.h"
#include "UI/UIManager.h"

// Config
typedef struct {
    char ssid[32];
    char password[64];
} wifi_cred_t;

std::vector<wifi_cred_t> wifiList; // Carregado da SD ou SPIFFS
bool soundEnabled = true;
uint8_t brightness = 100;

// Managers
AudioManager audioManager;
DisplayManager displayManager;
NetworkManager networkManager;
UIManager uiManager;

void setup() {
    M5Cardputer.begin();
    loadConfig();       // Carrega WiFi list, settings
    networkManager.connect(wifiList);
    displayManager.showSplash();
    uiManager.setScreen(SCREEN_IDLE); // Avatar inicial
}

void loop() {
    M5Cardputer.update();
    
    // Check input
    if (M5Cardputer.Keyboard.isChange()) {
        auto key = M5Cardputer.Keyboard.key();
        if (key.state == KEY_PRESSED) {
            handleKey(key.key_code);
        }
    }
    
    // Check button action (push-to-talk)
    if (M5Cardputer.BtnA.wasPressed()) {
        startRecording();
    }
    if (M5Cardputer.BtnA.wasReleased()) {
        stopRecordingAndSend();
    }
    
    // Network check & update UI
    networkManager.update();
    
    // Sleep timer
    static uint32_t lastActivity = millis();
    if (millis() - lastActivity > SLEEP_TIMEOUT_MS) {
        displayManager.sleep();
    }
}

void handleKey(uint8_t key) {
    resetSleepTimer();
    
    if (key == KEY_ESC) {
        uiManager.goBack();
        return;
    }
    
    uiManager.handleInput(key);
}
```

## API Bridge (VPS-side)

Precisamos de uma bridge leve que o CardPuter converse. Pode ser:

1. **Script Python** rodando como serviço na VPS — recebe POST /audio, chama Whisper, devolve resposta
2. **WebSocket server** Node.js que integra com OpenClaw
3. **Rota no backend existente** (IL-Separação)

Opção 1 é a mais simples e direta pro MVP.

### JSON de Resposta (formato)

```json
{
    "type": "chat_response",
    "text": "Bom dia! 🌤️ Hoje em Salvador: 25-32°C, sol com pancadas à tarde.",
    "emotion": "happy",
    "tts_url": "/tmp/neon_tts_123456.wav",
    "menu_action": null,
    "actions": [
        {"label": "Ver agenda", "action": "menu:dia"},
        {"label": "Silenciar som", "action": "config:toggle_sound"}
    ]
}
```

## Timeline MVP

1. ✅ **Boot + WiFi + Config** — conectar na rede automaticamente
2. ⏳ **Avatar estático + Menu navegável** — teclado funcional
3. ⏳ **Push-to-talk + áudio → VPS → Whisper → resposta na tela**
4. ⏳ **Emoções do avatar** baseadas no response
5. ⏳ **TTS de volta** — voz da Neon no speaker
6. ⏳ **Modo sleep + wake**
