// MAR4018 / NT35510, ESP32-S3, bus paralelo de 8 bits.
// R5 cerrado, R4 abierto. Sin bibliotecas externas.
// Secuencia de registros: LCDWIKI_KBV, paquete oficial MAR4018 V1.0 (MIT).
// https://www.lcdwiki.com/4.0inch_Arduino_Display-Mega2560_NT35510
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>
#include <freertos/queue.h>
#include "Configuracion.h"
#include <ESP_I2S.h>
#include <math.h>
#include <soc/gpio_reg.h>
#include <soc/soc.h>

enum class Vista : uint8_t { Inicio, Menu, Pomodoro, Chat, Calendario };

constexpr int WR = 15;
constexpr int RS = 21;
constexpr int CS = 38;
constexpr int RST = 39;
constexpr int RD = 40;
constexpr int PRIMER_DATO = 7; // DB0..DB7 conectados a GPIO 7..14.
constexpr uint32_t MASCARA_DATOS = 0xFFu << PRIMER_DATO;

// Orientacion vertical nativa del controlador.
constexpr uint16_t ANCHO = 480;
constexpr uint16_t ALTO = 800;
// Rotacion por software del dibujo; no cambia el modo del NT35510.
// 1 = horizontal. Si el texto sale de cabeza, cambia a 3.
constexpr int GIRO = 1;
static_assert(GIRO == 1 || GIRO == 3, "Usa GIRO 1 o 3 en horizontal");
constexpr uint16_t ROJO = 0xF800;
constexpr uint16_t VERDE = 0x07E0;
constexpr uint16_t AZUL = 0x001F;
constexpr uint16_t NEGRO = 0x0000;
constexpr uint16_t BLANCO = 0xFFFF;
constexpr uint16_t GRIS = 0x4208;
constexpr uint16_t AMARILLO = 0xFFE0;

constexpr int JOY_X = 1;
constexpr int JOY_Y = 2;
constexpr int JOY_SW = 4;
constexpr int BOTON_CHAT = 5;
constexpr int ZONA_MUERTA = 500;
// Cambia a true si el eje físico de tu joystick queda invertido.
constexpr bool INVERTIR_X = false;
constexpr bool INVERTIR_Y = false;

constexpr int FRECUENCIA_AUDIO = 16000;
constexpr int SEGUNDOS = 3;
constexpr int MUESTRAS_VOZ = FRECUENCIA_AUDIO * SEGUNDOS;
constexpr int FRAMES = 256;
constexpr int LIMITE_VOZ = 4000;
I2SClass microfono(I2S_NUM_0);
I2SClass bocina(I2S_NUM_1);
bool microfonoListo = false;
bool bocinaLista = false;
int32_t entrada[FRAMES * 2];
int16_t voz[MUESTRAS_VOZ];
int16_t salida[FRAMES * 2];
int16_t silencio[FRAMES * 2] = {};

struct Pulsador {
  int pin;
  bool ultimo = HIGH;
  bool estable = HIGH;
  uint32_t cambio = 0;

  bool nuevaPulsacion() {
    bool lectura = digitalRead(pin);
    uint32_t ahora = millis();
    if (lectura != ultimo) {
      ultimo = lectura;
      cambio = ahora;
    }
    if (lectura != estable && ahora - cambio >= 30) {
      estable = lectura;
      return estable == LOW;
    }
    return false;
  }
};

Pulsador centroJoystick{JOY_SW};
Pulsador botonChat{BOTON_CHAT};
int centroX = 2048;
int centroY = 2048;

struct Registro {
  uint16_t direccion;
  uint16_t valor;
};

