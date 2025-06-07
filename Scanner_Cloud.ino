/*****************************************************************
 * ESP32 Beacon Scanner → Firebase Realtime DB   (Room‑aware, WDT‑safe)
 *****************************************************************/
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <map>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEBeacon.h>

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const char* ROOM_NAME   = "Room 1";

const char* WIFI_SSID   = "UUID";
const char* WIFI_PSW    = "PWD";

#define API_KEY         "AIzaSyA1n-8l6g4cNwVCLJHOpcrmQfBSD4CCNKY"
#define DATABASE_URL    "https://wearablebraceletsystem-default-rtdb.europe-west1.firebasedatabase.app/"

const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET = 3*3600, DST_OFFSET = 0;

#define SERVICE_UUID  "7A0247E7-8E88-409B-A959-AB5092DDB03E"
#define CHAR_UUID     "82258BAA-DF72-47E8-99BC-B73D7ECD08A5"

const unsigned long VIBRATE_PERIOD = 10'000;  
const unsigned long LOST_TIMEOUT   = 20'000; 
const int           SCAN_TIME      = 5;      

#ifndef ENDIAN_CHANGE_U16
#define ENDIAN_CHANGE_U16(x) (uint16_t)(((x)>>8)|((x)<<8))
#endif

String nowTS(){
  struct tm t; char b[32];
  if(getLocalTime(&t)) strftime(b,sizeof(b),"%Y-%m-%d %H:%M:%S",&t);
  else sprintf(b,"%lu",millis()/1000);
  return String(b);
}

/* ------------ Firebase objects -------------------------- */
FirebaseData   fb;
FirebaseAuth   auth;     // stays empty → anonymous
FirebaseConfig cfg;
FirebaseJson   json;

/* ------------ thin wrappers (yield after write) --------- */
void setStr(const String& p,const String& v){ Firebase.RTDB.setString(&fb,p,v); if(Firebase.ready()) yield(); }
void setInt(const String& p,int v){ Firebase.RTDB.setInt(&fb,p,v); if(Firebase.ready()) yield(); }
void pushJSON(const String& p){ Firebase.RTDB.pushJSON(&fb,p.c_str(),&json); if(Firebase.ready()) yield(); }

/* ------------ event logger ------------------------------ */
void logEvent(const char* ev,const String& id,int rssi=0){
  if(!Firebase.ready()) return;

  json.clear();
  json.add("timestamp", nowTS());
  json.add("room", ROOM_NAME);
  json.add("event", ev);
  json.add("beacon", id);
  if(rssi) json.add("rssi", rssi);

  pushJSON(String("events/") + millis());

  String b = String("beacons/") + id;
  String r = String("rooms/")   + ROOM_NAME + "/beacons/" + id;

  setStr(b + "/status",   ev);
  setStr(b + "/lastSeen", nowTS());
  setStr(b + "/lastRoom", ROOM_NAME);
  if(rssi) setInt(b + "/rssi", rssi);

  setStr(r + "/status",   ev);
  setStr(r + "/lastSeen", nowTS());
  if(rssi) setInt(r + "/rssi", rssi);

  Serial.printf("[Firebase] %-14s (%s)\n", ev, ROOM_NAME);
}

/* ------------ beacon bookkeeping ------------------------ */
struct Info{
  String mac;
  int    rssi;
  unsigned long lastSeen=0, lastVib=0;
  bool present=false, pending=false;
};
std::map<String,Info> beacons;

/* ------------ BLE vibrate helper ------------------------ */
BLEClient* gCli=nullptr;
bool vibrate(const String& mac){
  if(!gCli) gCli = BLEDevice::createClient();
  if(gCli->isConnected()) gCli->disconnect();
  bool ok=false;
  if(gCli->connect(BLEAddress(mac.c_str()))){
    if(auto svc=gCli->getService(BLEUUID(SERVICE_UUID))){
      if(auto chr=svc->getCharacteristic(BLEUUID(CHAR_UUID))){
        chr->writeValue("VIBRATE",7); ok=true;
      }
    }
    gCli->disconnect();
  }
  return ok;
}

/* ------------ ultra‑light advert callback --------------- */
class CB : public BLEAdvertisedDeviceCallbacks{
  void onResult(BLEAdvertisedDevice d) override{
    if(!d.isAdvertisingService(BLEUUID(SERVICE_UUID))) return;
    String m=d.getManufacturerData();
    if(m.length()!=25||(uint8_t)m[0]!=0x4C) return;

    BLEBeacon b; b.setData(m);
    String id=String(b.getProximityUUID().toString().c_str())+"-"+
              String(ENDIAN_CHANGE_U16(b.getMajor()))+"-"+
              String(ENDIAN_CHANGE_U16(b.getMinor()));

    Info& inf = beacons[id];
    inf.mac      = d.getAddress().toString();
    inf.rssi     = d.getRSSI();
    inf.lastSeen = millis();

    if(!inf.present){
      inf.present = true;
      inf.lastVib = 0;
      inf.pending = true;      
    }
  }
};

/* ------------ setup ------------------------------------- */
void setup(){
  Serial.begin(115200); Serial.println("\nBooting…");

  WiFi.begin(WIFI_SSID, WIFI_PSW);
  while(WiFi.status()!=WL_CONNECTED){Serial.print('.'); delay(300);}
  configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
  Serial.printf("\nWi‑Fi OK  %s  (%s)\n", WiFi.localIP().toString().c_str(), ROOM_NAME);

  cfg.api_key      = API_KEY;
  cfg.database_url = DATABASE_URL;
  cfg.token_status_callback = tokenStatusCallback;
  Firebase.reconnectWiFi(true);
  fb.setBSSLBufferSize(4096,1024);

  /* anonymous sign‑up */
  if(Firebase.signUp(&cfg,&auth,"","")){
    Serial.println("Anonymous sign‑in OK");
  } else {
    Serial.printf("Sign‑in error: %s\n", cfg.signer.signupError.message.c_str());
  }

  Firebase.begin(&cfg,&auth);
  unsigned long t0=millis();
  while(!Firebase.ready() && millis()-t0<10000){Serial.print('.'); delay(250);}
  Serial.println(Firebase.ready()?"\nFirebase ready ✔":"\nFirebase NOT ready");

  BLEDevice::init("");
  auto scan=BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new CB());
  scan->setActiveScan(true);
  scan->setInterval(100); scan->setWindow(80);
}

/* ------------ loop -------------------------------------- */
void loop(){
  BLEDevice::getScan()->start(SCAN_TIME,false);
  unsigned long now = millis();

  for(auto& kv : beacons){
    String id = kv.first; Info& i = kv.second;

    if(i.pending){ Serial.println(nowTS()+"  DETECTED  "+id); logEvent("DETECTED",id,i.rssi); i.pending=false; }

    if(i.present && now-i.lastSeen >= LOST_TIMEOUT){
      i.present=false; Serial.println(nowTS()+"  LOST      "+id);
      logEvent("LOST",id); continue;
    }
    if(!i.present) continue;

    if(now-i.lastVib >= VIBRATE_PERIOD){
      Serial.println(nowTS()+"  VIBRATE → "+id);
      if(vibrate(i.mac)){ i.lastVib = now; logEvent("VIBRATE_SENT",id); }
      else               logEvent("VIBRATE_FAILED",id);
    }
  }

  BLEDevice::getScan()->clearResults();
  delay(250);      // yield to Wi‑Fi/BLE tasks
}
