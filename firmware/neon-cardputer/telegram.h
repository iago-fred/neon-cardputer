#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <M5Cardputer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

String botToken = "";
String chatId = "";
String textoInput = "";
String ultimaMensagemChat = "Bot iniciado! Digite algo...";

long ultimoUpdateId = 0;          // Controla para não ler a mesma mensagem duas vezes
unsigned long ultimoCheckJson = 0; // Temporizador para não sobrecarregar o Telegram

void carregarConfigTelegram(String conteudoJson) {
    int tokenParam = conteudoJson.indexOf("\"token\"");
    int chatParam = conteudoJson.indexOf("\"chat_id\"");

    if (tokenParam != -1 && chatParam != -1) {
        int tokenInicio = conteudoJson.indexOf("\"", tokenParam + 7) + 1;
        int tokenFim = conteudoJson.indexOf("\"", tokenInicio);
        botToken = conteudoJson.substring(tokenInicio, tokenFim);

        int chatInicio = conteudoJson.indexOf("\"", chatParam + 9) + 1;
        int chatFim = conteudoJson.indexOf("\"", chatInicio);
        chatId = conteudoJson.substring(chatInicio, chatFim);

        botToken.trim();
        chatId.trim();
    }
}

void enviarMensagemTelegram(String mensaje) {
    if (botToken == "" || chatId == "") return;

    WiFiClientSecure cliente;
    cliente.setInsecure();

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
    
    http.begin(cliente, url);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"chat_id\":\"" + chatId + "\",\"text\":\"" + mensaje + "\"}";
    http.POST(payload);
    http.end();
}

// Nova Função: Busca mensagens enviadas do Telegram para o Bot
void receberMensagensTelegram() {
    if (botToken == "") return;

    WiFiClientSecure cliente;
    cliente.setInsecure();

    HTTPClient http;
    // Solicita apenas 1 nova mensagem por vez para economizar processamento
    String url = "https://api.telegram.org/bot" + botToken + "/getUpdates?limit=1";
    if (ultimoUpdateId > 0) {
        url += "&offset=" + String(ultimoUpdateId);
    }
    
    http.begin(cliente, url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String resposta = http.getString();
        
        int upIdPos = resposta.indexOf("\"update_id\":");
        if (upIdPos != -1) {
            int upIdInicio = upIdPos + 12;
            int upIdFim = resposta.indexOf(",", upIdInicio);
            String upIdStr = resposta.substring(upIdInicio, upIdFim);
            
            // Atualiza o ID para que o Telegram saiba que já lemos essa mensagem
            ultimoUpdateId = upIdStr.toInt() + 1;

            // Extrai o texto enviado pelo usuário no Telegram
            int textPos = resposta.indexOf("\"text\":\"");
            if (textPos != -1) {
                int textInicio = textPos + 8;
                int textFim = resposta.indexOf("\"", textInicio);
                String textoRecebido = resposta.substring(textInicio, textFim);
                
                // Atualiza o painel com a resposta que veio do celular
                ultimaMensagemChat = "Telegram: " + textoRecebido;
            }
        }
    }
    http.end();
}

void atualizarTelaChat() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    
    // Cabeçalho azul
    M5Cardputer.Display.fillRect(0, 0, 240, 15, TFT_BLUE);
    M5Cardputer.Display.setCursor(5, 2);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.print("Telegram Bot Chat");

    // Histórico / Resposta central
    M5Cardputer.Display.setCursor(10, 40);
    M5Cardputer.Display.setTextColor(TFT_GREEN);
    M5Cardputer.Display.print(ultimaMensagemChat.c_str());

    // Campo de input cinza inferior
    M5Cardputer.Display.fillRect(0, 115, 240, 20, TFT_DARKGREY);
    M5Cardputer.Display.setCursor(5, 120);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.printf("> %s", textoInput.c_str());
}

void iniciarChatTelegram(String conteudoJson) {
    carregarConfigTelegram(conteudoJson);

    if (botToken == "" || chatId == "") {
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.println("Erro: Dados do Bot nao encontrados!");
        return;
    }

    enviarMensagemTelegram("M5Cardputer conectado e ativo!");
    atualizarTelaChat();

    while (true) {
        M5Cardputer.update();

        // Checa novas mensagens vindas do Telegram a cada 3 segundos (3000ms)
        if (millis() - ultimoCheckJson > 3000) {
            receberMensagensTelegram();
            atualizarTelaChat();
            ultimoCheckJson = millis();
        }

        // Gerenciamento de digitação no teclado físico
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
                    enviarMensagemTelegram(textoInput);
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