const Registro inicializacion[] = {
  {0xF000, 0x55}, {0xF001, 0xAA}, {0xF002, 0x52}, {0xF003, 0x08},
  {0xF004, 0x01}, {0xB600, 0x34}, {0xB601, 0x34}, {0xB602, 0x34},
  {0xB000, 0x0D}, {0xB001, 0x0D}, {0xB002, 0x0D}, {0xB700, 0x24},
  {0xB701, 0x24}, {0xB702, 0x24}, {0xB100, 0x0D}, {0xB101, 0x0D},
  {0xB102, 0x0D}, {0xB800, 0x24}, {0xB801, 0x24}, {0xB802, 0x24},
  {0xB200, 0x00}, {0xB900, 0x24}, {0xB901, 0x24}, {0xB902, 0x24},
  {0xB300, 0x05}, {0xB301, 0x05}, {0xB302, 0x05}, {0xBA00, 0x34},
  {0xBA01, 0x34}, {0xBA02, 0x34}, {0xB500, 0x0B}, {0xB501, 0x0B},
  {0xB502, 0x0B}, {0xBC00, 0X00}, {0xBC01, 0xA3}, {0xBC02, 0X00},
  {0xBD00, 0x00}, {0xBD01, 0xA3}, {0xBD02, 0x00}, {0xBE00, 0x00},
  {0xBE01, 0x63}, {0xD100, 0x00}, {0xD101, 0x37}, {0xD102, 0x00},
  {0xD103, 0x52}, {0xD104, 0x00}, {0xD105, 0x7B}, {0xD106, 0x00},
  {0xD107, 0x99}, {0xD108, 0x00}, {0xD109, 0xB1}, {0xD10A, 0x00},
  {0xD10B, 0xD2}, {0xD10C, 0x00}, {0xD10D, 0xF6}, {0xD10E, 0x01},
  {0xD10F, 0x27}, {0xD110, 0x01}, {0xD111, 0x4E}, {0xD112, 0x01},
  {0xD113, 0x8C}, {0xD114, 0x01}, {0xD115, 0xBE}, {0xD116, 0x02},
  {0xD117, 0x0B}, {0xD118, 0x02}, {0xD119, 0x48}, {0xD11A, 0x02},
  {0xD11B, 0x4A}, {0xD11C, 0x02}, {0xD11D, 0x7E}, {0xD11E, 0x02},
  {0xD11F, 0xBC}, {0xD120, 0x02}, {0xD121, 0xE1}, {0xD122, 0x03},
  {0xD123, 0x10}, {0xD124, 0x03}, {0xD125, 0x31}, {0xD126, 0x03},
  {0xD127, 0x5A}, {0xD128, 0x03}, {0xD129, 0x73}, {0xD12A, 0x03},
  {0xD12B, 0x94}, {0xD12C, 0x03}, {0xD12D, 0x9F}, {0xD12E, 0x03},
  {0xD12F, 0xB3}, {0xD130, 0x03}, {0xD131, 0xB9}, {0xD132, 0x03},
  {0xD133, 0xC1}, {0xD200, 0x00}, {0xD201, 0x37}, {0xD202, 0x00},
  {0xD203, 0x52}, {0xD204, 0x00}, {0xD205, 0x7B}, {0xD206, 0x00},
  {0xD207, 0x99}, {0xD208, 0x00}, {0xD209, 0xB1}, {0xD20A, 0x00},
  {0xD20B, 0xD2}, {0xD20C, 0x00}, {0xD20D, 0xF6}, {0xD20E, 0x01},
  {0xD20F, 0x27}, {0xD210, 0x01}, {0xD211, 0x4E}, {0xD212, 0x01},
  {0xD213, 0x8C}, {0xD214, 0x01}, {0xD215, 0xBE}, {0xD216, 0x02},
  {0xD217, 0x0B}, {0xD218, 0x02}, {0xD219, 0x48}, {0xD21A, 0x02},
  {0xD21B, 0x4A}, {0xD21C, 0x02}, {0xD21D, 0x7E}, {0xD21E, 0x02},
  {0xD21F, 0xBC}, {0xD220, 0x02}, {0xD221, 0xE1}, {0xD222, 0x03},
  {0xD223, 0x10}, {0xD224, 0x03}, {0xD225, 0x31}, {0xD226, 0x03},
  {0xD227, 0x5A}, {0xD228, 0x03}, {0xD229, 0x73}, {0xD22A, 0x03},
  {0xD22B, 0x94}, {0xD22C, 0x03}, {0xD22D, 0x9F}, {0xD22E, 0x03},
  {0xD22F, 0xB3}, {0xD230, 0x03}, {0xD231, 0xB9}, {0xD232, 0x03},
  {0xD233, 0xC1}, {0xD300, 0x00}, {0xD301, 0x37}, {0xD302, 0x00},
  {0xD303, 0x52}, {0xD304, 0x00}, {0xD305, 0x7B}, {0xD306, 0x00},
  {0xD307, 0x99}, {0xD308, 0x00}, {0xD309, 0xB1}, {0xD30A, 0x00},
  {0xD30B, 0xD2}, {0xD30C, 0x00}, {0xD30D, 0xF6}, {0xD30E, 0x01},
  {0xD30F, 0x27}, {0xD310, 0x01}, {0xD311, 0x4E}, {0xD312, 0x01},
  {0xD313, 0x8C}, {0xD314, 0x01}, {0xD315, 0xBE}, {0xD316, 0x02},
  {0xD317, 0x0B}, {0xD318, 0x02}, {0xD319, 0x48}, {0xD31A, 0x02},
  {0xD31B, 0x4A}, {0xD31C, 0x02}, {0xD31D, 0x7E}, {0xD31E, 0x02},
  {0xD31F, 0xBC}, {0xD320, 0x02}, {0xD321, 0xE1}, {0xD322, 0x03},
  {0xD323, 0x10}, {0xD324, 0x03}, {0xD325, 0x31}, {0xD326, 0x03},
  {0xD327, 0x5A}, {0xD328, 0x03}, {0xD329, 0x73}, {0xD32A, 0x03},
  {0xD32B, 0x94}, {0xD32C, 0x03}, {0xD32D, 0x9F}, {0xD32E, 0x03},
  {0xD32F, 0xB3}, {0xD330, 0x03}, {0xD331, 0xB9}, {0xD332, 0x03},
  {0xD333, 0xC1}, {0xD400, 0x00}, {0xD401, 0x37}, {0xD402, 0x00},
  {0xD403, 0x52}, {0xD404, 0x00}, {0xD405, 0x7B}, {0xD406, 0x00},
  {0xD407, 0x99}, {0xD408, 0x00}, {0xD409, 0xB1}, {0xD40A, 0x00},
  {0xD40B, 0xD2}, {0xD40C, 0x00}, {0xD40D, 0xF6}, {0xD40E, 0x01},
  {0xD40F, 0x27}, {0xD410, 0x01}, {0xD411, 0x4E}, {0xD412, 0x01},
  {0xD413, 0x8C}, {0xD414, 0x01}, {0xD415, 0xBE}, {0xD416, 0x02},
  {0xD417, 0x0B}, {0xD418, 0x02}, {0xD419, 0x48}, {0xD41A, 0x02},
  {0xD41B, 0x4A}, {0xD41C, 0x02}, {0xD41D, 0x7E}, {0xD41E, 0x02},
  {0xD41F, 0xBC}, {0xD420, 0x02}, {0xD421, 0xE1}, {0xD422, 0x03},
  {0xD423, 0x10}, {0xD424, 0x03}, {0xD425, 0x31}, {0xD426, 0x03},
  {0xD427, 0x5A}, {0xD428, 0x03}, {0xD429, 0x73}, {0xD42A, 0x03},
  {0xD42B, 0x94}, {0xD42C, 0x03}, {0xD42D, 0x9F}, {0xD42E, 0x03},
  {0xD42F, 0xB3}, {0xD430, 0x03}, {0xD431, 0xB9}, {0xD432, 0x03},
  {0xD433, 0xC1}, {0xD500, 0x00}, {0xD501, 0x37}, {0xD502, 0x00},
  {0xD503, 0x52}, {0xD504, 0x00}, {0xD505, 0x7B}, {0xD506, 0x00},
  {0xD507, 0x99}, {0xD508, 0x00}, {0xD509, 0xB1}, {0xD50A, 0x00},
  {0xD50B, 0xD2}, {0xD50C, 0x00}, {0xD50D, 0xF6}, {0xD50E, 0x01},
  {0xD50F, 0x27}, {0xD510, 0x01}, {0xD511, 0x4E}, {0xD512, 0x01},
  {0xD513, 0x8C}, {0xD514, 0x01}, {0xD515, 0xBE}, {0xD516, 0x02},
  {0xD517, 0x0B}, {0xD518, 0x02}, {0xD519, 0x48}, {0xD51A, 0x02},
  {0xD51B, 0x4A}, {0xD51C, 0x02}, {0xD51D, 0x7E}, {0xD51E, 0x02},
  {0xD51F, 0xBC}, {0xD520, 0x02}, {0xD521, 0xE1}, {0xD522, 0x03},
  {0xD523, 0x10}, {0xD524, 0x03}, {0xD525, 0x31}, {0xD526, 0x03},
  {0xD527, 0x5A}, {0xD528, 0x03}, {0xD529, 0x73}, {0xD52A, 0x03},
  {0xD52B, 0x94}, {0xD52C, 0x03}, {0xD52D, 0x9F}, {0xD52E, 0x03},
  {0xD52F, 0xB3}, {0xD530, 0x03}, {0xD531, 0xB9}, {0xD532, 0x03},
  {0xD533, 0xC1}, {0xD600, 0x00}, {0xD601, 0x37}, {0xD602, 0x00},
  {0xD603, 0x52}, {0xD604, 0x00}, {0xD605, 0x7B}, {0xD606, 0x00},
  {0xD607, 0x99}, {0xD608, 0x00}, {0xD609, 0xB1}, {0xD60A, 0x00},
  {0xD60B, 0xD2}, {0xD60C, 0x00}, {0xD60D, 0xF6}, {0xD60E, 0x01},
  {0xD60F, 0x27}, {0xD610, 0x01}, {0xD611, 0x4E}, {0xD612, 0x01},
  {0xD613, 0x8C}, {0xD614, 0x01}, {0xD615, 0xBE}, {0xD616, 0x02},
  {0xD617, 0x0B}, {0xD618, 0x02}, {0xD619, 0x48}, {0xD61A, 0x02},
  {0xD61B, 0x4A}, {0xD61C, 0x02}, {0xD61D, 0x7E}, {0xD61E, 0x02},
  {0xD61F, 0xBC}, {0xD620, 0x02}, {0xD621, 0xE1}, {0xD622, 0x03},
  {0xD623, 0x10}, {0xD624, 0x03}, {0xD625, 0x31}, {0xD626, 0x03},
  {0xD627, 0x5A}, {0xD628, 0x03}, {0xD629, 0x73}, {0xD62A, 0x03},
  {0xD62B, 0x94}, {0xD62C, 0x03}, {0xD62D, 0x9F}, {0xD62E, 0x03},
  {0xD62F, 0xB3}, {0xD630, 0x03}, {0xD631, 0xB9}, {0xD632, 0x03},
  {0xD633, 0xC1}, {0xF000, 0x55}, {0xF001, 0xAA}, {0xF002, 0x52},
  {0xF003, 0x08}, {0xF004, 0x00}, {0xB000, 0x08}, {0xB001, 0x05},
  {0xB002, 0x02}, {0xB003, 0x05}, {0xB004, 0x02}, {0xB600, 0x08},
  {0xB500, 0x50}, {0xB700, 0x00}, {0xB701, 0x00}, {0xB800, 0x01},
  {0xB801, 0x05}, {0xB802, 0x05}, {0xB803, 0x05}, {0xBC00, 0x00},
  {0xBC01, 0x00}, {0xBC02, 0x00}, {0xCC00, 0x03}, {0xCC01, 0x00},
  {0xCC02, 0x00}, {0xBD00, 0x01}, {0xBD01, 0x84}, {0xBD02, 0x07},
  {0xBD03, 0x31}, {0xBD04, 0x00}, {0xBA00, 0x01}, {0xFF00, 0xAA},
  {0xFF01, 0x55}, {0xFF02, 0x25}, {0xFF03, 0x01}, {0x3500, 0x00},
  {0x3600, 0x00}, {0x3a00, 0x55},
};

