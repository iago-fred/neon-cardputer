/*=============================================================================
 *  NEON WIDGET — Firmware para M5CardPuter
 *  
 *  Um widget fisico que se conecta a sua VPS para interagir com a assistente
 *  Neon (eu!). Funciona como um terminal portatil com tela, teclado,
 *  microfone e auto-falante.
 *  
 *  Hardware: M5CardPuter (ESP32-S3, 8MB Flash, PSRAM)
 *  Board Manager: M5Stack 3.2.2+
 *============================================================================*/
//=============================================================================
// BIBLIOTECAS
//=============================================================================
// M5Cardputer.h — biblioteca oficial do hardware (tela, teclado, speaker, etc)
#include <M5Cardputer.h>
// Network.h — necessario no Core 3.x pro WiFi funcionar (tipos de rede)
#include <Network.h>
// WiFi.h — controle de conexao WiFi (conectar, desconectar, status)
#include <WiFi.h>
// WiFiClient.h — cliente HTTP pra enviar dados pro servidor
#include <WiFiClient.h>
// ArduinoJson.h — ler/escrever JSON (config, respostas do servidor)
#include <ArduinoJson.h>
// SPIFFS.h — memoria interna do ESP32 (armazena config se nao tiver SD)
#include <SPIFFS.h>
// SD.h — leitura do cartao microSD (sprites, config)
#include <SD.h>
// SPI.h — protocolo de comunicacao com o SD card
#include <SPI.h>

//=============================================================================
// ARQUIVOS DO PROJETO (headers .h)
//=============================================================================
#include "Config.h"       // Gerenciamento de configuracoes (WiFi, servidor)
#include "Display.h"      // Controle da tela (texto, notificacoes)
#include "AudioManager.h" // Microfone + auto-falante (stub por enquanto)
#include "UI.h"           // Interface do usuario (menu, telas)
#include "SpriteManager.h"// Carregamento de sprites PNG do SD card
#include "Avatar.h"       // Desenho da Neon (com ou sem sprite)
#include "Version.h"      // Numero da versao do firmware
#include "OTAUpdate.h"    // Atualizacao automatica pelo GitHub

//=============================================================================
// PINOS DO SD CARD (segundo a documentacao oficial do M5CardPuter)
//=============================================================================
// CS   = GPIO 12 (selecao do chip)
// MOSI = GPIO 14 (dados do ESP32 pro SD)
// MISO = GPIO 40 (dados do SD pro ESP32)
// SCK  = GPIO 39 (clock)
#define SD_CS    GPIO_NUM_12
#define SD_MOSI  GPIO_NUM_14
#define SD_MISO  GPIO_NUM_40
#define SD_SCK   GPIO_NUM_39
// Caminho do arquivo de configuracao no SD card
#define SD_CONFIG_PATH "/neon/config.json"

//=============================================================================
// CONSTANTES GLOBAIS
//=============================================================================
// SLEEP_TIMEOUT_MS: tempo sem atividade ate apagar a tela (30 segundos)
#define SLEEP_TIMEOUT_MS      30000
// DEEP_SLEEP_TIMEOUT_MS: tempo ate desligar completamente (5 minutos)
#define DEEP_SLEEP_TIMEOUT_MS 300000
// POLL_INTERVAL_MS: a cada quantos ms o CardPuter pergunta se tem novidades (5s)
#define POLL_INTERVAL_MS      5000
// AUDIO_SAMPLE_RATE: qualidade do audio gravado (16000 Hz = qualidade de telefone)
#define AUDIO_SAMPLE_RATE     16000
// AUDIO_BITS: profundidade do audio (16 bits = CD quality)
#define AUDIO_BITS            16
// AUDIO_CHANNELS: mono (1) ou stereo (2) — mono pra economizar espaco
#define AUDIO_CHANNELS        1

