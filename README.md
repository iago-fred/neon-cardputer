# Neon Widget 👻📟

Widget fisico com **M5CardPuter** que se conecta ao WiFi e mostra status na tela.

## Hardware

- **M5CardPuter** (ESP32-S3, 8MB Flash, PSRAM)
- Tela 240x135 colorida
- Teclado QWERTY

## Estrutura

```
neon-cardputer/
└── firmware/
    ├── platformio.ini
    └── neon-cardputer/
        └── neon-cardputer.ino
```

## Como usar

### Arduino IDE

1. **Arquivo > Preferencias** → URL:
   ```
   https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
   ```
2. **Ferramentas > Placa > Gerenciador de Placas** → instalar `M5Stack` (>= 3.2.2)
3. **Ferramentas > Placa > M5Stack > M5Cardputer**
4. Abrir `firmware/neon-cardputer/neon-cardputer.ino`
5. Conectar USB-C e **Upload**

### SD Card (opcional)

Criar arquivo `/neon/config.json` no cartao:

```json
{
  "wifi": [{"ssid": "MinhaRede", "password": "minha_senha"}]
}
```

Sem SD, o firmware tenta conectar na ultima rede salva.
