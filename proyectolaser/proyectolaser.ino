#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// ==========================================
// DATOS DE CONEXIÓN (¡EDÍTALOS!)
// ==========================================
const char* ssid = "LAPTOP9238";
const char* password = "asdasdasd";
String botToken = "8034809258:AAHzoqIiSZERm3UAE4ojCibEo4IJONctHBw";  // El token que te dio BotFather
String chatId = "7349079243";        // Tu ID obtenido de IDBot

// ==========================================
// CONFIGURACIÓN DE PINES
// ==========================================
// Sensores (Lógica inversa: LOW = Alarma activada/Láser cortado)
const int pinSensor1 = 13;
const int pinSensor2 = 15;  // Cambiado de 12 a 15 (evita conflicto con strapping pin)
const int pinSensor3 = 14; 

// Modelo de Cámara: AI-THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ==========================================
// VARIABLES GLOBALES PARA MANEJO DE FOTOS
// ==========================================
WiFiClientSecure clientTCP;
UniversalTelegramBot bot(botToken, clientTCP);

// Variables auxiliares para subir la foto byte por byte
camera_fb_t * fb = NULL;
size_t currentByte;
size_t totalBytes;

// Funciones "Callback" que pide la librería (Aquí estaba el error antes)
bool isMoreDataAvailable() {
  return (currentByte < totalBytes);
}

byte getNextByte() {
  currentByte++;
  return fb->buf[currentByte - 1];
}

// ==========================================
// SERVIDOR WEB Y CONTROL REMOTO
// ==========================================
AsyncWebServer server(80);

// Estado de la alarma
bool alarmEnabled = true;
unsigned long pausedUntil = 0;
unsigned long lastIntrusion = 0;

// Registro de eventos (últimos 50)
struct Event {
  String type;
  int sector;
  unsigned long timestamp;
};
Event eventLog[50];
int eventIndex = 0;

