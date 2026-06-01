#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <M5Cardputer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ── Constantes ──────────────────────────────────────────────────────────────
#define MAX_MENSAGENS 8        // Tamanho do buffer de scrollback
#define TAMANHO_LINHA 55       // Máx caracteres por linha (240px / ~4px por char)
#define MAX_LINHA_INPUT 50     // Máx caracteres no input
#define INTERVALO_GETUPDATES 3000 // 3s entre polls
#define ANIMACAO_DELAY 600     // ms entre frames da animação idle

// ── Globais ─────────────────────────────────────────────────────────────────
String bridgeUrl = "";
String botToken = "";
String chatId = "";
String textoInput = "";

// Buffer de scrollback (circular)
String historico[MAX_MENSAGENS];
int histIndex = 0;       // Onde escrever a próxima mensagem
int histCount = 0;       // Quantas mensagens no buffer
int histScroll = 0;      // Posição de scroll (0 = última mensagem)

// Controle de recebimento
long ultimoUpdateId = 0;
unsigned long ultimoCheckJson = 0;
unsigned long ultimoWifiCheck = 0;

// Estado do WiFi backoff
int wifiBackoff = 5;     // segundos, começa com 5
unsigned long ultimoBackoff = 0;

// Animação idle
bool olhosAbertos = true;
unsigned long ultimaAnimacao = 0;

// ── Utilitários ────────────────────────────────────────────────────────────

String escaparJson(String texto) {
    texto.replace("\\", "\\\\");
    texto.replace("\"", "\\\"");
    texto.replace("\n", "\\n");
    texto.replace("\r", "\\r");
    texto.replace("\t", "\\t");
    return texto;
}

// Quebra texto longo em várias linhas pra caber na tela
String quebrarLinha(String texto) {
    if (texto.length() <= TAMANHO_LINHA) return texto;
    String resultado = "";
    int pos = 0;
    while (pos < (int)texto.length()) {
        if (pos > 0) resultado += "\n";
        int fim = pos + TAMANHO_LINHA;
        if (fim > (int)texto.length()) fim = texto.length();
        resultado += texto.substring(pos, fim);
        pos = fim;
    }
    return resultado;
}

// ── Tela inicial: carinha Neon ─────────────────────────────────────────────
void desenharNeonFace(int x, int y, bool olhosAbertos) {
    // Círculo da cabeça (ciano)
    M5Cardputer.Display.fillCircle(x, y, 14, TFT_CYAN);
    M5Cardputer.Display.drawCircle(x, y, 14, TFT_BLUE);
    
    // Olhos (branco)
    M5Cardputer.Display.fillCircle(x - 5, y - 4, 4, TFT_WHITE);
    M5Cardputer.Display.fillCircle(x + 5, y - 4, 4, TFT_WHITE);
    
    // Pupilas (preto ou azul escuro)
    if (olhosAbertos) {
        M5Cardputer.Display.fillCircle(x - 5, y - 4, 2, TFT_DARKGREY);
        M5Cardputer.Display.fillCircle(x + 5, y - 4, 2, TFT_DARKGREY);
    } else {
        // Olhos fechados: tracinho
        M5Cardputer.Display.drawLine(x - 8, y - 4, x - 2, y - 4, TFT_DARKGREY);
        M5Cardputer.Display.drawLine(x + 2, y - 4, x + 8, y - 4, TFT_DARKGREY);
    }
    
    // Boca (sorriso)
    M5Cardputer.Display.drawLine(x - 4, y + 5, x, y + 8, TFT_DARKGREY);
    M5Cardputer.Display.drawLine(x, y + 8, x + 4, y + 5, TFT_DARKGREY);
    
    // Fones de ouvido (tracinhos laterais)
    M5Cardputer.Display.drawLine(x - 15, y - 8, x - 18, y - 12, TFT_DARKGREY);
    M5Cardputer.Display.drawLine(x + 15, y - 8, x + 18, y - 12, TFT_DARKGREY);
    M5Cardputer.Display.fillCircle(x - 18, y - 12, 2, TFT_DARKGREY);
    M5Cardputer.Display.fillCircle(x + 18, y - 12, 2, TFT_DARKGREY);
}

