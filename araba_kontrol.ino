#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <U8g2lib.h>

// --- PİN TANIMLAMALARI ---
// Motor Sürücü (L298N)
#define IN1 3
#define IN2 4
#define IN3 5
#define IN4 6

// LED'ler
#define LED1 22
#define LED2 23
#define LED3 24
#define LED4 25

// Servo Motorlar
Servo servo1, servo2, servo3, servo4, servo5, servo6;
int servoPins[6] = {7, 8, 10, 11, 12, 13};

// Sıcaklık Sensörleri (DS18B20)
OneWire oneWire1(27); DallasTemperature sensor1(&oneWire1);
OneWire oneWire2(28); DallasTemperature sensor2(&oneWire2);
OneWire oneWire3(29); DallasTemperature sensor3(&oneWire3);

// MPU6050 ve OLED Ekran
Adafruit_MPU6050 mpu;
U8G2_SSD1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
bool mpuReady = false;

// Zamanlayıcı
unsigned long lastSendTime = 0;
const int sendInterval = 1000;

void setup() {
  Serial.begin(115200); 

  // Motor ve LED Pin Ayarları
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(LED1, OUTPUT); pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT); pinMode(LED4, OUTPUT);

  // Araba ilk açıldığında durduğu için LED'leri YAK
  setCarLEDs(true);

  // Servoları Başlat
  servo1.attach(servoPins[0]); servo2.attach(servoPins[1]);
  servo3.attach(servoPins[2]); servo4.attach(servoPins[3]);
  servo5.attach(servoPins[4]); servo6.attach(servoPins[5]);

  // Sensörleri Başlat
  sensor1.begin(); sensor2.begin(); sensor3.begin();
  sensor1.setWaitForConversion(false);
  sensor2.setWaitForConversion(false);
  sensor3.setWaitForConversion(false);
  sensor1.requestTemperatures();
  sensor2.requestTemperatures();
  sensor3.requestTemperatures();
  
  Wire.begin();
  mpuReady = mpu.begin();
  if (!mpuReady) {
    Serial.println("MPU6050 Bulunamadi!");
  }
  
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
}

void loop() {
  // Web Sitesinden Gelen Komutları Oku
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }

  // Sensör Verilerini Gönder
  if (millis() - lastSendTime >= sendInterval) {
    lastSendTime = millis();
    sendSensorData();
    updateDisplay();
  }
}

// --- LED YÖNETİM FONKSİYONU ---
void setCarLEDs(bool durum) {
  if (durum == true) { // Duruyor = LED'ler Yansın
    digitalWrite(LED1, HIGH); digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH); digitalWrite(LED4, HIGH);
  } else { // Hareket Ediyor = LED'ler Sönsün
    digitalWrite(LED1, LOW); digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW); digitalWrite(LED4, LOW);
  }
}

// --- KOMUT İŞLEME VE YÖN KONTROLÜ ---
void processCommand(String cmd) {
  // Örnek Komutlar: 
  // "MOT:F" (İleri), "MOT:B" (Geri), "MOT:L" (Sol), "MOT:R" (Sağ), "MOT:S" (Dur)

  if (cmd.startsWith("MOT:")) {
    char dir = cmd.charAt(4);
    
    if (dir == 'F') { // İLERİ: Tüm motorlar ileri
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      setCarLEDs(false); // Hareket başladı -> LED'leri kapat
    } 
    else if (dir == 'B') { // GERİ: Tüm motorlar geri
      digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
      setCarLEDs(false); // Hareket başladı -> LED'leri kapat
    } 
    else if (dir == 'L') { // SOLA DÖN: Sol motorlar geri, sağ motorlar ileri
      digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
      setCarLEDs(false); // Hareket başladı -> LED'leri kapat
    } 
    else if (dir == 'R') { // SAĞA DÖN: Sol motorlar ileri, sağ motorlar geri
      digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
      setCarLEDs(false); // Hareket başladı -> LED'leri kapat
    } 
    else if (dir == 'S') { // DUR: Tüm motorlar stop
      digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
      setCarLEDs(true); // Araba durdu -> LED'leri yak
    }
  } 
  // SERVO KONTROLÜ
  else if (cmd.startsWith("SRV:")) {
    int sIndex = cmd.substring(4, 5).toInt();
    int angle = cmd.substring(6).toInt();
    if (sIndex == 1) servo1.write(angle);
    else if (sIndex == 2) servo2.write(angle);
    else if (sIndex == 3) servo3.write(angle);
    else if (sIndex == 4) servo4.write(angle);
    else if (sIndex == 5) servo5.write(angle);
    else if (sIndex == 6) servo6.write(angle);
  }
}

// --- SENSÖR VERİLERİ ---
void sendSensorData() {
  float t1 = sensor1.getTempCByIndex(0);
  float t2 = sensor2.getTempCByIndex(0);
  float t3 = sensor3.getTempCByIndex(0);

  sensor1.requestTemperatures();
  sensor2.requestTemperatures();
  sensor3.requestTemperatures();

  sensors_event_t a, g, temp;
  if (mpuReady) {
    mpu.getEvent(&a, &g, &temp);
  }

  Serial.print("{\"T1\":"); Serial.print(t1);
  Serial.print(",\"T2\":"); Serial.print(t2);
  Serial.print(",\"T3\":"); Serial.print(t3);
  if (mpuReady) {
    Serial.print(",\"AccelX\":"); Serial.print(a.acceleration.x);
    Serial.print(",\"AccelY\":"); Serial.print(a.acceleration.y);
  }
  Serial.println("}");
}

// --- OLED EKRAN ---
void updateDisplay() {
  u8g2.clearBuffer();
  u8g2.setCursor(0, 15);
  u8g2.print("Sistem Aktif");
  u8g2.setCursor(0, 30);
  u8g2.print("Baglanti: USB");
  u8g2.setCursor(0, 45);
  u8g2.print("Motorlar: Hazir");
  u8g2.sendBuffer();
}