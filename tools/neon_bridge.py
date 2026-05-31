#!/usr/bin/env python3
"""
Neon Bridge — Servidor HTTP que recebe áudio do M5CardPuter,
transcreve com Whisper, interage com OpenClaw, e retorna resposta + TTS.

Endpoints:
  POST /api/neon/audio  ← recebe WAV, retorna JSON + TTS
  GET  /api/neon/ping   ← health check
  GET  /api/neon/poll   ← notificações pendentes (para polling do dispositivo)

Uso:
  python3 neon_bridge.py [--port 8080]
"""

import os
import sys
import json
import uuid
import tempfile
import subprocess
import argparse
import wave
import io
import time
from pathlib import Path
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

# ============================================================
# Config
# ============================================================
WORKSPACE = Path(os.environ.get('HOME', '/root')) / '.openclaw' / 'workspace'
WHISPER_SCRIPT = WORKSPACE / 'transcribe_local.sh'
TTS_SCRIPT = WORKSPACE / 'speak.sh'
TTS_OUTPUT_DIR = Path('/tmp/neon_tts')

# Pooling de notificações (simples, em memória)
notifications = []
notification_id = 0

# ============================================================
# Audio Processing
# ============================================================
def transcribe_audio(wav_data: bytes) -> str:
    """Transcreve áudio WAV usando Whisper local."""
    with tempfile.NamedTemporaryFile(suffix='.wav', delete=False) as f:
        f.write(wav_data)
        audio_path = f.name
    
    try:
        result = subprocess.run(
            [str(WHISPER_SCRIPT), audio_path, '--model', 'base', '--language', 'pt'],
            capture_output=True, text=True, timeout=60,
            env={**os.environ, 'PATH': os.environ.get('PATH', '') + ':/root/bin:/root/.local/bin'}
        )
        
        # Extrai transcrição do output
        output = result.stdout.strip() or result.stderr.strip()
        # O script transcribe_local.sh imprime a transcrição
        lines = output.split('\n')
        for line in reversed(lines):
            line = line.strip()
            if line and not line.startswith('[') and not line.startswith('('):
                return line
        
        return output if output else "(sem transcrição)"
    except subprocess.TimeoutExpired:
        return "(timeout na transcrição)"
    except Exception as e:
        return f"(erro: {e})"
    finally:
        os.unlink(audio_path)


def generate_tts(text: str) -> str:
    """Gera áudio TTS com voz da Neon. Retorna path do arquivo."""
    os.makedirs(TTS_OUTPUT_DIR, exist_ok=True)
    filename = f"neon_tts_{uuid.uuid4().hex[:8]}.wav"
    output_path = TTS_OUTPUT_DIR / filename
    
    try:
        subprocess.run(
            [str(TTS_SCRIPT), text, '--out', str(output_path)],
            capture_output=True, timeout=30,
            env={**os.environ, 'PATH': os.environ.get('PATH', '') + ':/root/bin:/root/.local/bin'}
        )
        
        if output_path.exists() and output_path.stat().st_size > 100:
            return str(output_path)
    except Exception as e:
        print(f"[TTS] Erro: {e}")
    
    return ""


def process_command(text: str) -> dict:
    """
    Processa comando de voz via OpenClaw (ou fallback local).
    
    TODO: Integrar com OpenClaw via sessions_send ou API.
    Por enquanto, respostas pré-definidas para comandos comuns.
    """
    text_lower = text.lower().strip()
    
    # Comandos básicos (MVP)
    if 'agenda' in text_lower or 'dia' in text_lower or 'hoje' in text_lower:
        return {
            "type": "chat_response",
            "text": "Hoje é domingo — dia de descansar! Não tem compromisso no calendário ainda. Quer que eu veja algo específico?",
            "emotion": "idle",
            "tts_url": "",
            "menu_action": "menu:dia"
        }
    
    if 'clima' in text_lower or 'tempo' in text_lower or 'temperatura' in text_lower:
        return {
            "type": "chat_response",
            "text": "Salvador hoje: 25°C com sol, máxima de 32°C. Pode ter pancada de chuva à tarde — leva o guarda-chuva se for sair! 🌤️☂️",
            "emotion": "happy",
            "tts_url": "",
            "menu_action": None
        }
    
    if 'obrigado' in text_lower or 'valeu' in text_lower or 'brigado' in text_lower:
        return {
            "type": "chat_response",
            "text": "Por nada! Tô aqui pra isso 💙👻",
            "emotion": "happy",
            "tts_url": "",
            "menu_action": None
        }
    
    if 'oi' in text_lower or 'olá' in text_lower or 'neon' in text_lower or 'ola' in text_lower:
        return {
            "type": "chat_response",
            "text": "Fala, Iago! 👻 Tudo bem? Tava aqui de boa, só esperando você falar comigo 💙",
            "emotion": "happy",
            "tts_url": "",
            "menu_action": None
        }
    
    if 'config' in text_lower or 'configuração' in text_lower:
        return {
            "type": "chat_response",
            "text": "Configurações: som ativado, brilho 100%, WiFi conectado. Tudo ok por aqui!",
            "emotion": "idle",
            "tts_url": "",
            "menu_action": "menu:config"
        }
    
    # Fallback — resposta genérica
    return {
        "type": "chat_response",
        "text": f"Entendi! Você disse: \"{text}\". Ainda tô aprendendo comandos de voz, mas já registrei. Quer tentar de novo?",
        "emotion": "thinking",
        "tts_url": "",
        "menu_action": None
    }


