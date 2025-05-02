// Referencias código base:                                                                                                                                                //
// - esp32cam-gdrive : https://github.com/gsampallo/esp32cam-gdrive                                                                                           //
// - ESP32 CAM Send Images to Google Drive, IoT Security Camera : https://www.electroniclinic.com/esp32-cam-send-images-to-google-drive-iot-security-camera/  //
// - esp32cam-google-drive : https://github.com/RobertSasak/esp32cam-google-drive                                                                             //


//Librerías
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "esp_camera.h"
#include <eloquent_esp32cam.h> // Librería para detección de movimiento (o cambios entre imágenes)
#include <eloquent_esp32cam/motion/detection.h> //idem
#include <ESP_Google_Sheet_Client.h>

using eloq::camera;
using eloq::motion::detection;

//======================================== CAMERA_MODEL_AI_THINKER GPIO.
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
//

//WiFi
const char* ssid = "";
const char* password = "";
//
// Google Drive
String myDeploymentID = "AKfycbzex5jxnHPtLETAnaWaZPLJGgKaIUtjOReyHtZ_axSMnVVTfiyrjPlmJhEYO5QPSqZX";
String myMainFolderName = "Fotos-Capturadas-Izquierdo";
//

WiFiClientSecure client; // Initialize WiFiClientSecure.

// Ultrasónico

// Variables para el sensor ultrasónico

#define PROJECT_ID "comederosmart"
#define CLIENT_EMAIL "smartcom@comederosmart.iam.gserviceaccount.com"
#define SOUND_SPEED 0.034 // cm/s

const char PRIVATE_KEY[] PROGMEM =
  "-----BEGIN PRIVATE KEY-----\n"
  "-----END PRIVATE KEY-----\n";

const char spreadsheetId[] = "";

// Timer 
unsigned long lastTime = 0;
unsigned long timerDelay = 20000;
// Pins for HC-SR04
const int trigPin = 15;
const int echoPin = 14;
long duration;
float distanceCm;
const char* ntpServer = "pool.ntp.org";
unsigned long epochTime;

//Funcion que obtiene el tiempo actual
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return (0);
  }
  time(&now);
  return now;
}

void tokenStatusCallback(TokenInfo info);

//========================================

//________________________________________________________________________________ Test_Con()
// This subroutine is to test the connection to "script.google.com".
void Test_Con() {
  const char* host = "script.google.com";
  while(1) {
    Serial.println("-----------");
    Serial.println("Connection Test...");
    Serial.println("Connect to " + String(host));
  
    client.setInsecure();
  
    if (client.connect(host, 443)) {
      Serial.println("Connection successful.");
      Serial.println("-----------");
      client.stop();
      break;
    } else {
      Serial.println("Connected to " + String(host) + " failed.");
      Serial.println("Wait a moment for reconnecting.");
      Serial.println("-----------");
      client.stop();
    }
  
    delay(1000);
  }
}
//________________________________________________________________________________ 
void SendCapturedPhotos() {
  const char* host = "script.google.com";
  Serial.println();
  Serial.println("-----------");
  Serial.println("Connect to " + String(host));

  client.setInsecure();

  //---------------------------------------- El proceso de conexión, captura y envío de fotos a Google Drive.
  if (client.connect(host, 443)) {
    Serial.println("Connection successful.");

    //.............................. Capturar foto usando camera.capture()
    Serial.println();
    Serial.println("Taking a photo...");

    // Capturamos el fotograma
    Eloquent::Error::Exception& captureException = camera.capture();
    if (captureException) {
      Serial.println("Camera capture failed.");
      return;
    }

    // Acceder al buffer de la imagen y tamaño
    const uint8_t* input = camera.frame->buf;   // Puntero al buffer de la imagen
    int fbLen = camera.frame->len;               // Tamaño del buffer

    Serial.println("Taking a photo was successful.");
    Serial.println("Size: " + String(fbLen) + " bytes");

    //.............................. Enviar imagen a Google Drive
    Serial.println();
    Serial.println("Sending image to Google Drive.");
    
    String url = "/macros/s/" + myDeploymentID + "/exec?folder=" + myMainFolderName;

    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Transfer-Encoding: chunked");
    client.println();

    int chunkSize = 3 * 1000; // Debe ser múltiplo de 3
    int chunkBase64Size = base64_enc_len(chunkSize);
    char output[chunkBase64Size + 1];

    Serial.println();
    int chunk = 0;
    for (int i = 0; i < fbLen; i += chunkSize) {
      int l = base64_encode(output, (char*)input, min(fbLen - i, chunkSize));
      client.print(l, HEX);
      client.print("\r\n");
      client.print(output);
      client.print("\r\n");
      delay(100);
      input += chunkSize;
      Serial.print(".");
      chunk++;
      if (chunk % 50 == 0) {
        Serial.println();
      }
    }
    client.print("0\r\n");
    client.print("\r\n");

    // Devolver el frame de la cámara
    camera.free();

    //.............................. 

    //.............................. Esperar respuesta.
    Serial.println("Waiting for response.");
    long int StartTime = millis();
    while (!client.available()) {
      Serial.print(".");
      delay(100);
      if ((StartTime + 10 * 1000) < millis()) {
        Serial.println();
        Serial.println("No response.");
        break;
      }
    }
    Serial.println();
    while (client.available()) {
      Serial.print(char(client.read()));
    }
    //.............................. 

  }
  else {
    Serial.println("Connected to " + String(host) + " failed."); 
  }
  //---------------------------------------- 

  Serial.println("-----------");

  client.stop();
}

