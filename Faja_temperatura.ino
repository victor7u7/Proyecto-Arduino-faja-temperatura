// ======================= PROYECTO CREADO POR Victor =======================
// ======================= PROYECTO CREADO POR Victor =======================
// ======================= B Y M E C H A T R O N I C S=======================
// ======================= B Y M E C H A T R O N I C S=======================
// ======================= https://bymechatronics.com.mx/=======================

// ======================= LIBRERÍAS =======================
// Librerías necesarias para manejar WiFi, servidor web y WebSockets en el ESP32
#include <ESPAsyncWebServer.h>  // Permite crear un servidor web asíncrono (no bloqueante)
#include <AsyncTCP.h>           // Proporciona soporte TCP asíncrono necesario para WebSocket

#include <WiFi.h>               // Maneja las funciones de red WiFi (modo AP o estación)


// ======================= SENSOR DE TEMPERATURA =======================
#include <OneWire.h>            //LIBRERIAS SENSOR DE TEMPERATURA
#include <DallasTemperature.h>  //LIBRERIAS SENSOR DE TEMPERATURA
// ======================= CONFIGURACIÓN DE PINES =======================
const int ativaVibracionIN1 = 25; // Pin que controlará la vibración Entrada IN1 del L298N
const int ativaVibracionIN2 = 27; // Pin que controlará la vibración Entrada IN2 del L298N

const int ativaTemp = 26;     // Pin que controlará la salida de temperatura (de ejemplo usamos igual led)

#define pinDS18B20 4      // DATA del sensor al pin 4 del ESP32 (CABLE COLOR AMARILLO)
OneWire oneWire(pinDS18B20);
DallasTemperature sensors(&oneWire);

// ======================= VARIABLES DE CRONÓMETRO =======================
// Se usan para contar el tiempo que la vibración está activa
unsigned long vibStartTime = 0;  // Tiempo inicial cuando empieza la vibración
unsigned long vibElapsed = 0;    // Tiempo transcurrido total
bool vibRunning = false;         // Indica si el cronómetro está corriendo o no

// ======================= CONFIGURACIÓN WIFI (MODO AP) =======================
// El ESP32 actuará como punto de acceso (Access Point) para que el usuario se conecte directamente (desde el navegador ip: 192.168.4.1)
const char* ssid = "VibeGlow";     // Nombre de la red WiFi (nombre modificable)
const char* password = "12345678";      // Contraseña de conexión (modificable desdee aqui)
//192.168.4.1

// ======================= VARIABLES GLOBALES =======================
// Variables que representan el estado actual de los sistemas
float temperatura = 25.0;   // Valor inicial de temperatura (ejemplo del variable)
bool tempOn = false;        // Estado del control de temperatura (desactivado para ajustar al gusto)
bool vibOn = true;          // Estado del control de vibración (iniciamos siempre prendido para que la faja de masaje siempre al reiniciar el esp32 o desconectar)

// ======================= SERVIDOR WEB Y WEBSOCKET =======================
// Servidor HTTP en el puerto 80
AsyncWebServer server(80);
// WebSocket para comunicación bidireccional en tiempo real con el navegador
AsyncWebSocket ws("/ws");

