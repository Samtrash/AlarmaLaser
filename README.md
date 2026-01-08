# 🛡️ Sistema de Detección de Intrusos Perimetral IoT

**Sistema de seguridad con verificación visual basado en ESP32-CAM**

## 📋 Descripción

Sistema de alarma perimetral que crea una "cerca invisible" utilizando módulos láser y fotoresistencias (LDR). Cuando un intruso cruza la línea de protección, el sistema captura una foto y envía una alerta a Telegram. Además, incluye un dashboard web para control remoto del sistema desde cualquier dispositivo conectado a la misma red WiFi.

## ✨ Características

### Detección y Alertas
- ⚡ Detección instantánea de intrusos con 3 sectores independientes
- 📸 Captura automática de foto al detectar intrusion
- 📱 Envío de alertas y fotos vía Telegram
- 🔍 Identificación del sector vulnerado (1, 2 o 3)

### Control Remoto
- 🌐 **Dashboard web local** con interfaz moderna
- 🎥 **Video streaming en vivo** desde la ESP32-CAM
- 📸 **Captura manual de fotos** desde el navegador
- 🔴 **Activar/Desactivar alarma** remotamente
- ⏸️ **Pausar alarma** con temporizador (5, 10, 15, 30, 60 minutos)
- 📊 **Monitoreo en tiempo real** del estado de sensores
- 📋 **Registro de eventos** con historial completo

## 🔧 Hardware Necesario

### Componentes Principales
- **ESP32-CAM** (modelo AI-Thinker) - Cerebro del sistema
- **3x Módulos Láser KY-008** (5V) - Emisores de luz
- **3x Fotoresistencias (LDR)** + Resistencias 10kΩ - Sensores
- **Regulador de Voltaje L7805CV** (9V → 5V)
- **Batería de 9V** - Alimentación

### Pinout ESP32-CAM

```
Sensores:
- Sensor Sector 1: GPIO 13
- Sensor Sector 2: GPIO 15 (¡IMPORTANTE! Cambiado de GPIO 12)
- Sensor Sector 3: GPIO 14

Láseres:
- Alimentación: 5V (desde regulador L7805CV)
- GND común con ESP32

Cámara: Configuración AI-Thinker (pines definidos en código)
```

> ⚠️ **IMPORTANTE:** Se cambió el Sensor 2 del pin GPIO 12 al GPIO 15 para evitar conflictos con el "strapping pin" que afectaba el arranque del ESP32.

## 💻 Software Requerido

### Arduino IDE
1. **Instalar ESP32 Board Support:**
   - Archivo → Preferencias → URLs adicionales de gestor de tarjetas:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
   - Herramientas → Placa → Gestor de tarjetas → Buscar "ESP32" → Instalar

### Librerías Necesarias

Instala desde el Gestor de Librerías (Programa → Incluir Librería → Administrar Librerías):

- `UniversalTelegramBot` (by Brian Lough)
- `ArduinoJson` (by Benoit Blanchon) - versión 6.x
- `ESPAsyncWebServer` (by me-no-dev)
- `AsyncTCP` (by me-no-dev)

> 💡 **Nota:** Si `ESPAsyncWebServer` no aparece en el gestor, descárgala manualmente desde:
> - https://github.com/me-no-dev/ESPAsyncWebServer
> - https://github.com/me-no-dev/AsyncTCP

## 🚀 Instalación

### 1. Configurar Telegram Bot

1. Abre Telegram y busca **@BotFather**
2. Envía `/newbot` y sigue las instrucciones
3. Guarda el **Token** que te proporciona
4. Busca **@myidbot** y envía `/getid` para obtener tu **Chat ID**

### 2. Configurar Código ESP32

Abre `proyectolaser/proyectolaser.ino` y edita estas líneas:

```cpp
const char* ssid = "TU_WIFI";              // Nombre de tu red WiFi (2.4GHz)
const char* password = "TU_PASSWORD";       // Contraseña WiFi
String botToken = "TU_BOT_TOKEN";          // Token de BotFather
String chatId = "TU_CHAT_ID";              // Tu Chat ID
```

### 3. Subir Firmware

1. Conecta ESP32-CAM a tu PC
2. Selecciona en Arduino IDE:
   - Herramientas → Placa → ESP32 → **AI Thinker ESP32-CAM**
   - Puerto → (tu puerto serial)
3. Presiona el botón **IO0** en el ESP32-CAM (para modo programación)
4. Click en **Subir** ⬆️
5. Espera a que compile y suba (puede tardar 1-2 minutos)
6. Abre el **Monitor Serial** (115200 baud)
7. Presiona el botón **RESET** en el ESP32-CAM
8. **Anota la dirección IP** que aparece en el monitor

Ejemplo de salida:
```
WiFi Conectado!
Servidor Web iniciado!
IP Address: 192.168.1.100
```

## 📱 Usar el Dashboard

### Paso 1: Abrir Dashboard

