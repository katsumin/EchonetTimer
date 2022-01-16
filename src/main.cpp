#define _M5DISPLAY_H_
class M5Display
{
};
#include "View.h"
#include <time.h>
#include <WiFi.h>
#include <NTPClient.h>
#include "config.h"
#include "FunctionButton.h"
#include "HeadView.h"
#include "DataStore.h"
#include "EthernetManager.h"
#include "SettingServer.h"
#include "Preferences.h"

// #define TEST

// instances
static TFT_eSPI lcd;
ViewController viewController(&lcd);
HeadView headView(&lcd);
DataStore dataStore(&viewController);
SettingServer settingServer;

const long gmtOffset_sec = 9 * 3600; // 9時間の時差を入れる

EthernetManager *em;
NTPClient *ntp;
void nw_init()
{
  IPAddress addr;
  UDP *udpNtp = nullptr;
  UDP *udpUni = nullptr;
  UDP *udpMulti = nullptr;

  headView.init();
  WiFi.disconnect();
  Serial.print("Connecting to WIFI");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  addr = WiFi.localIP();
  headView.setNwType("WiFi");
  udpNtp = new WiFiUDP();
  udpUni = nullptr;
  udpMulti = new WiFiUDP();
  headView.setIpAddress(addr);
  em = new EthernetManager(udpMulti, udpUni);
  em->setDataStore(&dataStore);

  // NTP
  ntp = new NTPClient(*udpNtp, NTP_SERVER, gmtOffset_sec, 10 * 60 * 1000);
  ntp->begin();
  ntp->update();
  headView.setNtp(ntp);
  viewController.setNtp(ntp);

  // Http
  settingServer.begin(&dataStore);
}

#define NTP_INTERVAL (60 * 60 * 24)
#define INTERVAL (60)
#define SCAN (30)
FunctionButton *btnB;
FunctionButton *btnC;
void setup()
{
  // LCD
  lcd.init();
  lcd.setRotation(1);

  // Serial
  Serial.begin(115200);
  Serial.flush();

  // 何故か、M5.BtnAとM5.BtnBが途中で置き換わってしまい、M5.update()と違うインスタンスを指してしまう
  // ので、新しいインスタンスを設定しなおす
  btnB = new FunctionButton(BUTTON_B_PIN, &lcd, POS_B_X);
  btnC = new FunctionButton(BUTTON_A_PIN, &lcd, POS_C_X);

  // LAN
  btnB->disable("NTP");
  nw_init();
  while (true)
  {
    boolean success = ntp->update();
    if (success)
      break;
    Serial.println("NTP retry..");
    delay(100);
  }
  btnB->enable("NTP");
  btnC->enable("DISP");

  // Echonet SCAN
  em->scan();
}

unsigned long preEpoch = 0;
boolean db = false;
void loop()
{
  unsigned long epoch = ntp->getEpochTime();

  // Echonet Refresh
  em->update();

  // Web Server Refresh
  settingServer.exec();

  // 表示更新
  if (preEpoch != epoch)
  {
    preEpoch = epoch;
    headView.update();
    viewController.update();
    if (epoch % INTERVAL == SCAN)
    {
      em->request();
    }
    if (epoch % NTP_INTERVAL == 0)
    {
      Serial.println("NTP update at day");
      ntp->update();
    }
  }

  if (btnB->isEnable() && btnB->getButton()->wasPressed())
  {
    Serial.println("NTP update");
    btnB->disable("NTP");
    ntp->update();
    btnB->enable("NTP");
  }
  if (btnC->isEnable() && btnC->getButton()->wasPressed())
  {
    Serial.println("DISP change");
    viewController.changeNext();
  }
  if (btnC->getButton()->wasReleasefor(3000))
  {
    Serial.println("nvs clear");
    Preferences _preferences;
    _preferences.begin("EchonetTimer");
    _preferences.clear();
    _preferences.end();
  }

  // ボタン状態更新
  btnB->read();
  btnC->read();

  delay(10);
}