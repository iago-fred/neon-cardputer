# 📇 Setup do SD Card

## Estrutura no cartão

```
SD CARD/
└── neon/
    ├── config.json          ← Configuração do widget (WiFi, servidor)
    └── sprites/             ← PNGs do avatar (opcional)
        ├── idle.png
        ├── happy.png
        ├── sad.png
        ├── surprised.png
        ├── thinking.png
        ├── listening.png
        ├── sleep.png
        └── error.png
```

## config.json

```json
{
  "wifi": [
    {"ssid": "MinhaRede", "password": "***"}
  ],
  "server_host": "IP_DA_VPS_AQUI",
  "server_port": 8080,
  "sound_enabled": true,
  "brightness": 100
}
```

## Sprites (PNG)

Os sprites do avatar são carregados do SD quando disponíveis.
Se não encontrar, usa o desenho programático (nativo).

**Formato recomendado:**
- Tamanho: 80×80 pixels (full) ou 55×55 (split)
- PNG com fundo transparente
- Nome: `{emoção}.png` (tudo minúsculo)

**Emoções:**

| Arquivo | Emoção |
|---|---|
| `idle.png` | Neutro, padrão |
| `happy.png` | Feliz |
| `sad.png` | Triste |
| `surprised.png` | Surpreso |
| `thinking.png` | Pensando |
| `listening.png` | Ouvindo áudio |
| `sleep.png` | Dormindo |
| `error.png` | Erro |

## Prioridade de leitura

1. ✅ SD card (`/neon/config.json`) — configuração
2. ✅ SPIFFS — fallback da config
3. ✅ Defaults — WiFi vazio, sem servidor