//=============================================================================
// OBJETOS GLOBAIS
//=============================================================================
// Cada "objeto" aqui e um modulo do sistema. Eles sao criados uma vez e
// usados pelo resto do programa.
ConfigManager   config;       // Configuracoes (WiFi, servidor, som, brilho)
SpriteManager   sprites;      // Gerenciador de sprites (PNGs no SD)
DisplayManager  display(&config);  // Controle da tela
AudioManager    audio(AUDIO_SAMPLE_RATE, AUDIO_BITS, AUDIO_CHANNELS); // Audio
UIManager       ui(&display, &config);  // Interface grafica (menus)
Avatar          avatar(&display, &sprites);  // Desenho da Neon
OTAUpdateManager ota;         // Atualizacao OTA (Over-The-Air)

//=============================================================================
// VARIAVEIS GLOBAIS (estado do sistema)
//=============================================================================
// lastActivity: quando foi a ultima vez que o usuario apertou uma tecla
static uint32_t lastActivity = 0;
// lastPoll: quando foi a ultima vez que perguntamos ao servidor se tem novidades
static uint32_t lastPoll = 0;
// sleeping: true quando a tela esta apagada / em modo de economia
static bool     sleeping = false;

// --- Controle do audio ---
// AudioState: em que etapa do audio estamos
enum AudioState {
    AUDIO_IDLE,       // Parado, esperando
    AUDIO_RECORDING,  // Gravando audio do microfone
    AUDIO_PROCESSING, // Enviando pro servidor e esperando resposta
    AUDIO_PLAYING     // Tocando resposta em audio (TTS)
};
static AudioState audioState = AUDIO_IDLE;

// audioBuffer: onde o audio gravado fica armazenado (na PSRAM)
static uint8_t* audioBuffer = nullptr;
// audioBufferSize: quantos bytes foram gravados
static size_t   audioBufferSize = 0;

//=============================================================================
// PROTOTIPOS DAS FUNCOES
//=============================================================================
// (Declaracoes pra que o compilador saiba que essas funcoes existem)
void setupWiFi();       // Conecta nas redes WiFi configuradas
void handleKeys();      // Le o teclado e executa acoes
void pushToTalk();      // Inicia a gravacao de audio
void sendAudioToVPS();  // Envia o audio pro servidor e mostra resposta
void pollServer();      // Pergunta ao servidor se tem notificacoes
void resetSleepTimer(); // Reinicia o contador de tempo ocioso
void goToSleep();       // Desliga o aparelho pra economizar bateria
void loadConfig();      // Carrega config do SD card ou SPIFFS
void saveConfig();      // Salva config no SPIFFS