static inline void enviarByte(uint8_t dato) {
  // Los ocho GPIO de datos se actualizan juntos.
  REG_WRITE(GPIO_OUT_W1TC_REG, MASCARA_DATOS);
  REG_WRITE(GPIO_OUT_W1TS_REG, uint32_t(dato) << PRIMER_DATO);
  delayMicroseconds(1);
  digitalWrite(WR, LOW);
  delayMicroseconds(1);
  digitalWrite(WR, HIGH);
}

static inline void enviarPalabra(uint16_t valor) {
  enviarByte(uint8_t(valor >> 8));
  enviarByte(uint8_t(valor));
}

void comando(uint16_t direccion) {
  digitalWrite(CS, LOW);
  digitalWrite(RS, LOW);
  enviarPalabra(direccion);
  digitalWrite(RS, HIGH);
  digitalWrite(CS, HIGH);
}

void escribirRegistro(uint16_t direccion, uint16_t valor) {
  digitalWrite(CS, LOW);
  digitalWrite(RS, LOW);
  enviarPalabra(direccion);
  digitalWrite(RS, HIGH);
  // El ejemplo oficial transmite dos bytes tambien para el valor.
  enviarPalabra(valor);
  digitalWrite(CS, HIGH);
}

uint16_t *cuadro = nullptr;
uint16_t *cuadroAnterior = nullptr;
bool primerCuadro = true;
bool dibujoPendiente = false;

