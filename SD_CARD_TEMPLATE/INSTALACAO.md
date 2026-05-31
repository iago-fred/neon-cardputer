# 📇 Setup do SD Card

## Estrutura no cartão

```
SD CARD/
└── neon/
    ├── config.json    ← Configurações (WiFi, servidor)
    ├── sprites/       ← (futuro) PNGs do avatar
    └── sounds/        ← (futuro) WAVs de feedback
```

## config.json

```json
{
  "wifi": [
    {"ssid": "MinhaRede", "password": "***"}
  ],
  "server_host": "IP_DA_VPS_AQUI",
  "server_port": 8080,
  "audio_endpoint": "/api/neon/audio",
  "poll_endpoint": "/api/neon/poll",
  "sound_enabled": true,
  "brightness": 100
}
```

- **server_host**: IP público da VPS onde o bridge roda
- **server_port**: 8080 (porta do bridge)
- Pode ter múltiplas redes WiFi — ele tenta cada uma até conectar
