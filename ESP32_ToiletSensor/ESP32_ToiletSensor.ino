#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// 設定WIFI
const char* ssid     = "rock";
const char* password = "bxBSDKtGgpx6pHfmGEaV3ZYvyHWsaTz7";

// 設定網路時間
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "tw.pool.ntp.org");

// 設定螢幕
#define SCREEN_WIDTH 128 // OLED 寬度像素
#define SCREEN_HEIGHT 64 // OLED 高度像素
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 設定溫溼度感測器及繼電器
#define relay 16          // GPIO16
#define relay1 17          // GPIO17
#define red 18
#define DHTPIN 19         // GPIO19   
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float difference = 0;   // 濕度誤差值預設是0
bool times = true;
int delayRelay1 = 0;      // 繼電器延遲計次器
int delayRelay2 = 0;      // 繼電器延遲計次器

void setup() {
  Serial.begin(115200);
  
  dht.begin();
  Wire.begin();
  pinMode(relay, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(red, INPUT);
  digitalWrite(relay, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 一般1306 OLED的位址都是0x3C
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  // 顯示Adafruit的LOGO，算是開機畫面
  display.display();
  delay(1000); // 停1秒

  // 嘗試連接WIFI
  int t = 0;
  WiFi.begin(ssid, password);
  Serial.print("WiFi Connecting");
  while (WiFi.status() != WL_CONNECTED && t <= 20) {
    delay(500);
    display.clearDisplay();
    display.setTextSize(1);             // 設定文字大小
    display.setTextColor(1);        // 1:OLED預設的顏色(這個會依該OLED的顏色來決定)
    display.setCursor(0, 0);            // 設定起始座標
    display.print( "WiFi " );
    display.println( times ? "Connecting" : "" );
    display.display();
    Serial.print(".");
    t++;
    times = !times;
  }
  Serial.println("");

  timeClient.begin();
  timeClient.setTimeOffset(28800); //設定時區(+8)*3600

  display.clearDisplay();
  display.setTextSize(1);             // 設定文字大小
  display.setTextColor(1);        // 1:OLED預設的顏色(這個會依該OLED的顏色來決定)
  display.setCursor(0, 0);            // 設定起始座標
  display.print( "WiFi " );
  if (WiFi.status() == WL_CONNECTED) showMsg("Connected");
  else showMsg("Not Connected");

  display.display();                  // 要有這行才會把文字顯示出來
  Serial.println("");
  digitalWrite(relay, LOW);
  delay(3000); // 停1秒
}

void loop() {
  // 基本參數
  if (times) timeClient.update();
  byte  error, I2C_address;
  int   nDevices = 0;
  String strHour = "--";
  String strMinute = "--";
  int rssi = 0;

  // 清除畫面
  display.clearDisplay();
  display.setTextSize(2);             // 設定文字大小
  display.setTextColor(1);        // 1:OLED預設的顏色(這個會依該OLED的顏色來決定)
  display.setCursor(0, 0);            // 設定起始座標

  // 取得時間
  if (WiFi.status() == WL_CONNECTED) {
    strHour = timeZero(timeClient.getHours());
    strMinute = timeZero(timeClient.getMinutes());
  }
  String strTimes = times ? ":" : " ";
  showMsg(String("") + strHour + strTimes + strMinute + getRSSI());

  display.println( "" );
  display.setTextSize(1);             // 設定文字大小

  // 取得溫濕度
  float flTemp = dht.readTemperature();
  float flHumidity = dht.readHumidity() + difference;
  flHumidity = flHumidity < 0 ? 0 : flHumidity;
  showMsg(String("") + "Temp:" + getFloat(flTemp) + " | " + "RH  :" + getFloat(flHumidity));

 String relayStatus = "";
  if (digitalRead(red) == HIGH) {
    relayStatus += "LED :  ON | ";//偵測到有人經過
    digitalWrite(relay1, HIGH);
  }
  else {
    relayStatus += "LED : OFF | ";//偵測無人經過
    digitalWrite(relay1, HIGH);
  }

  // 設定繼電器
  if (flHumidity > 70) {
    if (delayRelay1 >= 5  || relay == HIGH) {
      digitalWrite(relay, LOW);
      relayStatus += "FAN :  ON";
      delayRelay1 = 0;
    }
    else {
      relayStatus += ( times ? "FAN :    " : "FAN : OFF");
      delayRelay1++;
    }
  }
  else {
    if (delayRelay1 <= -8  || relay == LOW) {
      digitalWrite(relay, LOW);
      relayStatus += "FAN : OFF";
      delayRelay1 = 0;
    }
    else {
      relayStatus += ( times ? "FAN :    " : "FAN :  NO");
      delayRelay1--;
    }
  }
  showMsg(relayStatus+String("")+delayRelay1);

  //輸出結果
  display.display();   // 要有這行才會把文字顯示出來
  Serial.println("");

  times = !times;
  delay(500);
}

// 同時輸出到序列阜跟螢幕
void showMsg(String text) {
  display.println(text);
  Serial.print(text + " | ");
}

// 時間數字補零字串
String timeZero(int s) {
  if (s < 10)
    return "0" + String("") + s;
  else
    return String("") + s;
}

// 格式化浮點數字串
String getFloat(float data) {
  String x = String("") + data;
  if (data < 10) x = " " + x;
  String y = "";
  for (int i = 0; i < x.length() - 1; i++) {
    y += x[i];
  }
  return y;
}

// 格式化WIFI訊號字串
String getRSSI() {
  int rssi = 0;
  if (WiFi.status() == WL_CONNECTED) rssi = WiFi.RSSI();
  if (rssi >= 0) return String("") + "    " + rssi;
  else if (rssi <= -100) return String("") + " " + rssi;
  else return String("") + "  " + rssi;
}
