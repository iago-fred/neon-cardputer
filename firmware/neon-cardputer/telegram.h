#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <M5Cardputer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

String bridgeUrl = "";
String botToken = "";
String chatId = "";
String textoInput = "";
String ultimaMensagemChat = "Neon ativa! Digite algo...";

long ultimoUpdateId = 0;
unsigned long ultimoCheckJson = 0;
unsigned long ultimoWifiCheck = 0;

// ── Escapar caracteres especiais pra JSON ──────────────────────────────────
String escaparJson(String texto) {
    texto.replace("\\", "\\\\");
    texto.replace("\"", "\\\"");
    texto.replace("\n", "\\n");
    texto.replace("\r", "\\r");
    texto.replace("\t", "\\t");
    return texto;
}

// ── Carregar config do JSON ────────────────────────────────────────────────
void carregarConfigTelegram(String conteudoJson) {
    // Bridge URL (novo)
    int bridgeParam = conteudoJson.indexOf("\"bridge_url\"");
    if (bridgeParam != -1) {
        int bridgeInicio = conteudoJson.indexOf("\"", bridgeParam + 12) + 1;
        int bridgeFim = conteudoJson.indexOf("\"", bridgeInicio);
        bridgeUrl = conteudoJson.substring(bridgeInicio, bridgeFim);
        bridgeUrl.trim();
    }

    // Bot token (fallback)
    int tokenParam = conteudoJson.indexOf("\"token\"");
    if (tokenParam != -1) {
        int tokenInicio = conteudoJson.indexOf("\"", tokenParam + 7) + 1;
        int tokenFim = conteudoJson.indexOf("\"", tokenInicio);
        botToken = conteudoJson.substring(tokenInicio, tokenFim);
        botToken.trim();
    }

    // Chat ID (fallback)
    int chatParam = conteudoJson.indexOf("\"chat_id\"");
    if (chatParam != -1) {
        int chatInicio = conteudoJson.indexOf("\"", chatParam + 9) + 1;
        int chatFim = conteudoJson.indexOf("\"", chatInicio);
        chatId = conteudoJson.substring(chatInicio, chatFim);
        chatId.trim();
    }
}

// ── Enviar mensagem pra Neon Bridge ────────────────────────────────────────
void enviarParaBridge(String mensagem) {
    if (bridgeUrl == "") {
        // Fallback: manda direto pro Telegram (só visual, Neon não vê)
        if (botToken != "" && chatId != "") {
            enviarMensagemTelegram(mensagem);
        }
        return;
    }

    WiFiClientSecure cliente;
    cliente.setInsecure();

    HTTPClient http;
    http.begin(cliente, bridgeUrl + "/api/neon/message");
    http.addHeader("Content-Type", "application/json");

    String textoEscapado = escaparJson(mensagem);
    String payload = "{\"text\":\"" + textoEscapado + "\"}";
    int httpCode = http.POST(payload);

    if (httpCode <= 0 && botToken != "" && chatId != "") {
        // Se bridge falhou, tenta fallback direto no Telegram
        enviarMensagemTelegram(mensagem);
    }

    http.end();
}

// ── Enviar mensagem direto pro Telegram (fallback) ─────────────────────────
void enviarMensagemTelegram(String mensagem) {
    if (botToken == "" || chatId == "") return;

    WiFiClientSecure cliente;
    cliente.setInsecure();

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
    
    http.begin(cliente, url);
    http.addHeader("Content-Type", "application/json");

    String textoEscapado = escaparJson(mensagem);
    String payload = "{\"chat_id\":\"" + chatId + "\",\"text\":\"" + textoEscapado + "\"}";
    http.POST(payload);
    http.end();
}