void telaIdle() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    
    // Carinha Neon no centro
    desenharNeonFace(80, 45, olhosAbertos);
    
    // Texto "Neon" abaixo
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5Cardputer.Display.setCursor(35, 65);
    M5Cardputer.Display.print("Neon");
    
    // Status
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5Cardputer.Display.setCursor(25, 90);
    if (bridgeUrl != "") {
        M5Cardputer.Display.print("Bridge conectada");
    } else if (botToken != "") {
        M5Cardputer.Display.print("Modo Telegram");
    } else {
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.print("Sem config");
    }
    
    // WiFi
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setCursor(10, 108);
    M5Cardputer.Display.printf("WiFi: %s | IP: %s",
        WiFi.isConnected() ? "OK" : "...",
        WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "---");
    
    // Dica
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.setCursor(30, 122);
    M5Cardputer.Display.print("Pressione qualquer tecla");
}

// ── Gerenciamento de scrollback ────────────────────────────────────────────

void adicionarHistorico(String mensagem) {
    historico[histIndex] = mensagem;
    histIndex = (histIndex + 1) % MAX_MENSAGENS;
    if (histCount < MAX_MENSAGENS) histCount++;
    histScroll = 0;  // Volta o scroll pra última mensagem
}

// ── Carregar config ────────────────────────────────────────────────────────

void carregarConfigTelegram(String conteudoJson) {
    int bridgeParam = conteudoJson.indexOf("\"bridge_url\"");
    if (bridgeParam != -1) {
        int bridgeInicio = conteudoJson.indexOf("\"", bridgeParam + 12) + 1;
        int bridgeFim = conteudoJson.indexOf("\"", bridgeInicio);
        bridgeUrl = conteudoJson.substring(bridgeInicio, bridgeFim);
        bridgeUrl.trim();
        // Remove trailing slash se tiver
        if (bridgeUrl.endsWith("/")) bridgeUrl.remove(bridgeUrl.length() - 1);
    }

    int tokenParam = conteudoJson.indexOf("\"token\"");
    if (tokenParam != -1) {
        int tokenInicio = conteudoJson.indexOf("\"", tokenParam + 7) + 1;
        int tokenFim = conteudoJson.indexOf("\"", tokenInicio);
        botToken = conteudoJson.substring(tokenInicio, tokenFim);
        botToken.trim();
    }

    int chatParam = conteudoJson.indexOf("\"chat_id\"");
    if (chatParam != -1) {
        int chatInicio = conteudoJson.indexOf("\"", chatParam + 9) + 1;
        int chatFim = conteudoJson.indexOf("\"", chatInicio);
        chatId = conteudoJson.substring(chatInicio, chatFim);
        chatId.trim();
    }
}

// ── Forward declarations ───────────────────────────────────────────────────
void enviarMensagemTelegram(String mensagem);

// ── Enviar pra Bridge ───────────────────────────────────────────────────────

void enviarParaBridge(String mensagem) {
    if (mensagem.length() == 0) return;

    if (bridgeUrl != "") {
        WiFiClientSecure cliente;
        cliente.setInsecure();

        HTTPClient http;
        http.begin(cliente, bridgeUrl + "/api/neon/message");
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(5000);

        String textoEscapado = escaparJson(mensagem);
        String payload = "{\"text\":\"" + textoEscapado + "\"}";
        int httpCode = http.POST(payload);
        
        if (httpCode <= 0) {
            // Bridge falhou → fallback pro Telegram direto
            if (botToken != "" && chatId != "") {
                enviarMensagemTelegram(mensagem);
            }
        }
        http.end();
    } else if (botToken != "" && chatId != "") {
        enviarMensagemTelegram(mensagem);
    }
}

// ── Enviar direto pro Telegram (fallback) ──────────────────────────────────

void enviarMensagemTelegram(String mensagem) {
    if (botToken == "" || chatId == "" || mensagem.length() == 0) return;

    WiFiClientSecure cliente;
    cliente.setInsecure();

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
    http.begin(cliente, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);

    String textoEscapado = escaparJson(mensagem);
    String payload = "{\"chat_id\":\"" + chatId + "\",\"text\":\"" + textoEscapado + "\"}";
    http.POST(payload);
    http.end();
}

// ── Receber mensagens (getUpdates) ─────────────────────────────────────────

