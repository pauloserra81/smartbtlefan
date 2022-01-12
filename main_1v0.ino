 /**
 * SuperFan control
 * Controls a 3 speed fan via bluetooth HRM
 * author Paulo Serra
 */
#include <Arduino.h>
#include <Ticker.h>
#include "BLEDevice.h"
//#include "BLEScan.h"
#include <ezButton.h>
#include "FastLED.h"

// Set number of relays
#define NUM_RELAYS 3

// Heart Rate Zones
#define ZONE_1 117 // minimum to turn on fan
#define ZONE_2 146 // 140 is hard work, go to speed 2
#define ZONE_3 169 // almost dying full wind

//LED control
#define LED_PIN 27
#define NUM_LEDS 1

// Define the array of leds
CRGB leds[NUM_LEDS];

Ticker blinker;
const float blinkerPace = 0.5;
static bool isBlinking = false;

void blink() {

static bool isBlue = false;

 if (isBlue) {
      leds[0] = CRGB::Black;
      isBlue = false;
      FastLED.show();
  }
  else {
      leds[0] = CRGB::Blue;
      FastLED.show();
      isBlue = true;

  }
}

//Button setup
//using pins 25 and 21 for the push button, 25 will be used as ground to make connections easier
#define BUTTON_PIN 21
#define BUTTON_GDPIN 25

ezButton button(BUTTON_PIN);  // create ezButton object that attach to pin 15;


// Assign each GPIO to a relay
uint8_t relayGPIOs[NUM_RELAYS] = {19, 23, 33};

// The remote service we wish to connect to.
static BLEUUID serviceUUID("0000180d-0000-1000-8000-00805f9b34fb");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID(BLEUUID((uint16_t)0x2A37));
//0x2A37

static boolean doConnect = false;
static boolean connected = false;
static boolean notification = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;


uint8_t op_mode = 0;  //state variable for operating modes ->0 = BT, 1-> fixed speed 1, 2-> fixed speed 2, 3-> fixed speed 3.
uint8_t hr_zone = 0;  //hr zone as read on BT

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Heart Rate: ");
    Serial.print(pData[1], DEC);
    Serial.println("bpm");
    if(pData[1] == 0) {
      hr_zone = 0;
    }
    else if(pData[1] <= ZONE_1 && pData[1] > 0) {
      hr_zone = 0;
    }
    else if(pData[1] > ZONE_1 && pData[1] <= ZONE_2) {
      hr_zone = 1;
    }
    else if(pData[1] > ZONE_2 && pData[1] <= ZONE_3) {
      hr_zone = 2;
    }
    else if(pData[1] > ZONE_3) {
      hr_zone = 3;
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  button.setDebounceTime(100); // set debounce time to 100 milliseconds
  pinMode(BUTTON_GDPIN, OUTPUT);
  digitalWrite(BUTTON_GDPIN, LOW);
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);  // setup led - GRB ordering is assumed
  FastLED.setBrightness(20);
  leds[0] = CRGB::Blue;
  BLEDevice::init("");
  // Set all relays to off when the program starts
  for(int i=1; i<=NUM_RELAYS; i++){
    pinMode(relayGPIOs[i-1], OUTPUT);
      digitalWrite(relayGPIOs[i-1], LOW);
  }

  //LED starts blinkning until connected
  //pinMode(LED_PIN, OUTPUT);
  blinker.attach(blinkerPace, blink);
  isBlinking = true;
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {
  CRGB prev_color ;
  button.loop(); // Debounce button - MUST call the loop() function first
  //update mode
  if (button.isPressed()) {
     Serial.println("Button pressed !!");
     
     if (op_mode == NUM_RELAYS) 
     op_mode = 0;
     else
     op_mode = op_mode +1 ;

     Serial.print("Setting new mode to");
     Serial.println(op_mode,DEC);

     }

  if (op_mode == 0) { //we are in bluetooth HR mode

     //assign relays based on HR zones
      
      for(int i=1; i<=NUM_RELAYS; i++){
      digitalWrite(relayGPIOs[i-1], LOW);
      }
      if (connected && hr_zone >0) {
      digitalWrite(relayGPIOs[hr_zone-1], HIGH);
      }

  //LED indicates connected status and HR zone
  if (connected) {
     if (isBlinking) {
     blinker.detach(); 
     isBlinking =false;
     }
     
     prev_color = leds[0];
     if (hr_zone==0)
       leds[0] = CRGB::Cyan;
     else if (hr_zone==1)
       leds[0] = CRGB::Green;
     else if (hr_zone ==2 )
       leds[0] = CRGB::Orange;
     else if (hr_zone ==3 )
       leds[0] = CRGB::Red;

     if (leds[0] != prev_color)
        FastLED.show();
      }
  else { 
    if (isBlinking==false){
    blinker.attach(blinkerPace, blink);
    isBlinking = true;
    }
 //   BLEDevice::getScan()->start(3); //we are not connected, scan another 3s
    }

  
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    if (notification == false) {
        Serial.println("Turning Notification On");
        const uint8_t onPacket[] = {0x01, 0x0};
        pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)onPacket, 2, true);
        notification = true;
    }
  }else /*if(doScan)*/{
    BLEDevice::getScan()->start(3);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

//END OF BT CONTROL, NOW GETTING MANUAL MODE
      
  }else  { //manual control mode
     Serial.print("Manual speed :");
     Serial.println(op_mode, DEC);
     if (isBlinking){
     blinker.detach();  //turn off LED blink as we are in manual mode
     isBlinking = false;
     }
     prev_color = leds[0];
//     digitalWrite(LED_PIN, LOW);
      if (op_mode==1)
        leds[0] = CRGB::Green;
      else if (op_mode ==2 )
        leds[0] = CRGB::Yellow;
      else if (op_mode ==3 )
        leds[0] = CRGB::Red;

      if (leds[0] != prev_color)  
           FastLED.show();
      
     
      for(int i=1; i<=NUM_RELAYS; i++){
        digitalWrite(relayGPIOs[i-1], LOW);
      }
      digitalWrite(relayGPIOs[op_mode-1], HIGH);
     } 
     
  //delay(1000); // Delay a second between loops.
} // End of loop