void escribirBloqueLCD(int x, int y, int ancho, int alto, uint16_t color,
                       const uint16_t *pixeles) {
  if (x < 0 || y < 0 || ancho <= 0 || alto <= 0 ||
      x + ancho > ANCHO || y + alto > ALTO) return;

  const int ultimoX = x + ancho - 1;
  const int ultimoY = y + alto - 1;
  escribirRegistro(0x2A00, x >> 8);
  escribirRegistro(0x2A01, x & 0xFF);
  escribirRegistro(0x2A02, ultimoX >> 8);
  escribirRegistro(0x2A03, ultimoX & 0xFF);
  escribirRegistro(0x2B00, y >> 8);
  escribirRegistro(0x2B01, y & 0xFF);
  escribirRegistro(0x2B02, ultimoY >> 8);
  escribirRegistro(0x2B03, ultimoY & 0xFF);

  digitalWrite(CS, LOW);
  digitalWrite(RS, LOW);
  enviarPalabra(0x2C00); // Inicio de escritura en la memoria de imagen.
  digitalWrite(RS, HIGH);

  for (int fila = 0; fila < alto; ++fila) {
    for (int col = 0; col < ancho; ++col) {
      enviarPalabra(pixeles ? pixeles[(y + fila) * ANCHO + x + col] : color);
    }
    // Cede tiempo al sistema sin cambiar la transaccion de escritura.
    if ((fila & 15) == 15) delay(1);
  }

  digitalWrite(CS, HIGH);
}

void rellenarBloque(int x, int y, int ancho, int alto, uint16_t color) {
  if (!cuadro) {
    escribirBloqueLCD(x, y, ancho, alto, color, nullptr);
    return;
  }
  if (x < 0 || y < 0 || ancho <= 0 || alto <= 0 ||
      x + ancho > ANCHO || y + alto > ALTO) return;
  dibujoPendiente = true;
  for (int fila = y; fila < y + alto; ++fila) {
    uint16_t *inicio = cuadro + fila * ANCHO + x;
    for (int col = 0; col < ancho; ++col) inicio[col] = color;
  }
}

void prepararCuadro() {
  const size_t bytes = size_t(ANCHO) * ALTO * sizeof(uint16_t);
  cuadro = static_cast<uint16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  cuadroAnterior = static_cast<uint16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (cuadro && cuadroAnterior) return;
  free(cuadro);
  free(cuadroAnterior);
  cuadro = cuadroAnterior = nullptr;
  Serial.println("PSRAM no disponible: dibujo directo. Activa OPI PSRAM.");
}

void actualizarPantalla() {
  if (!dibujoPendiente) return;
  dibujoPendiente = false;
  // Compara bloques en el orden nativo del LCD. Cada bloque llega compuesto.
  constexpr int lado = 40;
  static_assert(ANCHO % lado == 0 && ALTO % lado == 0, "Bloques completos");
  for (int y = 0; y < ALTO; y += lado) {
    for (int x = 0; x < ANCHO; x += lado) {
      bool cambio = primerCuadro;
      for (int f = y; !cambio && f < y + lado; ++f) {
        const int i = f * ANCHO + x;
        cambio = memcmp(cuadro + i, cuadroAnterior + i, lado * sizeof(uint16_t)) != 0;
      }
      if (!cambio) continue;
      escribirBloqueLCD(x, y, lado, lado, 0, cuadro);
      for (int f = y; f < y + lado; ++f) {
        const int i = f * ANCHO + x;
        memcpy(cuadroAnterior + i, cuadro + i, lado * sizeof(uint16_t));
      }
    }
    delay(1);
  }
  primerCuadro = false;
}