void receberMensagensTelegram() {
    if (botToken == "") return;

    WiFiClientSecure cliente;
    cliente.setInsecure();

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/getUpdates?limit=1";
    if (ultimoUpdateId > 0) url += "&offset=" + String(ultimoUpdateId);
    
    http.begin(cliente, url);
    http.setTimeout(5000);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String resposta = http.getString();
        
        int upIdPos = resposta.indexOf("\"update_id\":");
        if (upIdPos != -1) {
            int upIdInicio = upIdPos + 12;
            int upIdFim = resposta.indexOf(",", upIdInicio);
            if (upIdFim != -1) {
                String upIdStr = resposta.substring(upIdInicio, upIdFim);
                ultimoUpdateId = upIdStr.toInt() + 1;
            }

            // Extrai texto da mensagem (procurando dentro de "message":{...})
            int msgPos = resposta.indexOf("\"message\":{");
            if (msgPos == -1) msgPos = resposta.indexOf("\"message\":{\"");
            if (msgPos != -1) {
                int textPos = resposta.indexOf("\"text\":\"", msgPos);
                if (textPos != -1) {
                    int textInicio = textPos + 8;
                    int textFim = resposta.indexOf("\"", textInicio);
                    if (textFim != -1 && textFim > textInicio) {
                        String textoRecebido = resposta.substring(textInicio, textFim);
                        String msgFormatada = "👻 Neon >> " + textoRecebido;
                        adicionarHistorico(msgFormatada);
                    }
                }
            }
        }
    }
    http.end();
}

// ── Conteúdo de uma mensagem do histórico (posição relativa ao scroll) ─────

String mensagemVisivel(int pos) {
    // pos 0 = mais recente, pos 1 = anterior, etc.
    int idx = (histIndex - 1 - pos + MAX_MENSAGENS) % MAX_MENSAGENS;
    if (pos < histCount) {
        return historico[idx];
    }
    return "";
}

// ── Atualizar tela do chat ─────────────────────────────────────────────────

void atualizarTelaChat() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    
    // Cabeçalho: nome Neon
    M5Cardputer.Display.fillRect(0, 0, 240, 15, TFT_BLUE);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(5, 2);
    M5Cardputer.Display.print("Neon Chat");
    
    // Indicador de scroll (se tiver mais mensagens que cabem)
    int linhasVisiveis = 4;  // Quantas linhas cabem na área de chat
    int totalMensagens = histCount;
    if (totalMensagens > linhasVisiveis) {
        M5Cardputer.Display.setTextColor(TFT_DARKGREY);
        M5Cardputer.Display.setCursor(180, 2);
        M5Cardputer.Display.printf("%d/%d", histScroll + 1, totalMensagens);
    }
    
    // Linha separadora
    M5Cardputer.Display.drawLine(0, 16, 240, 16, TFT_DARKGREY);
    
    // Área de chat: mostra últimas mensagens do scroll pra cima
    int yBase = 20;
    int alturaLinha = 11;
    
    for (int i = 0; i < linhasVisiveis; i++) {
        int msgIdx = histScroll + (linhasVisiveis - 1 - i);
        String msg = mensagemVisivel(msgIdx);
        if (msg.length() == 0) continue;
        
        int y = yBase + (i * alturaLinha);
        if (y > 110) break;  // Não passar da área de input
        
        // Cor diferente pra mensagens minhas vs Neon
        if (msg.startsWith("👻")) {
            M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
        } else if (msg.startsWith("Voce:")) {
            M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
        } else if (msg.startsWith("📟")) {
            M5Cardputer.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
        } else {
            M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        
        M5Cardputer.Display.setCursor(5, y);
        M5Cardputer.Display.print(msg.substring(0, TAMANHO_LINHA));
    }
    
    // Status WiFi (canto)
    M5Cardputer.Display.setTextColor(WiFi.isConnected() ? TFT_GREEN : TFT_RED, TFT_BLACK);
    M5Cardputer.Display.setCursor(5, 100);
    M5Cardputer.Display.print(WiFi.isConnected() ? "WiFi" : "---");
    
    // Mensagem de "conectando" se WiFi caiu
    if (!WiFi.isConnected()) {
        M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5Cardputer.Display.setCursor(35, 100);
        M5Cardputer.Display.print("Reconectando...");
    }
    
    // Campo de input
    M5Cardputer.Display.fillRect(0, 115, 240, 20, TFT_DARKGREY);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_DARKGREY);
    M5Cardputer.Display.setCursor(5, 120);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.printf("> %s", textoInput.c_str());
    M5Cardputer.Display.drawLine(0, 115, 240, 115, TFT_BLUE);
}