1. Abre el archivo `dashboard.html` en tu navegador preferido (Chrome/Firefox/Edge)
2. Puedes hacerlo de dos formas:
   - Doble click en el archivo
   - Arrastra el archivo al navegador

### Paso 2: Conectar al ESP32

1. Ingresa la **dirección IP** del ESP32 (la que anotaste del monitor serial)
2. Click en **"Conectar"**
3. Deberías ver:
   - ✅ Estado cambia a "Conectado"
   - El video en vivo de la cámara
   - Estado de los sensores actualizándose

### Funcionalidades del Dashboard

#### 📹 Video en Vivo
- Se actualiza automáticamente
- Click en **"📸 Capturar Foto"** para tomar una foto manual
- La foto se descarga automáticamente

#### 🎛️ Controles de Alarma
- **🟢 Activar Alarma:** Habilita detección de intrusos
- **🔴 Desactivar Alarma:** Ignora todos los sensores
- **⏸️ Pausar:** Desactiva temporalmente (5, 10, 15, 30, 60 min)
  - Muestra contador regresivo
  - Se reactiva automáticamente al terminar

#### 📡 Estado de Sensores
- **✅ Normal:** Láser detectado correctamente
- **⚠️ Activado:** Láser interrumpido (posible intruso)
- Se actualiza cada 2 segundos

#### 📋 Registro de Eventos
- 🔴 Rojo: Intrusiones detectadas
- 🟢 Verde: Eventos del sistema (boot, alarma activada)
- 🟡 Amarillo: Acciones de control (desactivar, pausar)

## 🔌 Diagrama de Conexión

```
BATERÍA 9V
    │
    ├──→ L7805CV (Regulador)
    │       │
    │       ├──→ 5V (Láseres)
    │       └──→ 5V (ESP32-CAM)
    │
    └──→ GND común

SECTORES (x3):
Láser KY-008 ────→ [Espacio vigilado] ────→ LDR + 10kΩ ────→ GPIO (13/15/14)
                                                      │
                                                     GND

DIVISOR DE VOLTAJE (por cada LDR):
5V ──→ LDR ──→ 10kΩ ──→ GND
            │
            └──→ GPIO ESP32
```

## 📡 API REST Endpoints

El ESP32 expone los siguientes endpoints:

### GET /status
Obtiene el estado actual del sistema

**Respuesta:**
```json
{
  "enabled": true,
  "paused_until": 0,
  "sensors": {
    "sector1": true,
    "sector2": true,
    "sector3": false
  },
  "uptime": 12345,
  "last_intrusion": 12000
}
```

### POST /alarm/enable
Activa la alarma

### POST /alarm/disable
Desactiva la alarma

### POST /alarm/pause?minutes=X
Pausa la alarma X minutos (1-60)

### GET /photo/capture
Captura y devuelve una foto (JPEG)

### GET /stream
Stream de video MJPEG continuo

### GET /logs
Retorna los últimos 50 eventos

## 🛠️ Troubleshooting

### El ESP32 no arranca
- ✅ Verifica que el sensor 2 esté en **GPIO 15** (NO GPIO 12)
- ✅ Asegúrate de tener alimentación estable de 5V
- ✅ Revisa conexión GND común

### No se conecta al WiFi
- ✅ Verifica que tu WiFi sea **2.4GHz** (ESP32 no soporta 5GHz)
- ✅ Revisa credenciales (ssid y password) en el código
- ✅ Asegúrate de que el ESP32 esté dentro del rango del WiFi

### Dashboard no conecta
- ✅ Verifica que estés en la **misma red WiFi** que el ESP32
- ✅ Revisa la IP en el Monitor Serial (puede cambiar al reiniciar)
- ✅ Intenta hacer ping a la IP: `ping 192.168.1.X`
- ✅ Desactiva firewall temporalmente para probar

### No recibo notificaciones de Telegram
- ✅ Verifica el Token y Chat ID
- ✅ Asegúrate de haber iniciado conversación con tu bot (envía `/start`)
- ✅ Revisa que el ESP32 tenga conexión a internet

### Video streaming no funciona
- ✅ Usa Chrome o Firefox (mejor compatibilidad)
- ✅ Verifica que la cámara esté bien conectada
- ✅ Reinicia el ESP32 y vuelve a conectar

### Sensores siempre activados
- ✅ Verifica que los láseres apunten correctamente a los LDRs
- ✅ Revisa el divisor de voltaje (LDR + resistencia 10kΩ)
- ✅ Ajusta posición para evitar luz ambiental fuerte

## 🔄 Migración a Cloud (Opcional - Fase 2)

Actualmente el sistema funciona **localmente** (misma WiFi). Si deseas acceder desde **cualquier lugar del mundo**, consulta el archivo `cloud_options.md` que explica cómo migrar a Firebase o Supabase.

## 📄 Licencia

Proyecto académico - UNAP CICLO VIII IoT

## 👨‍💻 Autor

Sistema Centinela - Proyecto IoT 2026

---

**🛡️ Protege tu perímetro con tecnología IoT de última generación**
