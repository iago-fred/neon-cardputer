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
├── firmware/
│   ├── platformio.ini
│   └── neon-cardputer/     ← Pasta compatível com Arduino IDE
│       ├── neon-cardputer.ino
│       ├── Config.h
│       ├── Display.h
│       ├── Avatar.h
│       ├── UI.h
│       ├── AudioManager.h
│       ├── Network.h
│       ├── Version.h
│       └── OTAUpdate.h
├── tools/
│   └── neon_bridge.py      # Bridge HTTP na VPS
├── SD_CARD_TEMPLATE/       # Template para configuração via SD
└── architecture/
    └── README.md           # Documentação completa
```

## Como funciona

```
Usuário aperta Botão A
  → CardPuter grava áudio (I2S, WAV)
  → POST /api/neon/audio (HTTP)
  → Bridge na VPS transcreve com Whisper
  → Neon processa comando
  → Retorna JSON: {text, emotion, tts_url}
  → CardPuter renderiza avatar + texto
  → Se tiver TTS, baixa e toca no speaker
```

## Dev Setup

### Opção A — Arduino IDE (recomendado pra Windows)

1. **Instalar suporte ESP32:**
   - **Arquivo > Preferências**
   - Em "URLs adicionais para gerenciadores de placas", colar:
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```

2. **Instalar placa:** **Ferramentas > Placa > Gerenciador de Placas**
   - Buscar `ESP32` → instalar **"ESP32 by Espressif Systems"**

3. **Instalar bibliotecas (Ctrl+Shift+I):**

   | Biblioteca | Buscar por |
   |---|---|
   | `M5CardPuter` by M5Stack | M5CardPuter |
   | `M5GFX` by M5Stack | M5GFX |
   | `ArduinoJson` by Benoit Blanchon | ArduinoJson |
   | `WiFiManager` by tzapu | WiFiManager |
   | `ESP8266Audio` by Earle F. Philhower | ESP8266Audio |

4. **Configurar a placa:**
   - **Ferramentas > Placa > ESP32 Arduino → ESP32S3 Dev Module**
   - USB CDC On Boot: **Enabled**
   - Flash Mode: **QIO**
   - Flash Size: **16MB (128Mb)**
   - Partition Scheme: **16MB Flash (3MB APP/9.9MB FATFS)**
   - PSRAM: **OPI PSRAM**
   - Upload Speed: **921600**

5. **Abrir e enviar:**
   - Abrir `firmware/neon-cardputer/neon-cardputer.ino`
   - Conectar M5CardPuter via USB-C
   - Selecionar porta em **Ferramentas > Porta**
   - Clicar **➡️ Upload**

### Opção B — PlatformIO (VS Code / CLI)

```bash
cd firmware

# Se o board 'm5stack-cardputer' não for reconhecido:
#   pio platform update espressif32
# Ou use o fallback:
#   pio run -e m5stack-cardputer-legacy

pio run --target upload
```

### Bridge na VPS

```bash
python3 tools/neon_bridge.py --port 8080
```

(O bridge já está rodando na VPS, porta 8080 — só precisa configurar o IP no config.json do SD card.)

## Próximos passos

- [ ] Endpoint de agenda + clima na bridge
- [ ] Notificações push (WebSocket ou polling)
- [ ] OTA update do firmware
- [ ] Efeitos de transição entre telas
- [ ] Salvamento de conversas no histórico

---

Feito com 💙👻 por Iago & Neon
