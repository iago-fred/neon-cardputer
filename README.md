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

### Opção A — Arduino IDE (recomendado)

1. **Instalar o M5Stack Board Manager (não o Espressif):**
   - **Arquivo > Preferências**
   - Em "URLs adicionais para gerenciadores de placas", colar:
     ```
     https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
     ```

2. **Instalar placa:** **Ferramentas > Placa > Gerenciador de Placas**
   - Buscar `M5Stack` → instalar **"M5Stack"** (versão >= 3.2.2)
   - **Ferramentas > Placa > M5Stack > M5Cardputer**

3. **Instalar bibliotecas (Ctrl+Shift+I):**

   | Biblioteca | Buscar por |
   |---|---|
   | `M5CardPuter` by M5Stack | M5CardPuter |
   | `M5GFX` by M5Stack | M5GFX |
   | `ArduinoJson` by Benoit Blanchon | ArduinoJson |
   | `ESP8266Audio` by Earle F. Philhower | ESP8266Audio |

4. **Abrir e enviar:**
   - Abrir `firmware/neon-cardputer/neon-cardputer.ino`
   - Conectar M5CardPuter via USB-C
   - **Colocar em modo download:** switch OFF → segurar G0 → conectar USB → soltar G0
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
