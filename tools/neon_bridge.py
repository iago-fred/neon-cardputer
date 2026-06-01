#!/usr/bin/env python3
"""
Neon Bridge — Ponte entre Cardputer e a Neon no Telegram

Recebe mensagens do Cardputer via HTTP e injeta na sessão do Telegram
através da API do Gateway do OpenClaw.

Endpoints:
  GET  /api/neon/ping           — Health check
  POST /api/neon/message        — Receber mensagem do Cardputer
  POST /api/neon/send           — Enviar mensagem pro chat do Telegram
"""

import json
import os
import sys
import time
import urllib.request
import urllib.error
from http.server import HTTPServer, BaseHTTPRequestHandler

# ── Config ──────────────────────────────────────────────────────────────────
GATEWAY_PORT = 18789
GATEWAY_TOKEN_FILE = "/data/.openclaw/openclaw.json"
TELEGRAM_CHAT_ID = "8829697706"
BOT_TOKEN = ""  # Preenchido na inicialização

# ── Utils ────────────────────────────────────────────────────────────────────

def load_gateway_token():
    """Lê o token do Gateway (env var ou openclaw.json)"""
    token = os.environ.get("OPENCLAW_GATEWAY_TOKEN", "")
    if token:
        return token
    try:
        with open(GATEWAY_TOKEN_FILE) as f:
            config = json.load(f)
        raw = config.get("gateway", {}).get("auth", {}).get("token", "")
        # Se for placeholder ${VAR}, tenta extrair nome
        if raw.startswith("${") and raw.endswith("}"):
            var_name = raw[2:-1]
            return os.environ.get(var_name, "")
        return raw
    except Exception as e:
        print(f"[Bridge] Erro ao ler token: {e}")
        return ""


def load_bot_token():
    """Tenta ler o token do bot da config do Telegram"""
    config_file = "/data/.openclaw/openclaw.json"
    try:
        with open(config_file) as f:
            config = json.load(f)
        telegram = config.get("channels", {}).get("telegram", {})
        accounts = telegram.get("accounts", {})
        neon = accounts.get("neon", {})
        return neon.get("botToken", "")
    except Exception:
        return ""


def invoke_gateway_tool(tool_name, args, session_key="agent:main:telegram:direct:8829697706"):
    """Invoca uma ferramenta do Gateway via HTTP API"""
    token = GATEWAY_TOKEN
    if not token:
        token = load_gateway_token()
    
    payload = json.dumps({
        "tool": tool_name,
        "sessionKey": session_key,
        "args": args
    }).encode()
    
    req = urllib.request.Request(
        f"http://localhost:{GATEWAY_PORT}/tools/invoke",
        data=payload,
        headers={
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json"
        }
    )
    
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            return json.loads(resp.read().decode())
    except urllib.error.HTTPError as e:
        body = e.read().decode() if e.fp else str(e)
        return {"error": f"HTTP {e.code}: {body}"}
    except Exception as e:
        return {"error": str(e)}


def send_telegram_message(text):
    """Envia mensagem para o Telegram via Bot API"""
    global BOT_TOKEN
    if not BOT_TOKEN:
        BOT_TOKEN = load_bot_token()
    if not BOT_TOKEN:
        return {"error": "No bot token"}
    
    payload = json.dumps({
        "chat_id": TELEGRAM_CHAT_ID,
        "text": text
    }).encode()
    
    url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"
    req = urllib.request.Request(
        url,
        data=payload,
        headers={"Content-Type": "application/json"}
    )
    
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            return json.loads(resp.read().decode())
    except Exception as e:
        return {"error": str(e)}


CARD_MESSAGES_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "cardputer_messages.jsonl")

def save_cardputer_message(text):
    """Salva mensagem do Cardputer em arquivo JSONL pra um cron processar"""
    try:
        entry = json.dumps({
            "text": text,
            "timestamp": time.time(),
            "processed": False
        }, ensure_ascii=False)
        with open(CARD_MESSAGES_FILE, "a") as f:
            f.write(entry + "\n")
        return True
    except Exception as e:
        print(f"[Bridge] Erro ao salvar mensagem: {e}")
        return False

def inject_cardputer_message(text):
    """Injeta a mensagem do Cardputer"""
    # Envia pro Telegram visualmente
    send_telegram_message(text)
    
    # Salva pra ser processada pelo cron
    save_cardputer_message(text)
    
    return {"status": "saved"}


# ── HTTP Server ──────────────────────────────────────────────────────────────

class BridgeHandler(BaseHTTPRequestHandler):
    
    def _send_json(self, data, status=200):
        body = json.dumps(data, ensure_ascii=False).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self._cors_headers()
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    
    def _cors_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization")
    
    def do_OPTIONS(self):
        self.send_response(204)
        self._cors_headers()
        self.end_headers()
    
    def do_GET(self):
        if self.path == "/api/neon/ping":
            self._send_json({"status": "ok", "timestamp": time.time()})
        elif self.path == "/api/neon/status":
            self._send_json({
                "status": "ok",
                "bridge": "Neon Bridge v1.0",
                "uptime": time.time() - start_time,
                "chat_id": TELEGRAM_CHAT_ID
            })
        else:
            self._send_json({"error": "Not found"}, 404)
    
    def do_POST(self):
        # Read body
        content_length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(content_length) if content_length > 0 else b"{}"
        
        try:
            data = json.loads(body.decode())
        except json.JSONDecodeError:
            self._send_json({"error": "Invalid JSON"}, 400)
            return
        
        if self.path == "/api/neon/message":
            text = data.get("text", "")
            if not text:
                self._send_json({"error": "Missing 'text' field"}, 400)
                return
            
            print(f"[Bridge] Cardputer: {text}")
            result = inject_cardputer_message(text)
            self._send_json({"status": "ok", "sent": True, "result": "injected"})
            
        elif self.path == "/api/neon/send":
            text = data.get("text", "")
            if not text:
                self._send_json({"error": "Missing 'text' field"}, 400)
                return
            
            result = send_telegram_message(text)
            self._send_json({"status": "ok", "sent": True, "result": result})
            
        else:
            self._send_json({"error": "Not found"}, 404)
    
    def log_message(self, format, *args):
        print(f"[Bridge] {args[0]} {args[1]} {args[2]}")


# ── Main ─────────────────────────────────────────────────────────────────────

def main():
    global start_time, GATEWAY_TOKEN, BOT_TOKEN
    
    port = int(sys.argv[sys.argv.index("--port") + 1]) if "--port" in sys.argv else 8080
    host = sys.argv[sys.argv.index("--host") + 1] if "--host" in sys.argv else "0.0.0.0"
    
    start_time = time.time()
    GATEWAY_TOKEN = load_gateway_token()
    BOT_TOKEN = load_bot_token()
    
    print(f"[Neon Bridge] Iniciando em {host}:{port}")
    print(f"[Neon Bridge] Chat ID: {TELEGRAM_CHAT_ID}")
    print(f"[Neon Bridge] Bot token: {'✅' if BOT_TOKEN else '❌'}")
    print(f"[Neon Bridge] Gateway token: {'✅' if GATEWAY_TOKEN else '❌'}")
    
    server = HTTPServer((host, port), BridgeHandler)
    print(f"[Neon Bridge] Pronto! Endpoints:")
    print(f"  GET  /api/neon/ping     — Health check")
    print(f"  POST /api/neon/message   — Receber mensagem do Cardputer")
    print(f"  POST /api/neon/send      — Enviar mensagem pro Telegram")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[Neon Bridge] Encerrando...")
        server.server_close()

if __name__ == "__main__":
    main()