// ── Reconexão WiFi com backoff exponencial ─────────────────────────────────

bool verificarWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        wifiBackoff = 5;  // Reset backoff
        return true;
    }
    
    unsigned long agora = millis();
    unsigned long espera = wifiBackoff * 1000;
    
    if (agora - ultimoBackoff > espera) {
        ultimoBackoff = agora;
        
        // Mostra status na tela
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        M5Cardputer.Display.setTextColor(TFT_YELLOW);
        M5Cardputer.Display.setCursor(10, 40);
        M5Cardputer.Display.printf("WiFi caiu!");
        M5Cardputer.Display.setCursor(10, 55);
        M5Cardputer.Display.printf("Tentando em %ds...", wifiBackoff);
        M5Cardputer.Display.setCursor(10, 70);
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.print("Conectando...");
        
        WiFi.reconnect();
        
        // Aguarda até 10s
        unsigned long timeout = millis();
        while (millis() - timeout < 10000) {
            if (WiFi.status() == WL_CONNECTED) {
                wifiBackoff = 5;
                return true;
            }
            delay(100);
        }
        
        // Backoff exponencial: 5s → 15s → 30s → 60s (máx)
        wifiBackoff = min(wifiBackoff * 3, 60);
    }
    
    return false;
}

// ── Loop principal do chat ─────────────────────────────────────────────────

void iniciarChatTelegram(String conteudoJson) {
    carregarConfigTelegram(conteudoJson);

    if (botToken == "" && bridgeUrl == "") {
        telaIdle();
        // Fica em loop mostrando idle mesmo sem config
        while (true) {
            M5Cardputer.update();
            if (millis() - ultimaAnimacao > ANIMACAO_DELAY) {
                olhosAbertos = !olhosAbertos;
                telaIdle();
                ultimaAnimacao = millis();
            }
            delay(50);
        }
    }

    // Envia notificação de início (sem hardcode)
    String startupMsg = "📟 Cardputer conectado!";
    enviarParaBridge(startupMsg);
    adicionarHistorico("Voce: " + startupMsg);
    
    atualizarTelaChat();
    ultimaAnimacao = millis();

    while (true) {
        M5Cardputer.update();
        unsigned long agora = millis();

        // ── Verificação de WiFi com backoff ──
        bool wifiOk = WiFi.isConnected();
        if (!wifiOk && (agora - ultimoWifiCheck > 30000)) {
            ultimoWifiCheck = agora;
            wifiOk = verificarWiFi();
            atualizarTelaChat();
        }

        // ── Polling de novas mensagens do Telegram ──
        if (wifiOk && (agora - ultimoCheckJson > INTERVALO_GETUPDATES)) {
            receberMensagensTelegram();
            atualizarTelaChat();
            ultimoCheckJson = agora;
        }

        // ── Teclado ── (funciona mesmo sem WiFi)
        if (M5Cardputer.Keyboard.isChange()) {
            if (M5Cardputer.Keyboard.isPressed()) {
                auto status = M5Cardputer.Keyboard.keysState();

                // Letras e números
                for (auto i : status.word) {
                    if (textoInput.length() < MAX_LINHA_INPUT) {
                        textoInput += i;
                    }
                }

                // Delete/Backspace
                if (status.del && textoInput.length() > 0) {
                    textoInput.remove(textoInput.length() - 1);
                    atualizarTelaChat();
                }

                // Scroll: tab volta (antiga), fn+del avança (recente)
                if (status.tab && histCount > 0) {
                    if (histScroll < histCount - 1) histScroll++;
                    else histScroll = 0;
                    atualizarTelaChat();
                }

                // Enter: envia mensagem
                if (status.enter && textoInput.length() > 0) {
                    enviarParaBridge(textoInput);
                    adicionarHistorico("Voce: " + textoInput);
                    textoInput = "";
                    histScroll = 0;
                    atualizarTelaChat();
                }
            }
        }
        delay(30);
    }
}

#endif
