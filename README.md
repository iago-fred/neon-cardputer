# Neon Widget 👻📟

Widget físico com **M5CardPuter** que se conecta ao WiFi e conversa com a **Neon** no Telegram.

## Arquitetura

```
Cardputer ──HTTP──▶ Neon Bridge (VPS) ──▶ Telegram (visual)
                    │                      ├── Neon responde 🧠
                    └──▶ OpenClaw ──▶ Telefone do Iago 📱
```

**Como funciona:**
1. Você digita no Cardputer
2. Mensagem vai pra **Neon Bridge** via HTTP (serveo tunnel)
3. Bridge envia pro Telegram (você vê no celular) + injeta na sessão da Neon
4. **Neon lê a mensagem como se fosse sua** e responde igual
5. Resposta aparece no Telegram → Cardputer pega via `getUpdates` e mostra na tela

## Hardware

- **M5CardPuter** (ESP32-S3, 8MB Flash, PSRAM)
- Tela 240x135 colorida
- Teclado QWERTY

## Estrutura

```
neon-cardputer/
├── firmware/
│   ├── platformio.ini
│   └── neon-cardputer/
│       ├── neon-cardputer.ino   # Main
│       ├── config.h             # SD + config JSON
│       ├── neon_wifi.h          # WiFi scan inteligente
│       └── telegram.h           # Chat com bridge + Telegram
├── tools/
│   └── neon_bridge.py           # Bridge HTTP
├── tunnel/
│   ├── keep_alive.sh            # Script pra manter tunnel serveo
│   └── current_url.txt          # URL pública atual do tunnel
└── README.md
```

## Config (SD Card)

Criar `neon/config.json` no SD card:

```json
{
  "wifi": [
    {"ssid": "MinhaRede", "password": "***"}
  ],
  "bridge_url": "https://seu-subdomain.serveousercontent.com",
  "token": "SEU_BOT_TOKEN",
  "chat_id": "SEU_CHAT_ID"
}
```

| Campo | Obrigatório | Descrição |
|---|---|---|
| `wifi` | ✅ | Lista de redes WiFi |
| `bridge_url` | ✅ | URL da Neon Bridge (pra eu ver suas mensagens) |
| `token` | ✅ (fallback) | Token do bot Telegram (usado pra receber respostas) |
| `chat_id` | ✅ | Seu chat ID no Telegram |

## Como usar

### Arduino IDE

1. **Arquivo > Preferencias** → URL:
   ```
   https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
   ```
2. **Ferramentas > Placa > Gerenciador de Placas** → instalar `M5Stack` (>= 3.2.2)
3. **Ferramentas > Placa > M5Stack > M5Cardputer**
4. Abrir `firmware/neon-cardputer/neon-cardputer.ino`
5. Conectar USB-C e **Upload**

### Config
Criar `/neon/config.json` no cartão SD com os dados acima.

### Bridge (servidor)
A Neon Bridge roda automaticamente no servidor. O tunnel serveo é mantido por cron job. Se a URL mudar, a Neon avisa.

## Neon Bridge API

| Método | Rota | Descrição |
|---|---|---|
| `GET` | `/api/neon/ping` | Health check |
| `POST` | `/api/neon/message` | Enviar mensagem `{"text": "..."}` |
| `POST` | `/api/neon/send` | Enviar pro Telegram `{"text": "..."}` |
