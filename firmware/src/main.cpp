/**
 * Neon Widget — M5CardPuter
 * 
 * Widget físico conectado à assistente Neon (OpenClaw VPS).
 * 
 * Funcionalidades:
 * - Avatar com emoções (Neon 👻)
 * - Menu: Dia, Chat, Config, Sobre
 * - Push-to-talk com microfone I2S → VPS → Whisper → resposta
 * - Feedback sonoro com voz da Neon (TTS)
 * - Sleep automático para economia de bateria
 * - WiFi auto-connect com lista de redes
 */

#include <M5Cardputer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "Config.h"
#include "Display.h"
#include "AudioManager.h"
#include "Network.h"
#include "UI.h"
#include "Avatar.h"
#include "Version.h"
#include "OTAUpdate.h"

// ============================================================
// Constantes
// ============================================================
#define SLEEP_TIMEOUT_MS      30000   // 30s idle → sleep tela
#define DEEP_SLEEP_TIMEOUT_MS 300000  // 5min idle → deep sleep
#define POLL_INTERVAL_MS      5000    // ping VPS a cada 5s
#define AUDIO_SAMPLE_RATE     16000
#define AUDIO_BITS            16
#define AUDIO_CHANNELS        1

// ============================================================
// Globais
// ============================================================
ConfigManager   config;
DisplayManager  display(&config);
AudioManager    audio(AUDIO_SAMPLE_RATE, AUDIO_BITS, AUDIO_CHANNELS);
NetworkManager  network(&config);
UIManager       ui(&display, &config);
Avatar          avatar(&display);
OTAUpdateManager ota;

// Timers
static uint32_t lastActivity = 0;
static uint32_t lastPoll = 0;
static bool     sleeping = false;

// Estados de áudio
enum AudioState {
    AUDIO_IDLE,
    AUDIO_RECORDING,
    AUDIO_PROCESSING,
    AUDIO_PLAYING
};
static AudioState audioState = AUDIO_IDLE;

// Buffer de áudio (PSRAM)
static uint8_t* audioBuffer = nullptr;
static size_t   audioBufferSize = 0;

// ============================================================
// Protótipos
// ============================================================
void setupWiFi();
void handleKeys();
void pushToTalk();
void sendAudioToVPS();
void pollServer();
void resetSleepTimer();
void goToSleep();
void loadConfig();
void saveConfig();

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== Neon Widget Boot ===");
    
    // Inicializa hardware
    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);
    
    // Semente para random() (frases, etc)
    randomSeed(esp_random());
    
    // Monta SPIFFS (config, assets)
    if (!SPIFFS.begin(true)) {
        Serial.println("❌ SPIFFS mount failed!");
    }
    
    // Carrega configurações
    loadConfig();
    
    // Inicializa módulos
    display.init();
    audio.init();
    
    // Tela de boot
    display.showSplash();
    delay(2000);
    
    // Conecta WiFi
    display.showStatus("WiFi...");
    setupWiFi();
    
    if (WiFi.isConnected()) {
        display.showStatus("Conectado! ✓");
        delay(500);
        
        // Verifica atualizações OTA
        display.showStatus("Verificando atualizacoes...");
        ota.setProgressCallback([](int progress, int total) {
            char buf[32];
            snprintf(buf, sizeof(buf), "OTA: %d%%", progress);
            M5Cardputer.Display.fillRect(0, 105, 240, 30, TFT_BLACK);
            M5Cardputer.Display.setCursor(5, 110);
            M5Cardputer.Display.printf("OTA: %d%%", progress);
            
            // Barra de progresso
            int barWidth = (progress * 200) / total;
            M5Cardputer.Display.drawRect(20, 125, 200, 8, TFT_CYAN);
            M5Cardputer.Display.fillRect(22, 127, barWidth, 4, TFT_GREEN);
        });
        
        if (ota.checkForUpdate()) {
            display.showStatus(("Nova versao: " + ota.getLatestTag()).c_str());
            delay(2000);
            
            display.showStatus("Atualizando firmware...");
            if (!ota.applyUpdate()) {
                display.showStatus("OTA falhou!");
                delay(2000);
            }
        } else {
            Serial.println("[OTA] Firmware atualizado");
        }
        
        avatar.setEmotion(AVATAR_HAPPY);
        delay(500);
    } else {
        display.showStatus("WiFi: off");
        avatar.setEmotion(AVATAR_SAD);
    }
    
    // Tela inicial — Neón no estado idle
    ui.setScreen(SCREEN_IDLE);
    resetSleepTimer();
    
    Serial.println("✅ Neon Widget ready!");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    M5Cardputer.update();
    
    // 1. Input handling
    handleKeys();
    
    // 2. Push-to-talk (botão físico do CardPuter)
    static bool btnWasPressed = false;
    if (M5Cardputer.BtnA.isPressed() && !btnWasPressed) {
        btnWasPressed = true;
        pushToTalk();
    } else if (!M5Cardputer.BtnA.isPressed() && btnWasPressed) {
        btnWasPressed = false;
    }
    
    // 3. Polling do servidor (notificações)
    if (WiFi.isConnected() && millis() - lastPoll > POLL_INTERVAL_MS) {
        lastPoll = millis();
        pollServer();
    }
    
    // 4. Áudio em andamento
    if (audioState == AUDIO_RECORDING) {
        audio.recordChunk();
    }
    if (audioState == AUDIO_PLAYING) {
        if (!audio.isPlaying()) {
            audioState = AUDIO_IDLE;
            avatar.setEmotion(AVATAR_IDLE);
        }
    }
    
    // 5. Sleep timer
    if (!sleeping && millis() - lastActivity > DEEP_SLEEP_TIMEOUT_MS) {
        goToSleep();
    } else if (sleeping && millis() - lastActivity < SLEEP_TIMEOUT_MS) {
        sleeping = false;
        display.wake();
    }
    
    // 6. Reconexão WiFi automática
    network.update();
    
    // 7. Atualiza UI
    ui.update();
    avatar.update();
    
    delay(10); // Evita watchdog
}

