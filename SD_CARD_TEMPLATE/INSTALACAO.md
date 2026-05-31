# 📇 Setup do SD Card

## Estrutura no cartão

```
SD CARD/
└── neon/
    └── config.json    ← OBRIGATÓRIO: configuração do widget
```

## Criando o config.json

Coloca num cartão microSD, formata como FAT32, e cria:

```
neon/
└── config.json
```

Conteúdo do `config.json`:

```json
{
  "wifi": [
    {"ssid": "MinhaRede", "password": "***"}
  ],
  "server_host": "187.127.243.164",
  "server_port": 8080,
  "sound_enabled": true,
  "brightness": 100
}
```

### Campos

| Campo | Obrigatório | Descrição |
|---|---|---|
| `wifi` | ✅ | Lista de redes WiFi (tenta cada uma) |
| `server_host` | ✅ | IP da VPS onde o bridge roda |
| `server_port` | ✅ | Porta do bridge (8080) |
| `audio_endpoint` | ❌ | Rota p/ enviar áudio (default: /api/neon/audio) |
| `poll_endpoint` | ❌ | Rota p/ notificações (default: /api/neon/poll) |
| `sound_enabled` | ❌ | Som ligado (default: true) |
| `brightness` | ❌ | Brilho da tela 0-100 (default: 100) |

### Prioridade de leitura

1. ✅ SD card (`/neon/config.json`) — se não achar, vai pro...
2. ✅ SPIFFS (`/config.json`) — se não achar, usa...
3. ✅ Defaults (WiFi vazio, sem servidor)

> ⚠️ **Nunca comitar o IP real da VPS no GitHub!**
> Use `IP_DA_VPS_AQUI` no template público, coloque o IP real só no SD físico.