//=============================================================================
// SETUP — roda UMA vez quando o aparelho liga
//=============================================================================
void setup() {
    // Inicia comunicacao serial (pro computador ler mensagens de debug)
    Serial.begin(115200);
    Serial.println("\n\n=== Neon Widget Boot ===");
    
    // --- Inicializa o hardware ---
    // M5Cardputer.begin() liga todos os perifericos (tela, teclado, speaker)
    M5Cardputer.begin();
    // Gira a tela pra orientacao correta (retrato)
    M5Cardputer.Display.setRotation(1);
    
    // --- Sorteio pra frases aleatorias ---
    // esp_random() pega um numero aleatorio real do hardware
    // randomSeed() usa esse numero pra "misturar" as frases sorteadas
    randomSeed(esp_random());
    
    // --- Monta o SD card ---
    // SPI.begin() configura os pinos de comunicacao do SD
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    // SD.begin() tenta montar o cartao. Se nao tiver SD, tudo bem
    if (!SD.begin(SD_CS, SPI)) {
        Serial.println("[SD] SD card nao encontrado, usando SPIFFS");
    } else {
        Serial.println("[SD] SD card montado");
    }
    
    // --- Monta a memoria interna (SPIFFS) ---
    // Se nao tiver SD, usamos a memoria interna do ESP32
    if (!SPIFFS.begin(true)) {
        Serial.println("[SPIFFS] SPIFFS mount failed!");
    }
    
    // --- Carrega as configuracoes ---
    // Prioridade: SD card > SPIFFS > valores padrao
    loadConfig();
    
    // --- Inicializa os sprites (PNGs do avatar) ---
    // Procura a pasta /neon/sprites/ no SD card
    sprites.init();
    
    // --- Inicializa os modulos ---
    display.init();   // Prepara a tela (configura fontes, etc)
    audio.init();     // Prepara o microfone e speaker
    
    // --- Tela de boot (splash) ---
    // Mostra "Neon" com uma barra de progresso na tela
    display.showSplash();
    delay(2000);  // Espera 2 segundos pra voce ver a tela inicial
    
    // --- Conecta no WiFi ---
    display.showStatus("WiFi...");
    setupWiFi();
    
    // --- Verifica atualizacoes e finaliza boot ---
    if (WiFi.isConnected()) {
        display.showStatus("Conectado!");
        delay(500);
        
        // --- Verifica se tem versao nova do firmware (OTA) ---
        display.showStatus("Verificando atualizacoes...");
        // Configura uma funcao que mostra o progresso do download na tela
        ota.setProgressCallback([](int progress, int total) {
            M5Cardputer.Display.fillRect(0, 105, 240, 30, TFT_BLACK);
            M5Cardputer.Display.setCursor(5, 110);
            M5Cardputer.Display.printf("OTA: %d%%", progress);
            int barWidth = (progress * 200) / total;
            M5Cardputer.Display.drawRect(20, 125, 200, 8, TFT_CYAN);
            M5Cardputer.Display.fillRect(22, 127, barWidth, 4, TFT_GREEN);
        });
        
        // Verifica se ha atualizacao disponivel no GitHub
        if (ota.checkForUpdate()) {
            display.showStatus(("Nova versao: " + ota.getLatestTag()).c_str());
            delay(2000);
            display.showStatus("Atualizando firmware...");
            // Se falhar, mostra mensagem de erro
            if (!ota.applyUpdate()) {
                display.showStatus("OTA falhou!");
                delay(2000);
            }
        }
        
        // --- Boot concluido: Neon esta ativa! ---
        avatar.setEmotion(AVATAR_LISTENING);  // Mostra Neon ouvindo
        delay(500);
    } else {
        // Sem WiFi: mostra aviso na tela
        display.showStatus("WiFi: off");
        avatar.setEmotion(AVATAR_IDLE);
    }
    
    // --- Tela inicial ---
    ui.setScreen(SCREEN_IDLE);   // Modo "descanso" - mostra o avatar
    resetSleepTimer();           // Inicia o contador de tempo ocioso
    
    Serial.println("Neon Widget ready!");  // Avisa no terminal que ligou
}

//=============================================================================
// LOOP — roda SEM PARAR, varias vezes por segundo
//=============================================================================
void loop() {
    // M5Cardputer.update() atualiza o estado dos botoes e teclado
    M5Cardputer.update();
    
    // 1. Le o teclado (deteccao de teclas pressionadas)
    handleKeys();
    
    // 2. Botao de acao (BtnA) — gravacao de audio
    // btnWasPressed evita ler o botao varias vezes enquanto segurado
    static bool btnWasPressed = false;
    if (M5Cardputer.BtnA.isPressed() && !btnWasPressed) {
        btnWasPressed = true;
        pushToTalk();  // Comeca a gravar
    } else if (!M5Cardputer.BtnA.isPressed() && btnWasPressed) {
        btnWasPressed = false;  // Botao foi solto
    }
    
    // 3. Polling do servidor (pergunta se tem novidades)
    if (WiFi.isConnected() && millis() - lastPoll > POLL_INTERVAL_MS) {
        lastPoll = millis();
        pollServer();
    }
    
    // 4. Se esta gravando, continua gravando chunk por chunk
    if (audioState == AUDIO_RECORDING) audio.recordChunk();
    // Se terminou de tocar audio, volta ao normal
    if (audioState == AUDIO_PLAYING && !audio.isPlaying()) {
        audioState = AUDIO_IDLE;
        avatar.setEmotion(AVATAR_IDLE);
    }
    
    // 5. Timer de economia (sleep)
    // Se passou 5 minutos sem atividade, desliga
    if (!sleeping && millis() - lastActivity > DEEP_SLEEP_TIMEOUT_MS) {
        goToSleep();
    } else if (sleeping && millis() - lastActivity < SLEEP_TIMEOUT_MS) {
        // Se atividade voltou, acorda a tela
        sleeping = false;
        display.wake();
    }
    
    // 6. Reconecta WiFi automaticamente se caiu (a cada 30 segundos)
    static uint32_t lastReconnect = 0;
    if (WiFi.status() != WL_CONNECTED && millis() - lastReconnect > 30000) {
        lastReconnect = millis();
        WiFi.reconnect();
    }
    
    // 7. Atualiza a UI (interface) e o Avatar (desenho da Neon)
    ui.update();
    avatar.update();
    
    // 8. Pequena pausa pra nao sobrecarregar o processador
    delay(10);
}