// ============================================================
// WIFI
// ============================================================
void setupWiFi() {
    auto& networks = config.getWiFiNetworks();
    
    if (networks.empty()) {
        Serial.println("⚠️ Nenhuma WiFi configurada!");
        display.showStatus("Config WiFi...");
        // Inicia WiFi Manager para config
        WiFiManager wm;
        wm.setConfigPortalTimeout(180);
        if (wm.autoConnect("Neon-Widget")) {
            // Salvou nova rede
            config.addWiFi(WiFi.SSID().c_str(), WiFi.psk().c_str());
            saveConfig();
        }
        return;
    }
    
    // Tenta redes conhecidas
    for (auto& net : networks) {
        Serial.printf("Conectando %s...\n", net.ssid);
        WiFi.begin(net.ssid, net.password);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.isConnected()) {
            Serial.printf("✅ WiFi: %s (%s)\n", net.ssid, WiFi.localIP().toString().c_str());
            return;
        }
    }
    
    Serial.println("❌ Nenhuma WiFi conhecida encontrada!");
}

// ============================================================
// INPUT
// ============================================================
void handleKeys() {
    if (M5Cardputer.Keyboard.isChange()) {
        auto key = M5Cardputer.Keyboard.key();
        
        if (key.state == KEY_PRESSED) {
            resetSleepTimer();
            
            uint8_t k = key.key_code;
            
            // ESC → volta / sai do menu
            if (k == KEY_ESC) {
                ui.goBack();
                return;
            }
            
            // Se estamos no menu, passa a tecla pra UI
            if (ui.getScreen() != SCREEN_IDLE) {
                ui.handleKey(k);
            } else {
                // Modo idle: qualquer tecla abre menu
                if (k == KEY_ENTER || k == ' ') {
                    ui.setScreen(SCREEN_MENU);
                }
            }
        }
    }
}

