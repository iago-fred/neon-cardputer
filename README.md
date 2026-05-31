# Neon Widget 👻📟

Widget físico com **M5CardPuter** para interagir com a assistente Neon (OpenClaw).

## Hardware

- **M5CardPuter** (ESP32-S3, 16MB Flash, PSRAM)
- Tela 240×135 colorida
- Teclado QWERTY
- Microfone I2S
- Speaker I2S
- Bateria 500mAh

## Funcionalidades

| Funcionalidade | Status |
|---|---|
| ✅ Avatar Neon com emoções (idle, happy, sad, surprised, thinking, listening, sleep, error) | 👻 Desenhado via M5GFX |
| ✅ Menu com navegação: Dia, Chat, Config, Sobre | ⌨️ Teclado |
| ✅ Push-to-talk: grava áudio → VPS → Whisper → resposta na tela | 🎤 Botão A |
| ✅ TTS de volta: voz da Neon no speaker | 🔊 Edge-TTS |
| ✅ Sleep automático (30s idle → display off → deep sleep 5min) | 💤 Economia |
| ✅ WiFi auto-connect com lista de redes | 📶 Config |
| ⏳ Agenda + Clima no display | 📅 Pendente |
| ⏳ Notificações push (VPS → dispositivo) | 📬 Pendente |

## Estrutura do Projeto

```
neon-cardputer/
├── firmware/          # Código do M5CardPuter (PlatformIO)
│   ├── platformio.ini
│   └── src/
│       ├── main.cpp        # Loop principal
│       ├── Config.h        # ConfigManager (WiFi, server, settings)
│       ├── Display.h       # DisplayManager (tela)
│       ├── Avatar.h        # Avatar Neon (emoções desenhadas)
│       ├── UI.h            # UIManager (telas, menu, navegação)
│       ├── AudioManager.h  # I2S mic + speaker
│       └── Network.h       # HTTP/WS client
├── tools/
│   └── neon_bridge.py      # Bridge HTTP na VPS
├── assets/                 # Placeholder para assets
└── architecture/
    └── README.md           # Documentação completa
```

## Como funciona

```
Usuário aperta Botão A
  → CardPuter grava áudio (I2S, WAV)
  → POST /api/neon/audio (HTTP/HTTPS)
  → Bridge na VPS transcreve com Whisper
  → Neon processa comando
  → Retorna JSON: {text, emotion, tts_url}
  → CardPuter renderiza avatar + texto
  → Se tiver TTS, baixa e toca no speaker
```

## Dev Setup

1. Instalar **PlatformIO** (VS Code extension ou CLI)
2. Conectar M5CardPuter via USB
3. Compilar e upload:
   ```bash
   cd firmware
   pio run --target upload
   ```
4. Bridge na VPS:
   ```bash
   python3 tools/neon_bridge.py --port 8080
   ```

## Próximos passos

- [ ] Endpoint de agenda + clima na bridge
- [ ] Notificações push (WebSocket ou polling)
- [ ] OTA update do firmware
- [ ] Efeitos de transição entre telas
- [ ] Salvamento de conversas no histórico

---

Feito com 💙👻 por Iago & Neon