void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_error) {
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
  } else {
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    Serial.println(GSheet.errorReason());  // Imprimir la razón del error
  }
}


void setup() {
 // Disable brownout detector.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  Serial.println();
  delay(1000);

  // Setting the ESP32 WiFi to station mode.
  Serial.println();
  Serial.println("Setting the ESP32 WiFi to station mode.");

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  //---------------------------------------- Set the camera ESP32 CAM.
  Serial.println();
  Serial.println("Set the camera ESP32 CAM...");
  
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
  config.frame_size = FRAMESIZE_QVGA;
   config.jpeg_quality = 10;  //0-63 lower number means higher quality
  config.fb_count = 1;
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println();
    Serial.println("Restarting the ESP32 CAM.");
    delay(100);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();

  // Selectable camera resolution details :
  // -UXGA   = 1600 x 1200 pixels
  // -SXGA   = 1280 x 1024 pixels
  // -XGA    = 1024 x 768  pixels
  // -SVGA   = 800 x 600   pixels
  // -VGA    = 640 x 480   pixels
  // -CIF    = 352 x 288   pixels
  // -QVGA   = 320 x 240   pixels
  // -HQVGA  = 240 x 160   pixels
  // -QQVGA  = 160 x 120   pixels
  s->set_framesize(s, FRAMESIZE_QVGA);  //--> UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

  Serial.println("Setting the camera successfully.");
  Serial.println();

  delay(100);

  Test_Con();
  delay(200);

    // configure motion detection
    // the higher the stride, the faster the detection
    // the higher the stride, the lesser granularity
  detection.stride(1); //Lo dejamos en 1 porque ya es muy chica la imagen
    // the higher the threshold, the lesser sensitivity
    // (at pixel level)
  detection.threshold(44); //a ajustar según que tanto cambio queremos detectar
    // the higher the threshold, the lesser sensitivity
    // (at image level, from 0 to 1)
  detection.ratio(0.35); //ídem arriba

  configTime(0, 0, ntpServer);
  //Pines del sensor ultrasónico
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //Configuración inicial de Google Sheets
  GSheet.setTokenCallback(tokenStatusCallback);
  GSheet.setPrerefreshSeconds(10 * 60); 
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
  
}

/**
 *
 */
void loop() {
    // capture picture
  if (!camera.capture().isOk()) {
    Serial.println(camera.exception.toString());
    return;
  }
      // run motion detection
  if (!detection.run().isOk()) {
    Serial.println(detection.exception.toString());
    return;
  }

    // on motion, perform action
  if (detection.triggered()){
    Serial.println("Motion detected!");
    SendCapturedPhotos();
  }else{
    Serial.println("No motion");
  }
  delay(500);

   //Bool para chequear la conexión con el GS
  
  
  bool ready = GSheet.ready();

  if (ready && millis() - lastTime > timerDelay) {
    lastTime = millis();

    FirebaseJson response;

    Serial.println("\nAppend spreadsheet values...");
    Serial.println("----------------------------");

    FirebaseJson valueRange;
    
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distanceCm = duration * SOUND_SPEED / 2;

    epochTime = getTime();

    // Preparar el rango de datos para enviar a Google Sheets
    valueRange.add("majorDimension", "ROWS");

    // Comprobar la distancia y escribir en la celda F2
    if (distanceCm >= 8) {
      valueRange.set("values/[0]/[0]", "Bajo");
    } else if((distanceCm > 5) && (distanceCm < 8)) {
      valueRange.set("values/[0]/[0]", "Medio");
    } else{
      valueRange.set("values/[0]/[0]", "Alto");
    }

    // Escribir el valor en la celda F2
    bool success = GSheet.values.update(&response /* respuesta devuelta */, spreadsheetId /* ID de la hoja de cálculo */, "Uno!F2" /* celda a escribir */, &valueRange /* datos a escribir */);

    Serial.println(ESP.getFreeHeap());
  }
}