// ============================================================
// PUSH-TO-TALK
// ============================================================
void pushToTalk() {
    if (audioState != AUDIO_IDLE) return;
    
    resetSleepTimer();
    audioState = AUDIO_RECORDING;
    avatar.setEmotion(AVATAR_LISTENING);
    display.showRecording(true);
    
    Serial.println("🎤 Gravando...");
    
    // Aloca buffer na PSRAM
    audioBuffer = (uint8_t*)ps_malloc(512 * 1024);  // 512KB
    if (!audioBuffer) {
        Serial.println("❌ Sem PSRAM!");
        audioState = AUDIO_IDLE;
        display.showRecording(false);
        avatar.setEmotion(AVATAR_ERROR);
        display.showStatus("Sem memoria");
        return;
    }
    audioBufferSize = 0;
    
    audio.startRecording(&audioBuffer, &audioBufferSize);
    
    // Aguarda botão soltar (loop blocking curto)
    uint32_t recordStart = millis();
    while (M5Cardputer.BtnA.isPressed() && (millis() - recordStart < 10000)) {
        M5Cardputer.update();
        audio.recordChunk();
        delay(5);
    }
    
    // Finaliza gravação
    audio.stopRecording();
    audioState = AUDIO_PROCESSING;
    display.showRecording(false);
    avatar.setEmotion(AVATAR_THINKING);
    display.showStatus("Processando...");
    
    Serial.printf("🎤 Gravado: %u bytes\n", audioBufferSize);
    
    // Envia pra VPS
    sendAudioToVPS();
    
    // Libera buffer
    if (audioBuffer) {
        free(audioBuffer);
        audioBuffer = nullptr;
    }
}

// ============================================================
// ENVIA ÁUDIO + RECEBE RESPOSTA
// ============================================================
void sendAudioToVPS() {
    if (!WiFi.isConnected()) {
        avatar.setEmotion(AVATAR_ERROR);
        display.showStatus("Sem WiFi!");
        audioState = AUDIO_IDLE;
        return;
    }
    
    WiFiClient client; // HTTP (bridge roda na porta 8080 sem HTTPS)
    
    const char* host = config.getServerHost();
    int port = config.getServerPort();
    
    if (!client.connect(host, port)) {
        Serial.println("❌ Conexão falhou!");
        avatar.setEmotion(AVATAR_ERROR);
        display.showStatus("Erro conexao");
        audioState = AUDIO_IDLE;
        return;
    }
    
    // WAV header + buffer de áudio
    size_t wavSize = audioBufferSize + 44;
    uint8_t* wavData = (uint8_t*)ps_malloc(wavSize);
    if (!wavData) {
        Serial.println("❌ Sem PSRAM para WAV");
        client.stop();
        audioState = AUDIO_IDLE;
        return;
    }
    
    // Monta header WAV
    AudioManager::buildWavHeader(wavData, wavSize, audioBufferSize, AUDIO_SAMPLE_RATE);
    memcpy(wavData + 44, audioBuffer, audioBufferSize);
    
    // HTTP multipart
    String boundary = "----NeonAudioBoundary";
    String bodyStart = "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"audio\"; filename=\"recording.wav\"\r\n"
        "Content-Type: audio/wav\r\n\r\n";
    String bodyEnd = "\r\n--" + boundary + "--\r\n";
    
    size_t contentLength = bodyStart.length() + wavSize + bodyEnd.length();
    
    client.println(String("POST ") + config.getAudioEndpoint() + " HTTP/1.1");
    client.println(String("Host: ") + host);
    client.println(String("Content-Type: multipart/form-data; boundary=") + boundary);
    client.println(String("Content-Length: ") + contentLength);
    client.println("Connection: close");
    client.println();
    client.print(bodyStart);
    
    // Envia WAV completo
    size_t sent = 0;
    while (sent < wavSize) {
        size_t chunk = min((size_t)1024, wavSize - sent);
        client.write(wavData + sent, chunk);
        sent += chunk;
    }
    
    free(wavData);
    client.print(bodyEnd);
    
    // Aguarda resposta
    uint32_t timeout = millis() + 15000;
    while (!client.available() && millis() < timeout) {
        delay(10);
    }
    
    // Lê resposta
    String response;
    while (client.available()) {
        response += client.readString();
    }
    client.stop();
    
    // Extrai JSON do body
    int jsonStart = response.indexOf('{');
    int jsonEnd = response.lastIndexOf('}');
    
    if (jsonStart >= 0 && jsonEnd > jsonStart) {
        String jsonBody = response.substring(jsonStart, jsonEnd + 1);
        Serial.printf("Resposta: %s\n", jsonBody.c_str());
        
        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonBody);
        
        if (!error) {
            const char* text = doc["text"] | "(sem resposta)";
            const char* emotion = doc["emotion"] | "idle";
            const char* ttsUrl = doc["tts_url"] | "";
            
            // Atualiza avatar
            avatar.setEmotionByName(emotion);
            
            // Mostra texto na tela
            display.showText(text);
            
            // Toca TTS se tiver URL
            if (strlen(ttsUrl) > 0 && config.isSoundEnabled()) {
                audioState = AUDIO_PLAYING;
                audio.playURL(ttsUrl);
            } else {
                audioState = AUDIO_IDLE;
            }
        } else {
            Serial.println("❌ Erro parse JSON");
            avatar.setEmotion(AVATAR_ERROR);
            audioState = AUDIO_IDLE;
        }
    } else {
        Serial.println("❌ Resposta inválida");
        avatar.setEmotion(AVATAR_ERROR);
        audioState = AUDIO_IDLE;
    }
}