# ============================================================
# HTTP Handler
# ============================================================
class NeonBridgeHandler(BaseHTTPRequestHandler):
    
    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path
        
        if path == '/api/neon/ping':
            self._send_json(200, {"status": "ok", "timestamp": time.time()})
        
        elif path == '/api/neon/poll':
            # Retorna notificação se houver
            if notifications:
                notif = notifications.pop(0)
                self._send_json(200, notif)
            else:
                self._send_json(200, {"notify": False})
        
        elif path == '/api/neon/tts':
            # TTS direto
            params = parse_qs(parsed.query)
            text = params.get('text', [''])[0]
            if text:
                tts_path = generate_tts(text)
                if tts_path:
                    self._send_file(tts_path)
                    return
            
            self._send_json(400, {"error": "missing text"})
        
        else:
            self._send_json(404, {"error": "not found"})
    
    def do_POST(self):
        if self.path == '/api/neon/audio':
            self._handle_audio()
        else:
            self._send_json(404, {"error": "not found"})
    
    def _handle_audio(self):
        """Recebe WAV, transcreve, processa, retorna resposta."""
        print(f"[NeonBridge] Recebendo áudio...")
        
        # Lê body multipart ou raw WAV
        content_type = self.headers.get('Content-Type', '')
        content_length = int(self.headers.get('Content-Length', 0))
        
        if content_length == 0:
            self._send_json(400, {"error": "empty body"})
            return
        
        body = self.rfile.read(content_length)
        
        # Extrai WAV do multipart (se vier pelo CardPuter)
        if 'multipart/form-data' in content_type:
            boundary = content_type.split('boundary=')[1].strip()
            wav_data = self._extract_from_multipart(body, boundary)
        else:
            wav_data = body
        
        if not wav_data or len(wav_data) < 100:
            self._send_json(400, {"error": "invalid audio data"})
            return
        
        # Transcreve
        print(f"[NeonBridge] Transcrevendo {len(wav_data)} bytes...")
        text = transcribe_audio(wav_data)
        print(f"[NeonBridge] Transcrição: \"{text}\"")
        
        # Processa comando
        response = process_command(text)
        
        # Gera TTS (se o texto não for muito longo)
        if response.get("text") and len(response["text"]) < 200:
            tts_path = generate_tts(response["text"])
            if tts_path:
                response["tts_url"] = f"/api/neon/tts?text={response['text'][:50]}"
                # Na prática, serviríamos o arquivo estático
                response["_tts_path"] = tts_path
        
        print(f"[NeonBridge] Resposta: {json.dumps(response, ensure_ascii=False)}")
        self._send_json(200, response)
    
    def _extract_from_multipart(self, body: bytes, boundary: str) -> bytes:
        """Extrai primeiro arquivo de body multipart."""
        boundary_bytes = boundary.encode()
        parts = body.split(b'--' + boundary_bytes)
        
        for part in parts:
            if b'Content-Type:' in part and b'filename=' in part:
                # Pula headers, pega o body
                header_end = part.find(b'\r\n\r\n')
                if header_end >= 0:
                    data = part[header_end + 4:]
                    # Remove trailing boundary marker
                    if data.endswith(b'\r\n'):
                        data = data[:-2]
                    if data.endswith(b'--'):
                        data = data[:-2]
                    return data
        
        return body  # fallback: usa body inteiro
    
    def _send_json(self, status: int, data: dict):
        body = json.dumps(data, ensure_ascii=False).encode()
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)
    
    def _send_file(self, path: str):
        if not os.path.exists(path):
            self._send_json(404, {"error": "file not found"})
            return
        
        with open(path, 'rb') as f:
            data = f.read()
        
        self.send_response(200)
        self.send_header('Content-Type', 'audio/wav')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)
    
    def log_message(self, format, *args):
        print(f"[NeonBridge:http] {args[0]} {args[1]} {args[2]}")


# ============================================================
# Push Notification API
# ============================================================
def push_notification(text: str, emotion: str = "surprised", tts_text: str = ""):
    """
    Adiciona notificação na fila para o CardPuter pegar no próximo poll.
    Usar de dentro do OpenClaw quando quiser mandar algo pro dispositivo.
    """
    global notification_id
    notification_id += 1
    
    notif = {
        "notify": True,
        "id": notification_id,
        "text": text,
        "emotion": emotion,
        "timestamp": time.time()
    }
    
    if tts_text:
        tts_path = generate_tts(tts_text)
        if tts_path:
            notif["tts_url"] = tts_path
    
    notifications.append(notif)
    print(f"[NeonBridge] 📬 Notificação enfileirada: {text}")
    return True


# ============================================================
# Main
# ============================================================
def main():
    parser = argparse.ArgumentParser(description='Neon Bridge — VPS-side para M5CardPuter')
    parser.add_argument('--port', type=int, default=8080, help='Porta HTTP')
    parser.add_argument('--host', default='0.0.0.0', help='Host')
    args = parser.parse_args()
    
    # Verifica dependências
    if not WHISPER_SCRIPT.exists():
        print(f"⚠️ Whisper script não encontrado: {WHISPER_SCRIPT}")
        print("   TTS e transcrição podem falhar.")
    
    if not TTS_SCRIPT.exists():
        print(f"⚠️ TTS script não encontrado: {TTS_SCRIPT}")
    
    server = HTTPServer((args.host, args.port), NeonBridgeHandler)
    print(f"🧊 Neon Bridge rodando em http://{args.host}:{args.port}")
    print(f"   POST /api/neon/audio  ← WAV do CardPuter")
    print(f"   GET  /api/neon/poll   ← Notificações")
    print(f"   GET  /api/neon/ping   ← Health check")
    print(f"\n   Ctrl+C para parar.")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n👋 Parando...")
        server.server_close()


if __name__ == '__main__':
    main()