void bloqueHorizontal(int x, int y, int ancho, int alto, uint16_t color) {
  if (GIRO == 1) {
    rellenarBloque(ANCHO - y - alto, x, alto, ancho, color);
  } else {
    rellenarBloque(y, ALTO - x - ancho, alto, ancho, color);
  }
}

void iniciarPantalla() {
  pinMode(RST, OUTPUT);
  digitalWrite(RST, LOW);
  const int controles[] = {CS, RD, WR, RS};
  for (int pin : controles) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }
  for (int i = 0; i < 8; ++i) {
    pinMode(PRIMER_DATO + i, OUTPUT);
    digitalWrite(PRIMER_DATO + i, LOW);
  }
  delay(30);
  digitalWrite(RST, HIGH);
  delay(200);

  Serial.println("MAR4018: iniciando NT35510, paralelo 8 bits");
  for (const Registro &reg : inicializacion) {
    if (reg.direccion == 0xFFFF) delay(reg.valor);
    else escribirRegistro(reg.direccion, reg.valor);
  }

  comando(0x1100); // Salir de reposo.
  delay(120);
  comando(0x2900); // Encender imagen.
  delay(50);
}

void barraEstado(uint16_t color) {
  bloqueHorizontal(100, 342, 600, 24, color);
}

void enviarSilencio() {
  if (bocinaLista) bocina.write((const uint8_t*)silencio, sizeof(silencio));
}