//=============================================================================
// TECLADO — Le as teclas pressionadas e executa acoes
//=============================================================================
// A API do teclado no M5CardPuter funciona assim:
//   - isChange()  : detecta QUANDO uma tecla muda de estado
//   - isPressed() : quantas teclas estao pressionadas agora
//   - keysState() : retorna um objeto com:
//       .word    : array de caracteres digitados
//       .del     : true se DEL foi pressionado
//       .enter   : true se ENTER foi pressionado
//   - status.word : contem as letras que voce digitou (ex: 'w', 's', ' ')
void handleKeys() {
    // Se nenhuma tecla mudou de estado, sai da funcao
    if (!M5Cardputer.Keyboard.isChange()) return;
    
    // Pega o estado atual do teclado
    auto status = M5Cardputer.Keyboard.keysState();
    // Se nenhuma tecla esta pressionada, sai
    if (M5Cardputer.Keyboard.isPressed() == 0) return;
    
    // Reinicia o timer de ociosidade (usuario interagiu)
    resetSleepTimer();
    
    // --- Alterna o layout da tela ---
    // Quando estamos numa tela de conteudo (menu/config), o avatar
    // fica menor no lado esquerdo e o conteudo no lado direito.
    // No modo idle, o avatar fica grande no centro.
    if (ui.getScreen() != SCREEN_IDLE) {
        avatar.setLayout(AVATAR_LAYOUT_SPLIT);
    } else {
        avatar.setLayout(AVATAR_LAYOUT_FULL);
    }
    
    // --- Tecla DEL = voltar ---
    if (status.del) { ui.goBack(); return; }
    
    // --- Tecla ENTER = selecionar / avancar ---
    if (status.enter) {
        if (ui.getScreen() == SCREEN_IDLE) {
            ui.setScreen(SCREEN_MENU);        // Abre o menu
        } else if (ui.getScreen() == SCREEN_MENU) {
            ui.selectCurrent();               // Seleciona item
        } else {
            ui.goBack();                      // Volta
        }
        return;
    }
    
    // --- Teclas de navegacao ---
    // O for loop percorre cada caractere que foi digitado
    // W/S = navegar para cima/baixo no menu
    // ESPACO = abrir menu / selecionar
    for (auto i : status.word) {
        if (i == 'w' || i == 'W') { ui.cycleMenuPrev(); }        // Cima
        else if (i == 's' || i == 'S') { ui.cycleMenu(); }       // Baixo
        else if (i == ' ') {
            if (ui.getScreen() == SCREEN_IDLE) {
                ui.setScreen(SCREEN_MENU);
            } else if (ui.getScreen() == SCREEN_MENU) {
                ui.selectCurrent();
            }
        }
    }
}

//=============================================================================
// WIFI — Conecta nas redes salvas na configuracao
//=============================================================================
void setupWiFi() {
    // Pega a lista de redes WiFi da configuracao
    auto& networks = config.getWiFiNetworks();
    
    // Se nao tem nenhuma rede configurada, avisa na tela
    if (networks.empty()) {
        Serial.println("Nenhuma WiFi configurada!");
        display.showStatus("Configure o SD card");
        // O usuario precisa criar um config.json no SD card
        return;
    }
    
    // Tenta cada rede da lista ate conectar em alguma
    for (auto& net : networks) {
        Serial.printf("Conectando %s...\n", net.ssid);
        WiFi.begin(net.ssid, net.password);  // Tenta conectar
        
        // Espera ate 10 segundos (20 tentativas de 500ms)
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
            Serial.print(".");
        }
        Serial.println();
        
        // Se conectou, sai da funcao
        if (WiFi.isConnected()) {
            Serial.printf("WiFi: %s (%s)\n", net.ssid, WiFi.localIP().toString().c_str());
            return;
        }
    }
    
    // Nenhuma rede funcionou
    Serial.println("Nenhuma WiFi conhecida encontrada!");
}