void addEvent(String type, int sector = 0) {
  eventLog[eventIndex].type = type;
  eventLog[eventIndex].sector = sector;
  eventLog[eventIndex].timestamp = millis() / 1000;
  eventIndex = (eventIndex + 1) % 50;
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Desactivar detector de caídas de tensión
  Serial.begin(115200);

  // 1. Configurar Pines de Sensores
  pinMode(pinSensor1, INPUT);
  pinMode(pinSensor2, INPUT);
  pinMode(pinSensor3, INPUT);

  // 2. Configurar Cámara
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Ajuste de calidad
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA; // 640x480
    config.jpeg_quality = 10;          // Menor número = Mejor calidad
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error iniciando cámara: 0x%x", err);
    return;
  }

  // 3. Conectar a WiFi
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Certificado HTTPS necesario
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");
  
  // Mensaje de prueba al iniciar
  bot.sendMessage(chatId, "🟢 Sistema Centinela: ONLINE y VIGILANDO", "");
  addEvent("boot");
  
  // ==========================================
  // CONFIGURAR SERVIDOR WEB Y ENDPOINTS
  // ==========================================
  
  // Endpoint: GET /status - Estado del sistema
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->addInterestingHeader("Access-Control-Allow-Origin");
    
    String json = "{";
    json += "\"enabled\":" + String(alarmEnabled ? "true" : "false") + ",";
    json += "\"paused_until\":" + String(pausedUntil) + ",";
    json += "\"sensors\":{";
    json += "\"sector1\":" + String(digitalRead(pinSensor1) == HIGH ? "true" : "false") + ",";
    json += "\"sector2\":" + String(digitalRead(pinSensor2) == HIGH ? "true" : "false") + ",";
    json += "\"sector3\":" + String(digitalRead(pinSensor3) == HIGH ? "true" : "false");
    json += "},";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"last_intrusion\":" + String(lastIntrusion);
    json += "}";
    
    request->send(200, "application/json", json);
  });
  
  // Endpoint: POST /alarm/enable
  server.on("/alarm/enable", HTTP_POST, [](AsyncWebServerRequest *request){
    request->addInterestingHeader("Access-Control-Allow-Origin");
    alarmEnabled = true;
    pausedUntil = 0;
    addEvent("alarm_enabled");
    request->send(200, "application/json", "{\"success\":true}");
  });
  
  // Endpoint: POST /alarm/disable
  server.on("/alarm/disable", HTTP_POST, [](AsyncWebServerRequest *request){
    request->addInterestingHeader("Access-Control-Allow-Origin");
    alarmEnabled = false;
    addEvent("alarm_disabled");
    request->send(200, "application/json", "{\"success\":true}");
  });
  
  // Endpoint: POST /alarm/pause?minutes=X
  server.on("/alarm/pause", HTTP_POST, [](AsyncWebServerRequest *request){
    request->addInterestingHeader("Access-Control-Allow-Origin");
    int minutes = 5;
    if(request->hasParam("minutes")) {
      minutes = request->getParam("minutes")->value().toInt();
      if(minutes > 60) minutes = 60;
      if(minutes < 1) minutes = 1;
    }
    pausedUntil = millis() + (minutes * 60 * 1000);
    addEvent("alarm_paused", minutes);
    
    String json = "{\"success\":true,\"paused_until\":" + String(pausedUntil) + "}";
    request->send(200, "application/json", json);
  });
  
  // Endpoint: GET /photo/capture - Captura manual de foto
  server.on("/photo/capture", HTTP_GET, [](AsyncWebServerRequest *request){
    request->addInterestingHeader("Access-Control-Allow-Origin");
    
    camera_fb_t * photo = esp_camera_fb_get();
    if(!photo) {
      request->send(500, "text/plain", "Error capturando foto");
      return;
    }
    
    addEvent("manual_photo");
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/jpeg", photo->buf, photo->len);
    response->addHeader("Content-Disposition", "inline; filename=capture.jpg");
    request->send(response);
    esp_camera_fb_return(photo);
  });
  
  // Endpoint: GET /logs - Registro de eventos
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request){
    request->addInterestingHeader("Access-Control-Allow-Origin");
    
    String json = "[";
    for(int i = 0; i < 50; i++) {
      int idx = (eventIndex + i) % 50;
      if(eventLog[idx].timestamp > 0) {
        if(json.length() > 1) json += ",";
        json += "{";
        json += "\"type\":\"" + eventLog[idx].type + "\",";
        json += "\"sector\":" + String(eventLog[idx].sector) + ",";
        json += "\"timestamp\":" + String(eventLog[idx].timestamp);
        json += "}";
      }
    }
    json += "]";
    
    request->send(200, "application/json", json);
  });
  
  // Endpoint: GET /stream - Streaming de video
  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request){
    request->addInterestingHeader("Access-Control-Allow-Origin");
    AsyncWebServerResponse *response = request->beginChunkedResponse("multipart/x-mixed-replace; boundary=frame", 
      [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        camera_fb_t * frame = esp_camera_fb_get();
        if(!frame) return 0;
        
        String header = "--frame\r\nContent-Type: image/jpeg\r\n\r\n";
        if(index == 0) {
          memcpy(buffer, header.c_str(), header.length());
          memcpy(buffer + header.length(), frame->buf, min((size_t)frame->len, maxLen - header.length()));
          size_t len = header.length() + min((size_t)frame->len, maxLen - header.length());
          esp_camera_fb_return(frame);
          return len;
        }
        esp_camera_fb_return(frame);
        return 0;
      });
    request->send(response);
  });
  
  // Habilitar CORS para todos los endpoints
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  
  server.begin();
  Serial.println("Servidor Web iniciado!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ==========================================
// LOOP PRINCIPAL
// ==========================================
void loop() {
  // Verificar si la pausa ha expirado
  if(pausedUntil > 0 && millis() > pausedUntil) {
    pausedUntil = 0;
    alarmEnabled = true;
    addEvent("alarm_resumed");
  }
  
  // Solo detectar intrusos si la alarma está habilitada y no pausada
  if(!alarmEnabled || (pausedUntil > 0 && millis() < pausedUntil)) {
    delay(100);
    return;
  }
  
  // Leemos los sensores (LOW significa que el láser se cortó)
  bool intruso1 = digitalRead(pinSensor1) == LOW; 
  bool intruso2 = digitalRead(pinSensor2) == LOW;
  bool intruso3 = digitalRead(pinSensor3) == LOW;

  if (intruso1 || intruso2 || intruso3) {
    Serial.println("¡ALERTA DE INTRUSO!");
    lastIntrusion = millis() / 1000;
    
    String mensaje = "⚠️ ALERTA: Intruso en ";
    if(intruso1) {
      mensaje += "[Sector 1] ";
      addEvent("intrusion", 1);
    }
    if(intruso2) {
      mensaje += "[Sector 2] ";
      addEvent("intrusion", 2);
    }
    if(intruso3) {
      mensaje += "[Sector 3] ";
      addEvent("intrusion", 3);
    }

    enviarFoto(mensaje);
    addEvent("photo_sent");
    
    Serial.println("Esperando 10 segundos para no saturar...");
    delay(10000); 
  }
  
  delay(100); // Pequeña pausa para estabilidad
}

// ==========================================
// FUNCIÓN PARA ENVIAR FOTO
// ==========================================
void enviarFoto(String caption) {
  fb = NULL; // Limpiamos puntero anterior
  fb = esp_camera_fb_get(); // Tomamos foto
  
  if(!fb) {
    Serial.println("Fallo al capturar foto");
    bot.sendMessage(chatId, "Error: Cámara no respondió", "");
    return;
  }

  // Preparamos las variables globales para los callbacks
  currentByte = 0;
  totalBytes = fb->len;
  
  Serial.println("Enviando foto a Telegram...");
  
  // Esta es la función corregida que daba error antes:
  bot.sendPhotoByBinary(chatId, "image/jpeg", fb->len,
                        isMoreDataAvailable, 
                        getNextByte,
                        nullptr, nullptr);

  Serial.println("Foto enviada!");
  esp_camera_fb_return(fb); // Liberamos memoria de la cámara
}