//=============================================================================
// TelegramClient.h — Envia/recebe mensagens do Telegram pelo CardPuter
//
// Como funciona:
//   1. O token do bot fica no SD card (/neon/config.json, campo bot_token)
//   2. Quando voce aperta o botao, envia audio/texto pro Telegram
//   3. Eu (Neon) recebo a mensagem e respondo pelo Telegram
//   4. O CardPuter "escuta" as respostas e mostra na tela
//
// Vantagem: nao precisa abrir firewall na VPS — usa a API publica do Telegram
//=============================================================================
#ifndef TELEGRAMCLIENT_H
#define TELEGRAMCLIENT_H

#include <M5Cardputer.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// URL base da API do Telegram
#define TG_API "api.telegram.org"
// Intervalo entre polls (em ms) — 3 segundos
#define TG_POLL_INTERVAL 3000
// Ultimo update_id lido (pra nao repetir mensagens)
static int tg_last_update_id = 0;

class TelegramClient {
private:
    String _token;          // Token do bot (lido do SD card)
    String _chatId;         // ID do chat (do usuario Iago)
    bool _ready = false;    // True quando configurado

public:
    TelegramClient() {}

    // Configura com token do config.json
    void setup(const String& token, const String& chatId) {
        _token = token;
        _chatId = chatId;
        _ready = (_token.length() > 0 && _chatId.length() > 0);
        if (_ready) {
            Serial.println("[TG] Telegram client pronto");
        } else {
            Serial.println("[TG] Token ou ChatID vazios, Telegram desativado");
        }
    }

    bool isReady() { return _ready; }

    //-----------------------------------------------------------------
    // Envia texto pro Telegram
    // Ex: client.sendMessage("Ola Neon!")
    //-----------------------------------------------------------------
    bool sendMessage(const String& text) {
        if (!_ready) return false;

        WiFiClientSecure client;
        client.setInsecure();  // HTTPS sem certificado

        if (!client.connect(TG_API, 443)) {
            Serial.println("[TG] Erro conectando ao Telegram");
            return false;
        }

        // Monta JSON do corpo da requisicao
        String body = "{\"chat_id\":" + _chatId + ",\"text\":\"" + jsonEscape(text) + "\"}";

        client.println("POST /bot" + _token + "/sendMessage HTTP/1.1");
        client.println("Host: " + String(TG_API));
        client.println("Content-Type: application/json");
        client.println("Content-Length: " + String(body.length()));
        client.println("Connection: close");
        client.println();
        client.print(body);

        // Espera resposta
        uint32_t timeout = millis() + 5000;
        while (!client.available() && millis() < timeout) delay(10);
        client.stop();

        Serial.println("[TG] Mensagem enviada");
        return true;
    }

    //-----------------------------------------------------------------
    // Verifica se tem resposta nova (polling)
    // Retorna o texto da ultima mensagem que eu (Neon) enviei
    //-----------------------------------------------------------------
    String pollResponse() {
        if (!_ready) return "";

        WiFiClientSecure client;
        client.setInsecure();

        if (!client.connect(TG_API, 443)) return "";

        // Pede atualizacoes desde o ultimo id
        String path = "/bot" + _token + "/getUpdates?offset=" + String(tg_last_update_id) + "&timeout=5";

        client.println("GET " + path + " HTTP/1.1");
        client.println("Host: " + String(TG_API));
        client.println("Connection: close");
        client.println();

        uint32_t timeout = millis() + 8000;
        String response;
        while (!client.available() && millis() < timeout) delay(10);
        while (client.available()) response += client.readString();
        client.stop();

        // Interpreta o JSON de resposta
        int jsonStart = response.indexOf('{');
        if (jsonStart < 0) return "";

        String jsonBody = response.substring(jsonStart);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonBody);

        if (error) return "";

        // Procura a ultima mensagem que NAO foi enviada pelo CardPuter
        // (ou seja, que veio de mim, a assistente)
        JsonArray results = doc["result"].as<JsonArray>();
        String lastResponse = "";

        for (auto msg : results) {
            int updateId = msg["update_id"] | 0;
            if (updateId > tg_last_update_id) {
                tg_last_update_id = updateId;

                // Verifica se a mensagem tem texto e NAO foi enviada pelo bot
                JsonObject message = msg["message"];
                if (!message.isNull()) {
                    bool fromBot = message["from"]["is_bot"] | false;
                    // Soh pega mensagens de humanos (que sao minhas respostas)
                    if (!fromBot) {
                        const char* text = message["text"] | "";
                        if (strlen(text) > 0) {
                            lastResponse = text;
                        }
                    }
                }
            }
        }

        return lastResponse;
    }

    //-----------------------------------------------------------------
    // Envia arquivo de audio pro Telegram
    // (usado para enviar gravacao do microfone)
    //-----------------------------------------------------------------
    bool sendAudio(const uint8_t* data, size_t len) {
        if (!_ready || data == nullptr || len == 0) return false;

        WiFiClientSecure client;
        client.setInsecure();
        if (!client.connect(TG_API, 443)) return false;

        // Multipart form-data pra enviar audio
        String boundary = "----TGNeonBoundary";
        String header = "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + _chatId + "\r\n"
            "--" + boundary + "\r\n"
            "Content-Disposition: form-data; name=\"audio\"; filename=\"audio.ogg\"\r\n"
            "Content-Type: audio/ogg\r\n\r\n";
        String footer = "\r\n--" + boundary + "--\r\n";

        size_t bodySize = header.length() + len + footer.length();

        client.println("POST /bot" + _token + "/sendAudio HTTP/1.1");
        client.println("Host: " + String(TG_API));
        client.println("Content-Type: multipart/form-data; boundary=" + boundary);
        client.println("Content-Length: " + String(bodySize));
        client.println("Connection: close");
        client.println();
        client.print(header);

        size_t sent = 0;
        while (sent < len) {
            size_t chunk = min((size_t)1024, len - sent);
            client.write(data + sent, chunk);
            sent += chunk;
        }

        client.print(footer);

        uint32_t timeout = millis() + 10000;
        while (!client.available() && millis() < timeout) delay(10);
        client.stop();

        Serial.println("[TG] Audio enviado");
        return true;
    }

private:
    // Escapa caracteres especiais pra JSON
    String jsonEscape(const String& s) {
        String out;
        for (size_t i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        return out;
    }
};

#endif // TELEGRAMCLIENT_H