//=============================================================================
// PUSH-TO-TALK — Grava audio quando segura o botao
//=============================================================================
void pushToTalk() {
    // Se ja estiver gravando/tocando, ignora
    if (audioState != AUDIO_IDLE) return;
    
    resetSleepTimer();
    audioState = AUDIO_RECORDING;
    avatar.setEmotion(AVATAR_LISTENING);  // Neon "ouvindo"
    display.showRecording(true);           // Mostra "GRAVANDO" na tela
    
    Serial.println("Gravando...");
    
    // --- Aloca memoria pro audio (PSRAM = memoria extra) ---
    // 512KB da pra uns 30 segundos de audio
    audioBuffer = (uint8_t*)ps_malloc(512 * 1024);
    if (!audioBuffer) {
        // Se nao tem memoria, avisa e desiste
        audioState = AUDIO_IDLE;
        display.showRecording(false);
        display.showStatus("Sem memoria");
        avatar.setEmotion(AVATAR_ERROR);
        return;
    }
    audioBufferSize = 0;
    
    // Inicia a gravacao (o AudioManager guarda o buffer)
    audio.startRecording(&audioBuffer, &audioBufferSize);
    
    // --- Aguarda o usuario soltar o botao ---
    // O loop fica aqui enquanto o botao estiver pressionado
    // (maximo 10 segundos)
    uint32_t recordStart = millis();
    while (M5Cardputer.BtnA.isPressed() && (millis() - recordStart < 10000)) {
        M5Cardputer.update();
        audio.recordChunk();  // Le mais dados do microfone
        delay(5);
    }
    
    // --- Finaliza a gravacao ---
    audio.stopRecording();
    audioState = AUDIO_PROCESSING;
    display.showRecording(false);
    avatar.setEmotion(AVATAR_THINKING);  // Neon "pensando" (processando)
    display.showStatus("Processando...");
    
    Serial.printf("Gravado: %u bytes\n", audioBufferSize);
    
    // --- Envia pra VPS se gravou alguma coisa ---
    if (audioBufferSize > 100) {
        sendAudioToVPS();
    } else {
        display.showStatus("Nada gravado");
        avatar.setEmotion(AVATAR_IDLE);
        audioState = AUDIO_IDLE;
    }
    
    // --- Libera a memoria do audio ---
    if (audioBuffer) {
        free(audioBuffer);
        audioBuffer = nullptr;
    }
}

