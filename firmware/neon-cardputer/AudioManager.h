#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <M5Cardputer.h>

// ============================================================
// AudioManager — Gerencia microfone I2S e speaker do M5CardPuter
//
// Nota: Implementação real do I2S mic precisa de configuração
// específica dos pinos do M5CardPuter.
// Por enquanto, as funções de gravação simulam o buffer.
// ============================================================
class AudioManager {
private:
    int _sampleRate;
    int _bitsPerSample;
    int _channels;
    bool _recording = false;
    bool _playing = false;
    
    // Buffer e tamanho (ponteiros para as variáveis globais em main.cpp)
    // NOTA: startRecording recebe &audioBuffer e &audioBufferSize,
    // então _bufferArmazena o endereço do ponteiro global, não de um parâmetro local.
    uint8_t** _buffer = nullptr;
    size_t* _bufferSize = nullptr;
    
public:
    AudioManager(int sampleRate, int bitsPerSample, int channels) 
        : _sampleRate(sampleRate), _bitsPerSample(bitsPerSample), _channels(channels) {}
    
    void init() {
        Serial.println("[Audio] Inicializado (I2S mic + speaker)");
        // TODO: Configurar I2S pins do M5CardPuter:
        //   Mic: I2S_WS=GPIO_NUM_12, I2S_SCK=GPIO_NUM_11, I2S_SD=GPIO_NUM_13
        //   Speaker: I2S_BCK=GPIO_NUM_34, I2S_WS=GPIO_NUM_35, I2S_DOUT=GPIO_NUM_37
    }
    
    // Inicia gravação — recebe &audioBuffer (ponteiro para o ponteiro global)
    // e &audioBufferSize (ponteiro para o size_t global)
    void startRecording(uint8_t** buffer, size_t* bufferSize) {
        _buffer = buffer;        // Guarda endereço do ponteiro global
        _bufferSize = bufferSize;
        _recording = true;
        *_bufferSize = 0;
        
        Serial.println("[Audio] Gravação iniciada");
        // TODO: Iniciar I2S_READ() em loop
    }
    
    void recordChunk() {
        if (!_recording || !_buffer || !_bufferSize) return;
        
        // TODO: Ler dados reais do I2S mic
        // Na implementação real:
        //   size_t bytesRead = 0;
        //   esp_i2s_read(I2S_NUM_0, (*_buffer) + (*_bufferSize), 512, &bytesRead, portMAX_DELAY);
        //   *_bufferSize += bytesRead;
        
        delay(1); // Simula tempo de leitura (remover na impl real)
    }
    
    void stopRecording() {
        _recording = false;
        Serial.printf("[Audio] Gravação finalizada: %u bytes\n", 
                      _bufferSize ? *_bufferSize : 0);
    }
    
    bool isPlaying() { 
        return _playing; 
    }
    
    void playURL(const String& url) {
        Serial.printf("[Audio] Tocando TTS: %s\n", url.c_str());
        _playing = true;
        
        // TODO: Implementar streaming HTTP + I2S speaker
        // Usar ESP8266Audio ou AudioOutputI2S
        delay(500);
        _playing = false;
    }
    
    // WAV header helper (monta cabeçalho WAV no buffer)
    static size_t buildWavHeader(uint8_t* buffer, size_t maxSize, size_t dataSize, int sampleRate) {
        if (maxSize < 44) return 0;
        
        // RIFF header
        memcpy(buffer, "RIFF", 4);
        uint32_t chunkSize = 36 + dataSize;
        memcpy(buffer + 4, &chunkSize, 4);
        memcpy(buffer + 8, "WAVE", 4);
        
        // fmt chunk
        memcpy(buffer + 12, "fmt ", 4);
        uint32_t fmtSize = 16;
        memcpy(buffer + 16, &fmtSize, 4);
        uint16_t audioFormat = 1; // PCM
        memcpy(buffer + 20, &audioFormat, 2);
        uint16_t numChannels = 1;
        memcpy(buffer + 22, &numChannels, 2);
        memcpy(buffer + 24, &sampleRate, 4);
        uint32_t byteRate = sampleRate * 1 * 16 / 8;
        memcpy(buffer + 28, &byteRate, 4);
        uint16_t blockAlign = 1 * 16 / 8;
        memcpy(buffer + 32, &blockAlign, 2);
        uint16_t bitsPerSample = 16;
        memcpy(buffer + 34, &bitsPerSample, 2);
        
        // data chunk
        memcpy(buffer + 36, "data", 4);
        memcpy(buffer + 40, &dataSize, 4);
        
        return 44; // Tamanho do header
    }
};

#endif // AUDIOMANAGER_H
