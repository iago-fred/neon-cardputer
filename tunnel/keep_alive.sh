#!/bin/sh
# keep_alive.sh — Mantém o serveo tunnel rodando e captura a URL

TUNNEL_DIR="/data/.openclaw/workspace/neon-cardputer/tunnel"
PID_FILE="$TUNNEL_DIR/tunnel.pid"
URL_FILE="$TUNNEL_DIR/current_url.txt"
LOG_FILE="/tmp/serveo_tunnel.log"

# Se o tunnel já está rodando, sai
if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if kill -0 "$PID" 2>/dev/null; then
        exit 0
    fi
fi

# Remove PID antigo
rm -f "$PID_FILE"

# Inicia novo tunnel
nohup ssh -o StrictHostKeyChecking=no -o ServerAliveInterval=30 \
    -R 80:localhost:8080 serveo.net > "$LOG_FILE" 2>&1 &
TUNNEL_PID=$!
echo "$TUNNEL_PID" > "$PID_FILE"

# Aguarda e captura URL
sleep 10
URL=$(grep -oP 'https://[a-zA-Z0-9-]+\.serveousercontent\.com' "$LOG_FILE" | head -1)
if [ -n "$URL" ]; then
    echo "$URL" > "$URL_FILE"
    echo "Tunnel: $URL"
fi