void iniciarAudio() {
  // Cableado original: reloj, selección, datos.
  bocina.setPins(41, 42, 47, -1);
  bocinaLista = bocina.begin(I2S_MODE_STD, FRECUENCIA_AUDIO,
                            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  Serial.println(bocinaLista ? "Bocina I2S: lista" : "ERROR: I2S de bocina");
  enviarSilencio();

  microfono.setPins(16, 17, -1, 18);
  microfonoListo = microfono.begin(I2S_MODE_STD, FRECUENCIA_AUDIO,
                                  I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO);
  Serial.println(microfonoListo ? "Microfono I2S: listo" : "ERROR: I2S de microfono");
}

bool enviarAudio(size_t frames) {
  const size_t bytes = frames * 2 * sizeof(salida[0]);
  if (bocina.write((const uint8_t*)salida, bytes) == bytes) return true;
  Serial.println("ERROR: escritura I2S incompleta");
  return false;
}

void probarTono() {
  if (!bocinaLista) {
    Serial.println("Bocina no inicializada.");
    return;
  }
  Serial.println("Tono de 440 Hz");
  constexpr int total = FRECUENCIA_AUDIO / 5;
  for (int offset = 0; offset < total; offset += FRAMES) {
    int cantidad = min(FRAMES, total - offset);
    for (int i = 0; i < cantidad; ++i) {
      int t = offset + i;
      float rampa = min(1.0f, min(t, total - 1 - t) / 160.0f);
      int16_t muestra = int16_t(1000.0f * rampa *
        sinf(2.0f * PI * 440.0f * t / FRECUENCIA_AUDIO));
      salida[i * 2] = muestra;
      salida[i * 2 + 1] = muestra;
    }
    if (!enviarAudio(cantidad)) break;
  }
  for (int i = 0; i < 8; ++i) enviarSilencio();
}

bool capturarVoz() {
  // El micro permanece iniciado. Vacía muestras anteriores a la pulsación.
  for (int i = 0; i < 20; ++i) {
    size_t bytes = microfono.readBytes((char*)entrada, sizeof(entrada));
    if (bytes < 2 * sizeof(entrada[0]) || bytes % 8 != 0) return false;
    enviarSilencio();
  }

  Serial.println("GRABANDO: habla durante tres segundos");
  int guardadas = 0;
  while (guardadas < MUESTRAS_VOZ) {
    size_t bytes = microfono.readBytes((char*)entrada, sizeof(entrada));
    if (bytes < 2 * sizeof(entrada[0]) || bytes % 8 != 0) return false;
    int frames = bytes / (2 * sizeof(entrada[0]));
    // INMP441: L/R a GND, canal izquierdo.
    for (int i = 0; i < frames && guardadas < MUESTRAS_VOZ; ++i) {
      voz[guardadas++] = int16_t(entrada[i * 2] / 65536);
    }
    enviarSilencio();
  }
  return true;
}

bool reproducirVoz() {
  int32_t suma = 0;
  for (int i = 0; i < MUESTRAS_VOZ; ++i) suma += voz[i];
  int32_t promedio = suma / MUESTRAS_VOZ;
  int32_t pico = 1;
  for (int i = 0; i < MUESTRAS_VOZ; ++i) {
    int32_t v = int32_t(voz[i]) - promedio;
    if (v < 0) v = -v;
    if (v > pico) pico = v;
  }
  Serial.print("Pico de la voz capturada: ");
  Serial.println(pico);
  const int32_t divisor = pico > LIMITE_VOZ ? pico : LIMITE_VOZ;
  for (int offset = 0; offset < MUESTRAS_VOZ; offset += FRAMES) {
    int cantidad = min(FRAMES, MUESTRAS_VOZ - offset);
    for (int i = 0; i < cantidad; ++i) {
      int t = offset + i;
      int32_t v = (int32_t(voz[t]) - promedio) * LIMITE_VOZ / divisor;
      // Atenúa los primeros y últimos 20 ms para reducir saltos de señal.
      int rampa = min(320, min(t, MUESTRAS_VOZ - 1 - t));
      int16_t muestra = int16_t(v * rampa / 320);
      salida[i * 2] = muestra;
      salida[i * 2 + 1] = muestra;
    }
    if (!enviarAudio(cantidad)) return false;
  }
  for (int i = 0; i < 16; ++i) enviarSilencio();
  return true;
}

void probarVoz() {
  if (!microfonoListo || !bocinaLista) {
    barraEstado(AMARILLO);
    Serial.println("No se puede grabar/reproducir: revisa los errores de I2S.");
    return;
  }
  barraEstado(ROJO);
  if (!capturarVoz()) {
    barraEstado(AMARILLO);
    Serial.println("ERROR: captura I2S incompleta");
    return;
  }
  barraEstado(AZUL);
  Serial.println("REPRODUCIENDO");
  bool ok = reproducirVoz();
  barraEstado(ok ? VERDE : AMARILLO);
  if (ok) Serial.println("Prueba de voz terminada.");
}

// Interfaz nativa basada en el prototipo MEMO.html.
// Letra de 5 x 7: cada byte representa una columna, bit 0 = arriba.
constexpr char ALFABETO[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:?-/.,";
constexpr uint8_t GLIFOS[][5] = {
  {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
  {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
  {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
  {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
  {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
  {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
  {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
  {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
  {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43},
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
  {0x00,0x36,0x36,0x00,0x00}, {0x02,0x01,0x51,0x09,0x06},
  {0x08,0x08,0x08,0x08,0x08}, {0x20,0x10,0x08,0x04,0x02},
  {0x00,0x60,0x60,0x00,0x00}, {0x00,0x40,0x30,0x00,0x00}
};
static_assert(sizeof(ALFABETO) - 1 == sizeof(GLIFOS) / sizeof(GLIFOS[0]),
              "La fuente debe tener un glifo por caracter");

constexpr uint16_t rgb565(unsigned r, unsigned g, unsigned b) {
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}
constexpr uint16_t NARANJA = rgb565(255, 158, 61);
constexpr uint16_t SELECCION = rgb565(255, 224, 181);
constexpr uint16_t TINTA = rgb565(37, 37, 37);
constexpr uint16_t BORDE = rgb565(145, 145, 140);
constexpr uint16_t SUAVE = rgb565(244, 244, 240);

const char *const OPCIONES[] = {"POMODORO", "CHATBOT", "CALENDARIO"};
const Vista DESTINOS[] = {Vista::Pomodoro, Vista::Chat, Vista::Calendario};
Vista vista = Vista::Inicio;
int seleccion = 0;
bool joystickOcupado = false;
constexpr uint32_t DURACION = 25u * 60u * 1000u;
uint32_t restante = DURACION;
uint32_t vencimiento = 0;
bool contando = false;
int ultimoSegundo = -1;

void texto(int x, int y, const char *frase, int escala, uint16_t color) {
  for (; *frase; ++frase, x += 6 * escala) {
    const char *indice = strchr(ALFABETO, *frase);
    if (!indice) continue;  // Espacio u otro carácter sin dibujo.
    const uint8_t *glifo = GLIFOS[indice - ALFABETO];
    for (int fila = 0; fila < 7; ++fila) {
      int col = 0;
      while (col < 5) {
        if ((glifo[col] & (1u << fila)) == 0) {
          ++col;
          continue;
        }
        int inicio = col;
        while (col < 5 && (glifo[col] & (1u << fila)) != 0) ++col;
        bloqueHorizontal(x + inicio * escala, y + fila * escala,
                         (col - inicio) * escala, escala, color);
      }
    }
  }
}

void textoCentrado(int x, int y, const char *frase, int escala, uint16_t color) {
  int ancho = (int(strlen(frase)) * 6 - 1) * escala;
  texto(x - ancho / 2, y, frase, escala, color);
}

void ojo(int x, int y) {
  // Rectángulo redondeado por franjas sólidas.
  bloqueHorizontal(x + 20, y, 64, 8, NARANJA);
  bloqueHorizontal(x + 9, y + 8, 86, 10, NARANJA);
  bloqueHorizontal(x, y + 18, 104, 46, NARANJA);
  bloqueHorizontal(x + 9, y + 64, 86, 10, NARANJA);
  bloqueHorizontal(x + 20, y + 74, 64, 8, NARANJA);
}

void cara(int centro, int y) {
  ojo(centro - 116, y);
  ojo(centro + 12, y);
}

void pie(const char *mensaje) {
  bloqueHorizontal(0, 421, 800, 59, BLANCO);
  bloqueHorizontal(0, 421, 800, 2, BORDE);
  texto(28, 440, mensaje, 3, TINTA);
}

void opcionMenu(int i) {
  const int y = 104 + i * 100;
  const bool activa = i == seleccion;
  bloqueHorizontal(420, y, 342, 76, activa ? NARANJA : BORDE);
  bloqueHorizontal(422, y + 2, 338, 72, activa ? SELECCION : BLANCO);
  bloqueHorizontal(438, y + 24, 24, 28, activa ? NARANJA : SUAVE);
  texto(479, y + 24, OPCIONES[i], 4, TINTA);
}

void mostrarContador() {
  const int segundos = (restante + 999) / 1000;
  if (segundos == ultimoSegundo) return;
  ultimoSegundo = segundos;
  char valor[8];
  snprintf(valor, sizeof(valor), "%02d:%02d", segundos / 60, segundos % 60);
  bloqueHorizontal(145, 157, 510, 119, SUAVE);
  textoCentrado(400, 157, valor, 17, TINTA);
}

void botonPomodoro() {
  bloqueHorizontal(258, 319, 284, 63, NARANJA);
  textoCentrado(400, 335, contando ? "PAUSAR" : "INICIAR", 4, TINTA);
}

struct RespuestaChat {
  char texto[465];
};
QueueHandle_t colaChat = nullptr;
bool consultaActiva = false;
RespuestaChat respuestaChat{};
const char *estadoChat = "PULSA EL BOTON Y HABLA";

bool chatConfigurado() {
  return WIFI_NOMBRE[0] && SERVIDOR_MEMO[0] && TOKEN_MEMO[0];
}

void mostrarChat() {
  bloqueHorizontal(0, 140, 800, 278, BLANCO);
  if (!respuestaChat.texto[0]) {
    cara(400, 168);
    textoCentrado(400, 278, estadoChat, 3, TINTA);
  } else {
    const char *p = respuestaChat.texto;
    for (int fila = 0; fila < 8 && *p; ++fila) {
      char linea[63]{};
      size_t n = min(strlen(p), sizeof(linea) - 1);
      if (p[n] && p[n] != ' ') {
        size_t corte = n;
        while (corte > 0 && p[corte] != ' ') --corte;
        if (corte) n = corte;
      }
      memcpy(linea, p, n);
      texto(30, 151 + fila * 28, linea, 2, TINTA);
      p += n;
      while (*p == ' ') ++p;
    }
  }
  bloqueHorizontal(100, 405, 600, 5, consultaActiva ? NARANJA : BORDE);
}

void consultarServidor(void *) {
  RespuestaChat r{};
  const char *error = "SIN WIFI";
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient cliente;
    HTTPClient http;
    http.setConnectTimeout(4000);
    http.setTimeout(26000);
    error = "SERVIDOR MEMO NO DISPONIBLE";
    if (http.begin(cliente, SERVIDOR_MEMO)) {
      http.addHeader("Content-Type", "application/octet-stream");
      http.addHeader("X-Memo-Token", TOKEN_MEMO);
      const int codigo = http.POST(reinterpret_cast<uint8_t *>(voz), sizeof(voz));
      if (codigo == 200 && http.getSize() > 0 && http.getSize() < int(sizeof(r.texto))) {
        String textoRecibido = http.getString();
        if (textoRecibido.length() == size_t(http.getSize())) {
          textoRecibido.toCharArray(r.texto, sizeof(r.texto));
        }
      }
      if (codigo == 401) error = "REVISA EL TOKEN DE MEMO";
      else if (codigo == 429) error = "CUOTA DE GEMINI AGOTADA";
      else if (codigo == 503) error = "REVISA LA CLAVE O EL MODELO DE GEMINI";
      else if (codigo >= 400) error = "NO SE PUDO OBTENER RESPUESTA";
      http.end();
    }
  }
  if (!r.texto[0]) snprintf(r.texto, sizeof(r.texto), "%s", error);
  xQueueSend(colaChat, &r, 0);
  vTaskDelete(nullptr);
}

void iniciarConsulta() {
  if (consultaActiva) return;
  respuestaChat.texto[0] = 0;
  if (!chatConfigurado()) estadoChat = "CONFIGURA WIFI Y SERVIDOR EN EL IDE";
  else if (!colaChat) estadoChat = "NO HAY MEMORIA PARA EL CHAT";
  else if (WiFi.status() != WL_CONNECTED) estadoChat = "SIN WIFI REVISA LA CONEXION";
  else if (!microfonoListo) estadoChat = "MICROFONO NO DISPONIBLE";
  else {
    estadoChat = "GRABANDO 3 SEGUNDOS";
    mostrarChat();
    actualizarPantalla();
    if (capturarVoz()) {
      estadoChat = "GEMINI ESTA RESPONDIENDO";
      consultaActiva = true;
      if (xTaskCreate(consultarServidor, "gemini", 8192, nullptr, 1, nullptr) != pdPASS) {
        consultaActiva = false;
        estadoChat = "NO SE PUDO INICIAR LA CONSULTA";
      }
    } else estadoChat = "ERROR AL GRABAR";
  }
  mostrarChat();
}

void mostrarVista(Vista nueva) {
  vista = nueva;
  bloqueHorizontal(0, 67, 800, 354, BLANCO);
  switch (vista) {
    case Vista::Inicio:
      cara(400, 148);
      textoCentrado(400, 273, "MEMO", 9, TINTA);
      pie("PULSA EL CENTRO PARA EMPEZAR");
      break;
    case Vista::Menu:
      cara(205, 161);
      textoCentrado(205, 276, "HOLA", 5, TINTA);
      textoCentrado(205, 327, "QUE HACEMOS HOY?", 3, TINTA);
      for (int i = 0; i < 3; ++i) opcionMenu(i);
      pie("ARRIBA / ABAJO: MOVER   CENTRO: ABRIR");
      break;
    case Vista::Pomodoro:
      textoCentrado(400, 99, "POMODORO", 5, TINTA);
      ultimoSegundo = -1;
      mostrarContador();
      botonPomodoro();
      pie("CENTRO: INICIAR / PAUSAR   IZQ: VOLVER");
      break;
    case Vista::Chat:
      textoCentrado(400, 100, "CHATBOT", 5, TINTA);
      pie("BOTON: HABLAR   IZQ: VOLVER");
      mostrarChat();
      break;
    case Vista::Calendario:
      textoCentrado(400, 113, "CALENDARIO", 5, TINTA);
      bloqueHorizontal(122, 202, 556, 104, SUAVE);
      textoCentrado(400, 238, "PENDIENTE DE INTEGRAR", 4, TINTA);
      pie("IZQ: VOLVER AL MENU");
      break;
  }
}

void actualizarPomodoro() {
  if (!contando) return;
  int32_t falta = int32_t(vencimiento - millis());
  if (falta <= 0) {
    contando = false;
    restante = 0;
    if (vista == Vista::Pomodoro) {
      mostrarContador();
      botonPomodoro();
    }
    probarTono();
  } else {
    restante = uint32_t(falta);
    if (vista == Vista::Pomodoro) mostrarContador();
  }
}

void alternarPomodoro() {
  if (contando) {
    int32_t falta = int32_t(vencimiento - millis());
    restante = falta > 0 ? uint32_t(falta) : 0;
    contando = false;
  } else {
    if (restante == 0) restante = DURACION;
    vencimiento = millis() + restante;
    contando = true;
  }
  botonPomodoro();
  mostrarContador();
}

void moverMenu(int paso) {
  int anterior = seleccion;
  seleccion = (seleccion + paso + 3) % 3;
  opcionMenu(anterior);
  opcionMenu(seleccion);
}

void leerMovimiento() {
  int dx = (analogRead(JOY_X) - centroX) * (INVERTIR_X ? -1 : 1);
  int dy = (analogRead(JOY_Y) - centroY) * (INVERTIR_Y ? -1 : 1);
  if (abs(dx) < 300 && abs(dy) < 300) {
    joystickOcupado = false;
    return;
  }
  if (joystickOcupado || (abs(dx) < ZONA_MUERTA && abs(dy) < ZONA_MUERTA)) return;
  joystickOcupado = true;
  if (vista == Vista::Menu && abs(dy) >= abs(dx)) moverMenu(dy > 0 ? 1 : -1);
  else if (vista != Vista::Menu && vista != Vista::Inicio &&
           dx <= -ZONA_MUERTA && abs(dx) > abs(dy)) mostrarVista(Vista::Menu);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(BOTON_CHAT, INPUT_PULLUP);

  iniciarPantalla();
  prepararCuadro();
  rellenarBloque(0, 0, ANCHO, ALTO, BLANCO);
  texto(28, 17, "MEMO", 5, TINTA);
  texto(680, 24, "LOCAL", 3, TINTA);
  bloqueHorizontal(0, 65, 800, 2, BORDE);

  Serial.println("Deja el joystick quieto para calibrar el centro.");
  int sumaX = 0;
  int sumaY = 0;
  for (int i = 0; i < 32; ++i) {
    sumaX += analogRead(JOY_X);
    sumaY += analogRead(JOY_Y);
    delay(5);
  }
  centroX = constrain(sumaX / 32, 700, 3395);
  centroY = constrain(sumaY / 32, 700, 3395);
  iniciarAudio();
  if (chatConfigurado()) {
    colaChat = xQueueCreate(1, sizeof(RespuestaChat));
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_NOMBRE, WIFI_CLAVE);
  }
  mostrarVista(Vista::Inicio);
  Serial.println("MEMO listo. Centro del joystick para entrar.");
}

void loop() {
  enviarSilencio();
  actualizarPomodoro();
  const bool pulsarChat = botonChat.nuevaPulsacion();
  const bool pulsarCentro = centroJoystick.nuevaPulsacion();
  leerMovimiento();

  if (pulsarChat) {
    if (vista == Vista::Chat) iniciarConsulta();
    else mostrarVista(Vista::Chat);
  } else if (pulsarCentro) {
    switch (vista) {
      case Vista::Inicio: mostrarVista(Vista::Menu); break;
      case Vista::Menu: mostrarVista(DESTINOS[seleccion]); break;
      case Vista::Pomodoro: alternarPomodoro(); break;
      case Vista::Chat: iniciarConsulta(); break;
      case Vista::Calendario: mostrarVista(Vista::Menu); break;
    }
  }
  if (consultaActiva && xQueueReceive(colaChat, &respuestaChat, 0) == pdTRUE) {
    consultaActiva = false;
    if (vista == Vista::Chat) mostrarChat();
  }
  actualizarPantalla();
  delay(5);
}