//=============================================================================
// ENVIA AUDIO PRA VPS — Manda o audio pro servidor e recebe resposta
//=============================================================================
void sendAudioToVPS() {
    // Verifica se esta conectado ao WiFi
    if (!WiFi.isConnected()) {
        avatar.setEmotion(AVATAR_ERROR);
        display.showStatus("Sem WiFi!");
        audioState = AUDIO_IDLE;
        return;
    }
    
    // Cria um cliente HTTP (conexao com o servidor)
    WiFiClient client;
    const char* host = config.getServerHost();  // IP da VPS
    int port = config.getServerPort();          // Porta (8080)
    
    // Tenta conectar no servidor
    if (!client.connect(host, port)) {
        avatar.setEmotion(AVATAR_ERROR);
        display.showStatus("Erro conexao");
        audioState = AUDIO_IDLE;
        return;
    }
    
    // --- Cria o arquivo WAV (audio + cabecalho) ---
    // Um arquivo WAV tem 44 bytes de cabecalho + dados de audio
    size_t wavSize = audioBufferSize + 44;
    uint8_t* wavData = (uint8_t*)ps_malloc(wavSize);
    if (!wavData) {
        client.stop();
        audioState = AUDIO_IDLE;
        return;
    }
    
    // Monta o cabecalho WAV (diz que e audio PCM 16000Hz mono 16bit)
    AudioManager::buildWavHeader(wavData, wavSize, audioBufferSize, AUDIO_SAMPLE_RATE);
    // Copia os dados de audio pra depois do cabecalho
    memcpy(wavData + 44, audioBuffer, audioBufferSize);
    
    // --- Monta a requisicao HTTP multipart ---
    // Multipart e um formato que permite enviar arquivos junto com a requisicao
    String boundary = "----NeonAudioBoundary";
    String bodyStart = "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"audio\"; filename=\"recording.wav\"\r\n"
        "Content-Type: audio/wav\r\n\r\n";
    String bodyEnd = "\r\n--" + boundary + "--\r\n";
    size_t contentLength = bodyStart.length() + wavSize + bodyEnd.length();
    
    // Envia os cabecalhos HTTP
    client.println(String("POST ") + config.getAudioEndpoint() + " HTTP/1.1");
    client.println(String("Host: ") + host);
    client.println(String("Content-Type: multipart/form-data; boundary=") + boundary);
    client.println(String("Content-Length: ") + contentLength);
    client.println("Connection: close");
    client.println();
    
    // Envia o cabecalho multipart + audio + fechamento
    client.print(bodyStart);
    size_t sent = 0;
    while (sent < wavSize) {
        size_t chunk = min((size_t)1024, wavSize - sent);
        client.write(wavData + sent, chunk);
        sent += chunk;
    }
    free(wavData);  // Libera memoria do WAV
    client.print(bodyEnd);
    
    // --- Aguarda a resposta do servidor (ate 15 segundos) ---
    uint32_t timeout = millis() + 15000;
    while (!client.available() && millis() < timeout) delay(10);
    
    // Le a resposta
    String response;
    while (client.available()) response += client.readString();
    client.stop();
    
    // --- Interpreta o JSON de resposta ---
    // Exemplo de resposta: {"text":"Ola!","reaction":"happy","tts_url":"..."}
    int jsonStart = response.indexOf('{');
    int jsonEnd = response.lastIndexOf('}');
    
    if (jsonStart >= 0 && jsonEnd > jsonStart) {
        String jsonBody = response.substring(jsonStart, jsonEnd + 1);
        JsonDocument doc;                    // Objeto JSON
        DeserializationError error = deserializeJson(doc, jsonBody);
        
        if (!error) {
            // Extrai os campos da resposta
            // "text": o que a Neon vai dizer
            // "reaction": qual emocao mostrar (idle, happy, sad, etc)
            // "tts_url": URL do audio pra tocar (opcional)
            const char* text = doc["text"] | "(sem resposta)";
            const char* reaction = doc["reaction"] | "idle";
            const char* ttsUrl = doc["tts_url"] | "";
            
            // Aplica a reacao no avatar
            avatar.setEmotionByName(reaction);
            // Mostra o texto na tela
            display.showText(text);
            
            // Se tiver audio pra tocar, toca
            if (strlen(ttsUrl) > 0 && config.isSoundEnabled()) {
                audioState = AUDIO_PLAYING;
                audio.playURL(ttsUrl);
            } else {
                audioState = AUDIO_IDLE;
            }
        } else {
            avatar.setEmotion(AVATAR_ERROR);
            audioState = AUDIO_IDLE;
        }
    } else {
        avatar.setEmotion(AVATAR_ERROR);
        audioState = AUDIO_IDLE;
    }
}