// ======================= PÁGINA HTML (INTERFAZ WEB) =======================
// Se guarda dentro del ESP32 usando PROGMEM (memoria de programa)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>vibeGlow</title>
  <style>
    /* Estilos generales de la página */
    body {
      height: 100%;
      margin: 0;
      padding: 0;

      font-family: Arial;
      background: linear-gradient(0deg, rgba(224, 227, 235, 1) 0%, rgba(192, 198, 217, 1) 10%, rgba(0, 207, 255, 1) 25%, rgba(60, 109, 240, 1) 47%, rgba(139, 75, 255, 1) 72%, rgba(11, 14, 26, 1) 100%);
      color: #fff;
      text-align: center;
      background-repeat: no-repeat; 
      background-attachment: fixed; /* Hace que el gradiente cubra toda la pantalla */
      font-size:  1.1rem;
    }

    h2 { color: #3c6df0;
font-size: 3.5rem;
font-weight: 800;
font-family: "Arial Black", Gadget, sans-serif;

text-shadow: -0.71px 0.71px 0 rgba(11, 14, 26, 0.5), 
-1.41px 1.41px 0 rgba(14, 17, 28, 0.5),
 -2.12px 2.12px 0 rgba(17, 20, 30, 0.5), 
 -2.83px 2.83px 0 rgba(20, 23, 32, 0.5), 
 -3.54px 3.54px 0 rgba(23, 25, 34, 0.5), 
 -4.24px 4.24px 0 rgba(26, 28, 36, 0.5),
  -4.95px 4.95px 0 rgba(29, 31, 38, 0.5), 
  -5.66px 5.66px 0 rgba(33, 34, 39, 0.5), 
  -6.36px 6.36px 0 rgba(36, 37, 41, 0.5), 
  -7.07px 7.07px 0 rgba(39, 40, 43, 0.5),
   -7.78px 7.78px 0 rgba(42, 42, 45, 0.5), 
   -8.49px 8.49px 0 rgba(45, 45, 47, 0.5), 
   -9.19px 9.19px 0 rgba(48, 48, 49, 0.5),
    -9.90px 9.90px 0 rgba(51, 51, 51, 0.5);
 }

    /* Cajas donde se muestran los controles switchs */
    .box {
      background: #222;
      border-radius: 40px;
      padding: 2.3rem;
      margin: 10px;
      display: inline-block;
    }

    /* Color diferente cuando está activado */
    .box.active {
      background: #2ecc71;
    }

    /* Diseño del interruptor tipo slider */
    .switch {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 34px;
    }

    /* El input real (checkbox) se oculta */
    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }

    /* Parte visual del interruptor */
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background-color: #444;
      transition: .4s;
      border-radius: 34px;
    }

    /* Círculo blanco del interruptor */
    .slider:before {
      position: absolute;
      content: "";
      height: 26px; width: 26px;
      left: 4px; bottom: 4px;
      background-color: white;
      transition: .4s;
      border-radius: 50%;
    }

    /* Cambia de color al activarse */
    input:checked + .slider {
      background-color: #2196F3;
    }

    /* Mueve el círculo a la derecha al activarse */
    input:checked + .slider:before {
      transform: translateX(26px);
    }
    
  </style>
</head>

<body>
  <h2>VibeGlow</h2>
 
  <div>Temperatura actual: <span id="tempValue">-- °C</span></div>

  <!-- Sección de control de temperatura -->
  <div class="box" id="tempBox">
    <h3>Temperatura</h3>
    <label class="switch">
      <input type="checkbox" id="tempSwitch">
      <span class="slider"></span>
    </label>
  </div>

  <!-- Sección de control de vibración -->
  <div class="box active" id="vibBox">
    <h3>Vibración</h3>
    <label class="switch">
      <input type="checkbox" id="vibSwitch" checked>
      <span class="slider"></span>
    </label>
  </div>

  <h3>​🕥​ Tiempo de vibración: <span id="vibTime">00:00</span></h3>

  <script>
    // ==================== VARIABLES DEL LADO DEL NAVEGADOR ====================
    const tempValueEl = document.getElementById('tempValue');
    const tempSwitch = document.getElementById('tempSwitch');
    const vibSwitch = document.getElementById('vibSwitch');
    const tempBox = document.getElementById('tempBox');
    const vibBox = document.getElementById('vibBox');
    const vibTimeEl = document.getElementById('vibTime');

    let vibActive = true;
    let vibSeconds = 0;

    // ==================== CONEXIÓN WEBSOCKET ====================
    // Se abre un canal de comunicación con el ESP32
    const ws = new WebSocket(`ws://${location.hostname}/ws`);

    // Cuando el ESP32 envía datos, el navegador los recibe aquí
    ws.onmessage = (event) => {
      // Los datos llegan como texto JSON ? se convierten en objeto JS
      const data = JSON.parse(event.data);

      // Se actualizan los elementos visuales según el estado recibido
      tempValueEl.innerText = data.tempOn ? (data.temperatura.toFixed(1) + " °C") : "-- °C";
      tempSwitch.checked = data.tempOn;
      vibSwitch.checked = data.vibOn;
      tempBox.classList.toggle('active', data.tempOn);
      vibBox.classList.toggle('active', data.vibOn);
      vibActive = data.vibOn;
      vibSeconds = data.vibTime;
    };

    // Función para formatear el tiempo como mm:ss
    function formatTime(seconds) {
      const m = Math.floor(seconds / 60);
      const s = seconds % 60;
      return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;

    }

    // Contador de tiempo que aumenta cada segundo si la vibración está activa
    setInterval(() => {
      if (vibActive) vibSeconds++;
      vibTimeEl.innerText = formatTime(vibSeconds);
    }, 1000);

    // Envía el estado actual al ESP32 en formato JSON
    function sendState() {
      const msg = { tempOn: tempSwitch.checked, vibOn: vibSwitch.checked };
      ws.send(JSON.stringify(msg)); // Convierte el objeto a JSON y lo manda por WebSocket
    }

    // Detecta cambios en los interruptores y envía el nuevo estado
    tempSwitch.addEventListener('change', sendState);
    vibSwitch.addEventListener('change', sendState);
  </script>