// ── Receber mensagens do Telegram (getUpdates) ─────────────────────────────
void receberMensagensTelegram() {
    if (botToken == "") return;

    // Verifica se WiFi ainda tá conectado
    if (WiFi.status() != WL_CONNECTED) {
        // Tenta reconectar
        WiFi.reconnect();
        return;
    }

    WiFiClientSecure cliente;
    cliente.setInsecure();

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/getUpdates?limit=1";
    if (ultimoUpdateId > 0) {
        url += "&offset=" + String(ultimoUpdateId);
    }
    
    http.begin(cliente, url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String resposta = http.getString();
        
        // Procura update_id
        int upIdPos = resposta.indexOf("\"update_id\":");
        if (upIdPos != -1) {
            int upIdInicio = upIdPos + 12;
            int upIdFim = resposta.indexOf(",", upIdInicio);
            String upIdStr = resposta.substring(upIdInicio, upIdFim);
            
            ultimoUpdateId = upIdStr.toInt() + 1;

            // Extrai texto da mensagem
            int msgPos = resposta.indexOf("\"message\":{\"");
            if (msgPos == -1) msgPos = resposta.indexOf("\"message\":{");
            if (msgPos != -1) {
                int textPos = resposta.indexOf("\"text\":\"", msgPos);
                if (textPos != -1) {
                    int textInicio = textPos + 8;
                    int textFim = resposta.indexOf("\"", textInicio);
                    if (textFim != -1) {
                        String textoRecebido = resposta.substring(textInicio, textFim);
                        ultimaMensagemChat = "Neon: " + textoRecebido;
                    }
                }
            }
        }
    }
    http.end();
}

// ── Atualizar tela ─────────────────────────────────────────────────────────
void atualizarTelaChat() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    
    // Cabeçalho azul
    M5Cardputer.Display.fillRect(0, 0, 240, 15, TFT_BLUE);
    M5Cardputer.Display.setCursor(5, 2);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.print("Neon Chat");

    // Linha de separação
    M5Cardputer.Display.drawLine(0, 16, 240, 16, TFT_DARKGREY);

    // Última mensagem
    M5Cardputer.Display.setCursor(10, 30);
    M5Cardputer.Display.setTextColor(TFT_GREEN);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.print(ultimaMensagemChat.c_str());

    // Status WiFi
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setCursor(5, 100);
    M5Cardputer.Display.printf("WiFi: %s", WiFi.isConnected() ? "OK" : "---");

    // Campo de input
    M5Cardputer.Display.fillRect(0, 115, 240, 20, TFT_DARKGREY);
    M5Cardputer.Display.setCursor(5, 120);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.printf("> %s", textoInput.c_str());
}

// ── Loop principal do chat ─────────────────────────────────────────────────
void iniciarChatTelegram(String conteudoJson) {
    carregarConfigTelegram(conteudoJson);

    if (botToken == "" && bridgeUrl == "") {
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.setCursor(10, 40);
        M5Cardputer.Display.println("Erro: sem config!");
        return;
    }

    enviarParaBridge("Cardputer conectado!");
    atualizarTelaChat();

    while (true) {
        M5Cardputer.update();

        // Checa WiFi a cada 30s
        if (millis() - ultimoWifiCheck > 30000) {
            if (WiFi.status() != WL_CONNECTED) {
                WiFi.reconnect();
            }
            ultimoWifiCheck = millis();
        }

        // Checa novas mensagens do Telegram a cada 3 segundos
        if (millis() - ultimoCheckJson > 3000) {
            receberMensagensTelegram();
            atualizarTelaChat();
            ultimoCheckJson = millis();
        }

        // Teclado
        if (M5Cardputer.Keyboard.isChange()) {
            if (M5Cardputer.Keyboard.isPressed()) {
                auto status = M5Cardputer.Keyboard.keysState();

                for (auto i : status.word) {
                    textoInput += i;
                }

                if (status.del && textoInput.length() > 0) {
                    textoInput.remove(textoInput.length() - 1);
                }

                if (status.enter && textoInput.length() > 0) {
                    enviarParaBridge(textoInput);
                    ultimaMensagemChat = "Voce: " + textoInput;
                    textoInput = "";
                }
                atualizarTelaChat();
            }
        }
        delay(30);
    }
}

#endif