//=============================================================================
// POLLING — Pergunta ao servidor se tem notificacoes novas
//=============================================================================
void pollServer() {
    WiFiClient client;
    // Tenta conectar no servidor. Se falhar, sai sem erro (tudo bem)
    if (!client.connect(config.getServerHost(), config.getServerPort())) return;
    
    // Faz uma requisicao GET pro endpoint de poll
    client.println(String("GET ") + config.getPollEndpoint() + " HTTP/1.1");
    client.println(String("Host: ") + config.getServerHost());
    client.println("Connection: close");
    client.println();
    
    // Aguarda resposta (ate 5 segundos)
    uint32_t timeout = millis() + 5000;
    String response;
    while (!client.available() && millis() < timeout) delay(10);
    while (client.available()) response += client.readString();
    client.stop();
    
    // Tenta interpretar o JSON
    int jsonStart = response.indexOf('{');
    int jsonEnd = response.lastIndexOf('}');
    
    if (jsonStart >= 0 && jsonEnd > jsonStart) {
        String jsonBody = response.substring(jsonStart, jsonEnd + 1);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonBody);
        
        if (!error) {
            // Verifica se tem notificacao pendente
            bool hasNotification = doc["notify"] | false;
            if (hasNotification) {
                // Extrai os dados da notificacao
                const char* text = doc["text"] | "";
                const char* reaction = doc["reaction"] | "surprised";
                
                // Acorda a tela se estiver dormindo
                if (sleeping) { display.wake(); sleeping = false; }
                resetSleepTimer();
                
                // Aplica a reacao no avatar
                avatar.setEmotionByName(reaction);
                // Mostra a notificacao na tela
                if (strlen(text) > 0) display.showNotification(text);
                
                // Se tiver audio, toca
                const char* ttsUrl = doc["tts_url"] | "";
                if (strlen(ttsUrl) > 0 && config.isSoundEnabled()) {
                    audioState = AUDIO_PLAYING;
                    audio.playURL(ttsUrl);
                }
            }
        }
    }
}

//=============================================================================
// UTILITARIOS
//=============================================================================

// resetSleepTimer: marca que o usuario fez alguma coisa agora
void resetSleepTimer() {
    lastActivity = millis();
    // Se a tela estava dormindo, acorda
    if (sleeping) { sleeping = false; display.wake(); }
}

// goToSleep: desliga o aparelho pra economizar bateria
void goToSleep() {
    Serial.println("Deep sleep...");
    sleeping = true;
    avatar.setEmotion(AVATAR_SLEEP);  // Neon dormindo
    display.sleep();                   // Apaga a tela
    
    // Configura o botao G0 (GPIO 0) pra acordar o aparelho
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
    
    delay(100);
    esp_deep_sleep_start();  // Desliga de verdade
    // O codigo nunca passa daqui — o ESP32 reinicia quando acordar
}

//=============================================================================
// CONFIGURACAO — Carrega/Salva as configuracoes
//=============================================================================
// A configuracao e carregada nesta ordem:
//   1. SD card (/neon/config.json) — prioridade maxima
//   2. SPIFFS (/config.json) — fallback
//   3. Valores padrao — se nada existir

void loadConfig() {
    File file;
    bool fromSD = false;
    
    // --- 1. Tenta ler do SD card ---
    if (SD.cardType() != CARD_NONE) {
        if (SD.exists(SD_CONFIG_PATH)) {
            file = SD.open(SD_CONFIG_PATH, FILE_READ);
            if (file) fromSD = true;
        }
    }
    
    // --- 2. Fallback: tenta ler do SPIFFS (memoria interna) ---
    if (!file) {
        if (SPIFFS.exists("/config.json")) {
            file = SPIFFS.open("/config.json", "r");
        }
    }
    
    // --- 3. Nada encontrado: usa valores padrao ---
    if (!file) {
        Serial.println("[Config] Nenhum config, defaults");
        config.setDefaults();
        saveConfig();  // Salva os defaults no SPIFFS
        return;
    }
    
    // --- Interpreta o JSON e carrega as configuracoes ---
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("[Config] Erro JSON: %s\n", error.c_str());
        config.setDefaults();
        return;
    }
    
    config.load(doc);
    Serial.printf("[Config] Carregada (%s)\n", fromSD ? "SD" : "SPIFFS");
}

// saveConfig: salva as configuracoes atuais no SPIFFS
void saveConfig() {
    if (!SPIFFS.begin(false)) return;
    File file = SPIFFS.open("/config.json", "w");
    if (!file) return;
    JsonDocument doc;
    config.save(doc);
    serializeJson(doc, file);
    file.close();
}