</body>
</html>
)rawliteral";

// ======================= FUNCIÓN PARA NOTIFICAR CLIENTES =======================
// Envía a todos los navegadores conectados el estado actual en formato JSON
void notifyClients() {
  // JSON manual: se arma una cadena de texto con los valores
  String json = "{\"temperatura\":" + String(temperatura, 1) +
                ",\"tempOn\":" + String(tempOn ? "true" : "false") +
                ",\"vibOn\":" + String(vibOn ? "true" : "false") +
                ",\"vibTime\":" + String(vibElapsed / 1000) + "}";
  ws.textAll(json);  // Envia el JSON por WebSocket a todos los clientes
}

// ======================= MANEJO DE MENSAJES DESDE EL NAVEGADOR =======================
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) /* verifica que el mensaje está completo (no dividido en partes).*/
  {
    data[len] = 0;            // Termina la cadena
    String msg = (char*)data; // Convierte los datos a String

    // Busca dentro del JSON recibido las palabras clave y actualiza variables
    if (msg.indexOf("\"tempOn\":true") != -1) tempOn = true;
    else if (msg.indexOf("\"tempOn\":false") != -1) tempOn = false;

    if (msg.indexOf("\"vibOn\":true") != -1) vibOn = true;
    else if (msg.indexOf("\"vibOn\":false") != -1) vibOn = false;

    // Envía el nuevo estado a todos los clientes conectados
    notifyClients();
  }
}

// ======================= EVENTOS DEL WEBSOCKET =======================
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {         // Cuando un cliente se conecta
    Serial.println("Cliente conectado");
    notifyClients();                    // Le manda el estado actual
  } else if (type == WS_EVT_DATA) {     // Cuando llegan datos desde el cliente
    handleWebSocketMessage(arg, data, len);
  }
}

// ======================= CONFIGURACIÓN INICIAL =======================
void setup() {
 
  Serial.begin(115200);

  pinMode(ativaVibracionIN1, OUTPUT);
  pinMode(ativaVibracionIN2, OUTPUT);
  pinMode(ativaTemp, OUTPUT);
  
  

  sensors.begin(); // Inicializa el DS18B20
  






  // El ESP32 crea su propia red WiFi
  WiFi.softAP(ssid, password); // nombre y contraseña
  Serial.print("WiFi AP iniciado. IP: ");
  Serial.println(WiFi.softAPIP());

  // Configuración del servidor
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Cuando alguien entra a la raíz "/", se envía la página HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  
  });

  server.begin();
  vibOn = true;
  Serial.println("Servidor iniciado correctamente");
}

// ======================= BUCLE PRINCIPAL =======================
void loop() {
  // Actualiza las salidas físicas del ESP32
 if (vibOn) {
  digitalWrite(ativaVibracionIN1, HIGH);
  digitalWrite(ativaVibracionIN2, LOW); // Gira en un solo sentido
} else {
  digitalWrite(ativaVibracionIN1, LOW); //se apagan es decir se manda un bajo para que se apaguen
  digitalWrite(ativaVibracionIN2, LOW); //se apagan es decir se manda un bajo para que se apaguen
}
  digitalWrite(ativaTemp, tempOn ? HIGH : LOW); // sirve para que cada que este prendida la opcion de temperatura se ejecute un prendido o apagado (interpretado por led)

  // Cronómetro real: mide cuánto tiempo ha estado activa la vibración
  if (vibOn) { //entra luego luego ya que la vibracion al iniciar siempre prende
    if (!vibRunning) {
      vibStartTime = millis() - vibElapsed;
      vibRunning = true;
    }
    vibElapsed = millis() - vibStartTime;
  } else {
    vibRunning = false;
    vibElapsed = 0;
  }

  // Cada 500 ms actualiza el estado a los clientes (refresca la pagina sin volver a cargar asegurando que todos vean lo mismo)
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 500) {
    lastUpdate = millis();

    // Simulación de temperatura 
    if (tempOn) {
  sensors.requestTemperatures();          // Pide lectura del sensor
  float t = sensors.getTempCByIndex(0);   // Obtiene temperatura del primer sensor

  if (t != DEVICE_DISCONNECTED_C) {
    temperatura = t; // Guarda valor real
  }
}

    // Enviar datos actualizados en formato JSON a la página web
notifyClients();
  }
}
