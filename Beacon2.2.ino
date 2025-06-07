/*************************************************************
 *  Beacon / Motor / Fingerprint – ESP32‑C3 SuperMini
 *  ▸ Vibrates on command "VIBRATE"
 *  ▸ Stops when an enrolled finger is matched
 *  ▸ Notifies with "AUTHENTICATED:<id>" for logging
 *************************************************************/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEBeacon.h>
#include <Adafruit_Fingerprint.h>

/* ----------------‑ Hardware ----------------------------- */
#define VIBRATION_PIN    2
#define FPRINT_RX       10          // sensor TXD → GPIO10
#define FPRINT_TX        9          // sensor RXD → GPIO9

/* ----------------‑ BLE UUIDs ---------------------------- */
#define DEVICE_NAME        "ESP32"
#define SERVICE_UUID       "7A0247E7-8E88-409B-A959-AB5092DDB03E"
#define CHAR_UUID          "82258BAA-DF72-47E8-99BC-B73D7ECD08A5"
#define BEACON_UUID_REV    "A134D0B2-1DA2-1BA7-C94C-E8E00C9F7A2D"

/* ----------------‑ Globals ------------------------------ */
BLECharacteristic* pCharacteristic = nullptr;
HardwareSerial     fpSerial(1);
Adafruit_Fingerprint finger(&fpSerial);

enum class State { IDLE, VIBRATING };
State state = State::IDLE;

void motorOn () { digitalWrite(VIBRATION_PIN, HIGH); }
void motorOff() { digitalWrite(VIBRATION_PIN, LOW);  }

/* ----------------‑ Restart advertising on disconnect ---- */
class RestartAdvCB : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {}
  void onDisconnect(BLEServer* s) override {
    Serial.println("Central disconnected – advertising restarted");
    s->getAdvertising()->start();
  }
};

/* ----------------‑ Command handler ---------------------- */
class CmdCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* ch) override {
    String data = ch->getValue();
    if (data == "VIBRATE" && state == State::IDLE) {
      motorOn();
      state = State::VIBRATING;
      Serial.println("CMD: VIBRATE → motor ON");
    }
  }
};

bool fingerMatch()
{
  if (finger.getImage()     != FINGERPRINT_OK) return false;
  if (finger.image2Tz()     != FINGERPRINT_OK) return false;
  if (finger.fingerSearch() != FINGERPRINT_OK) return false;
  
  // Return true and store the matched fingerprint ID
  Serial.print("Matched fingerprint ID #");
  Serial.println(finger.fingerID);
  return true;
}

void setup()
{
  Serial.begin(115200);
  pinMode(VIBRATION_PIN, OUTPUT);
  motorOff();

  /* Fingerprint */
  fpSerial.begin(57600, SERIAL_8N1, FPRINT_RX, FPRINT_TX);
  finger.begin(57600);
  if (!finger.verifyPassword()) {
    Serial.println("❌ Fingerprint sensor not found");
    while (true) delay(1);
  }
  Serial.print("Templates in sensor: "); Serial.println(finger.templateCount);

  /* BLE */
  BLEDevice::init(DEVICE_NAME);
  BLEServer* srv = BLEDevice::createServer();
  srv->setCallbacks(new RestartAdvCB());           // <‑‑ key line

  BLEService* svc = srv->createService(SERVICE_UUID);
  pCharacteristic = svc->createCharacteristic(
        CHAR_UUID,
        BLECharacteristic::PROPERTY_READ  |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new CmdCallbacks());
  svc->start();

  /* beacon payload */
  BLEBeacon beacon;
  beacon.setManufacturerId(0x4C00);
  beacon.setMajor(5);
  beacon.setMinor(88);
  beacon.setSignalPower(0xC5);
  beacon.setProximityUUID(BLEUUID(BEACON_UUID_REV));

  BLEAdvertising* adv = srv->getAdvertising();
  BLEAdvertisementData advData;
  advData.setFlags(0x1A);
  advData.setManufacturerData(beacon.getData());
  adv->setAdvertisementData(advData);
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();

  Serial.println("Beacon advertising – ready.");
}

void loop()
{
  if (state == State::VIBRATING) {
    if (fingerMatch()) {
      motorOff();
      state = State::IDLE;
      Serial.println("Fingerprint OK – motor OFF");
      
      // Send authentication notification with the finger ID
      String authMsg = "AUTHENTICATED:" + String(finger.fingerID);
      pCharacteristic->setValue(authMsg.c_str());
      pCharacteristic->notify();
    }
  }
  delay(50);
}
