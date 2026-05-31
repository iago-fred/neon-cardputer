# 🧊 Neon Bridge — Uso Interno (OpenClaw)

O bridge roda em background na VPS, porta 8080.  
Usado pelo M5CardPuter para enviar áudio e receber respostas.

## Como usar daqui do OpenClaw

### Enviar notificação push pro CardPuter

```bash
curl -s -X POST http://localhost:8080/api/neon/push \
  -H "Content-Type: application/json" \
  -d '{"text": "Iago, lembrando do Botox 100UI!", "emotion": "surprised", "tts": "Lembrando do Botox 100UI"}'
```

### Verificar status

```bash
curl -s http://localhost:8080/api/neon/ping
```

## Pipeline de áudio

CardPuter → POST /api/neon/audio (WAV) → Whisper transcreve → 
processa comando → retorna JSON {text, emotion, tts_url}

Comandos reconhecidos (modo offline):
- "agenda", "dia", "hoje" → mostra agenda
- "clima", "tempo" → previsão Salvador
- "oi", "olá", "neon" → saudação
- "config" → settings
- fallback → resposta genérica

## Healthcheck

Cron `neon-bridge-healthcheck` (a cada 10min) verifica se o bridge tá vivo
e reinicia se necessário.