// ============================================================
// POLLING
// ============================================================
void pollServer() {
    // Verifica se há notificações novas na VPS
    WiFiClient client;
    
    if (!client.connect(config.getServerHost(), config.getServerPort())) {
        return;
    }
    
    client.println(String("GET ") + config.getPollEndpoint() + " HTTP/1.1");
    client.println(String("Host: ") + config.getServerHost());
    client.println("Connection: close");
    client.println();
    
    uint32_t timeout = millis() + 5000;
    String response;
    while (!client.available() && millis() < timeout) {
        delay(10);
    }
    while (client.available()) {
        response += client.readString();
    }
    client.stop();
    
    int jsonStart = response.indexOf('{');
    int jsonEnd = response.lastIndexOf('}');
    
    if (jsonStart >= 0 && jsonEnd > jsonStart) {
        String jsonBody = response.substring(jsonStart, jsonEnd + 1);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonBody);
        
        if (!error) {
            bool hasNotification = doc["notify"] | false;
            if (hasNotification) {
                const char* text = doc["text"] | "";
                const char* emotion = doc["emotion"] | "surprised";
                
                // Acorda display
                if (sleeping) {
                    display.wake();
                    sleeping = false;
                }
                
                resetSleepTimer();
                avatar.setEmotionByName(emotion);
                
                if (strlen(text) > 0) {
                    display.showNotification(text);
                }
                
                // Se tiver TTS, toca
                const char* ttsUrl = doc["tts_url"] | "";
                if (strlen(ttsUrl) > 0 && config.isSoundEnabled()) {
                    audioState = AUDIO_PLAYING;
                    audio.playURL(ttsUrl);
                }
                
                Serial.printf("📬 Notificação: %s\n", text);
            }
        }
    }
}

// ============================================================
// UTILITÁRIOS
// ============================================================
void resetSleepTimer() {
    lastActivity = millis();
    if (sleeping) {
        sleeping = false;
        display.wake();
    }
}

void goToSleep() {
    Serial.println("💤 Deep sleep...");
    sleeping = true;
    avatar.setEmotion(AVATAR_SLEEP);
    display.sleep();
    
    // Configura wake por botão
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW); // Botão físico
    
    delay(100);
    esp_deep_sleep_start();
}

// ============================================================
// CONFIG (SPIFFS)
// ============================================================
void loadConfig() {
    if (!SPIFFS.exists("/config.json")) {
        Serial.println("⚠️ config.json não encontrado, usando defaults");
        config.setDefaults();
        saveConfig();
        return;
    }
    
    File file = SPIFFS.open("/config.json", "r");
    if (!file) return;
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.println("❌ Erro lendo config.json");
        return;
    }
    
    config.load(doc);
    Serial.println("✅ Config carregada");
}

void saveConfig() {
    File file = SPIFFS.open("/config.json", "w");
    if (!file) {
        Serial.println("❌ Erro salvando config.json");
        return;
    }
    
    JsonDocument doc;
    config.save(doc);
    serializeJson(doc, file);
    file.close();
    Serial.println("✅ Config salva");
}
