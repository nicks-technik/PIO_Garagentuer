/* TH: 08.0.2019
   This Version is a Garage Door Opener, Door Opener and a 3x4 Code pad.
   The reed contact is implemented on A0-Analog Input. The reed contact must be a normally open contact, to save power when no amperes flowing, when the door is closed. 
   FIXME: I had problems on the ESP8266 with the 2.5.0 Version and the Secure MQTT
   const_cast<char *> to convert String to char
*/
#define LogOn // Do you want debug messages?
// #define WithReed // are the reed contacts on the garage doors?
// #define LogOnDetail  // More messages on the main loop

#include "Arduino.h"

// Contains all the secret credentials
#include "secure.h"

#include <myGENERAL.h> // Contains some general routines, e.g. PrintVar
#include <myWIFI.h>    // Contains all the WIFI routines
#include <myOTA.h>     // Contains the Over-The-Air routines
#include <myMQTT.h>    // Contains the MQTT(S) routines
#include <myKEYPAD.h>    // Contains some Coding/Keypad routines

const unsigned int _CONST_ReedRight_PIN = A0;  // PIN right NO-Reed, normally open
const unsigned int _CONST_Opener_PIN = D0;     // D8 is GPIO15 PIN for the door opening of normal door (Relay)
const unsigned int _CONST_GDoorLeft_PIN = D2;  // is GPIO PIN for the opening of the left door (Relay)
const unsigned int _CONST_GDoorRight_PIN = D1; // is GPIO PIN for the opening of the right door (Relay)

const unsigned int _CONST_DoorHighDuration = 1000;  // The relay will we on for this time
const unsigned int _CONST_ButtonDebounceTimer = 10; // msec wait to debounce the push button
const unsigned int _CONST_Timeout = 3000;           // wait 3 secs until the code is reseted
const unsigned int _CONST_OpeningDuration = 30000;  // Duration the DoorOpener relay is switched on

// Defined in secured.h
// #define _CONST_GarageDoor_MQTT_Confirm       "_GarageDoor_MQTT_Confirm"
// #define _CONST_GarageDoor_MQTT              "_GarageDoor_MQTT"
// #define _CONST_GarageDoorRight_MQTT          "_GarageDoorRight_MQTT"
// #define _CONST_GarageDoorRight_MQTT_Confirm  "_GarageDoorRight_MQTT_Confirm"
// #define _CONST_GarageDoorLeft_MQTT           "_GarageDoorLeft_MQTT "
// #define _CONST_GarageDoorLeft_MQTT_Confirm   "_GarageDoorLeft_MQTT_Confirm"

unsigned long _VAR_LastKeyPress;       // Millis, when the last button was pressed
unsigned long _VAR_OpenerOnMillis = 0; // When was the DoorOpener pressed, in Millis
unsigned long _VAR_Millis;             // actual Millis at the moment, for comparision und timer

//======================================================================================
//======================================================================================
//======================================================================================
boolean FUNC_CodeComparision() // Compares the input code with the code key
{
#ifdef LogOnDetail
  String _VARL_Output = "Code: " + _VAR_Code;
  Serial.println(_VARL_Output);
#endif

  if (_VAR_Code == _CONST_Code) // Compares the input with the specification
  {
    return true; // the 2 values are equal
  }
  return false;
}

//======================================================================================
void FUNC_OpenGarageDoor() // Open garage door (small)
{
  digitalWrite(_CONST_Opener_PIN, HIGH);
  // delay(1000);
  _VAR_OpenerOnMillis = _VAR_Millis; // The opener relay was swithed on here
  _VAR_LastKeyPress = 0;             // The last key pressed reset to default
  _VAR_Code = "";                    // reset the code String after the opening the code
#ifdef LogOn
  FUNC_PrintVar("Opener ON"); // Show logging informations Opened garage door
#endif
  FUNC_MQTT_PublishMessage(const_cast<char *>(_CONST_GarageDoor_MQTT_Confirm)); // Sends an MQTTS Message to confirm the action

} // End of FUNC_OpenGarageDoor

//======================================================================================
// R or L Door Open, PIN of the relays, PublishText, duration of the activated relay
void FUNC_OpenDoorX(unsigned int _VARL_Relay_PIN, char *_VARL_ConfirmText, unsigned int _VARL_Timeout)
{
  String _VARL_ConfirmTextStr = _VARL_ConfirmText;
#ifdef WithReed
  // This code is only activated, when a reed switch is connected.
  // if ((analogRead(_CONST_ReedRight_PIN)) < 500)
#endif
  {

#ifdef WithReed
#ifdef LogOn
    // Serial.println(analogRead(_CONST_RelayedRight_PIN));
#endif
#endif
    digitalWrite(_VARL_Relay_PIN, HIGH);
    delay(_VARL_Timeout);
    digitalWrite(_VARL_Relay_PIN, LOW);

#ifdef WithReed
    // This code is only activated, when a reed switch is connected.
  }
  else
  {
    _VARL_ConfirmTextStr = _VARL_ConfirmTextStr + ", but already open";
#ifdef LogOn
    Serial.println(analogRead(_CONST_ReedRight_PIN));
    FUNC_PrintVar(const_cast<char *>(_VARL_ConfirmTextStr.c_str()));
#endif

#endif
  }
  FUNC_MQTT_PublishMessage(const_cast<char *>(_VARL_ConfirmTextStr.c_str()));
}

//======================================================================================
void FUNC_MQTT_SubFeed_Callback(char *_VARL_data, uint16_t _VARL_len) // This Function is called, when new MQTT Data received
{
#ifdef LogOn
  Serial.print("Hey we're in a feed_test callback, the value is: ");
  Serial.println(_VARL_data);
#endif

  if (String(_VARL_data) == _CONST_GarageDoor_MQTT)
  {
    FUNC_OpenGarageDoor(); // Open the garage door
  }

  if (String(_VARL_data) == _CONST_GarageDoorRight_MQTT)
  {
#ifdef LogOn
    Serial.print(__FUNCTION__);
    Serial.print("() Line: ");
    Serial.println(__LINE__);
#endif
    FUNC_OpenDoorX(_CONST_GDoorRight_PIN, const_cast<char *>(_CONST_GarageDoorRight_MQTT_Confirm), _CONST_DoorHighDuration);
  }

  //Check, if the Left Garage Door should be opened
  if (String(_VARL_data) == _CONST_GarageDoorLeft_MQTT)
  {
#ifdef LogOn
    Serial.print(__FUNCTION__);
    Serial.print("() Line: ");
    Serial.println(__LINE__);
#endif
    FUNC_OpenDoorX(_CONST_GDoorLeft_PIN, const_cast<char *>(_CONST_GarageDoorLeft_MQTT_Confirm), _CONST_DoorHighDuration);
  }
}

//======================================================================================
void FUNC_PushButton()
{
  // Read the keypad Pin
  char _VARL_key = _keypad.getKey();

  if (_VARL_key)
  {                                                         // when a button is pressed
    if ((_VAR_Millis - _CONST_Timeout) < _VAR_LastKeyPress) // check, if the code input was interupted
    {                                                       // if no interrupt
      _VAR_Code = _VAR_Code + _VARL_key;
      //      Serial.println(_VAR_Code);
      //      Serial.println();
    }
    else
    { // interrupt of the code input
      // _VAR_Code = "";
      _VAR_Code = _VARL_key;
    }
    _VAR_LastKeyPress = _VAR_Millis; // when are the last key pressed

#ifdef LogOn
    Serial.println("Key:Code:" + String(_VARL_key) + ":" + String(_VAR_Code));
#endif
  }

  // Are the code accepted and is the Opener relay off
  if (FUNC_CodeComparision() == true && _VAR_OpenerOnMillis == 0)
  { // Swicht the relay on
    FUNC_OpenGarageDoor();
  }
}

//======================================================================================
void FUNC_MQTT_KeepalivePing()
{ // Regular MQTT Pings of the Adafruit IO Server
  if (_VAR_Millis > (_CONST_MQTTIntervall + _VAR_MQTTAn))
  {
    if (!_VAR_MQTT_Secure_Client.ping())
    {
      _VAR_MQTT_Secure_Client.disconnect();
    }
    _VAR_MQTTAn = _VAR_Millis;
#ifdef LogOn
    FUNC_PrintVar("KeepalivePing");
#endif
  }
}

//======================================================================================
//======================================================================================
void setup()
{

#ifdef LogOn
  Serial.begin(115200);
  // delay(10);
  Serial.println("Booting");
#endif

// Setup the WIFI
  FUNC_WIFI_Setup();

// Setup the OTA Setup
  FUNC_OTA_Setup();

  // Setup for the Secure MQTT subscription
  _VAR_MQTT_SubFeed.setCallback(FUNC_MQTT_SubFeed_Callback);

  // Setup MQTT subscription for time feed.
  _VAR_MQTT_Secure_Client.subscribe(&_VAR_MQTT_SubFeed);
  // check the fingerprint of io.adafruit.com's SSL cert
  _VAR_WIFIClientSecure.setFingerprint(_CONST_Fingerprint);

  FUNC_MQTT_Connect();
  // _VAR_MQTT_Secure_Client.ping();

  pinMode(_CONST_Opener_PIN, OUTPUT);     // Set the PIN as Output for the Door Opener
  pinMode(_CONST_GDoorRight_PIN, OUTPUT); // Set the PIN as Output for the right Door Opener
  pinMode(_CONST_GDoorLeft_PIN, OUTPUT);  // Set the PIN as Output for the left Door Opener

#ifdef LogOn
  FUNC_PrintVar("End of Setup");
#endif
}
//======================================================================================
//======================================================================================
void loop()
{ // Main
  // Managing the OTA Request
  ArduinoOTA.handle();
#ifdef LogOnDetail
  Serial.println("After OTA.handle");
  PrintVar("in loop");
#endif

  _VAR_Millis = millis(); // Save the Millis of the actual Loop

  FUNC_MQTT_Paket();         // Checks, if the MQTT connection is up or start.
  FUNC_PushButton();         // Compares the code input with the keypad
  FUNC_MQTT_KeepalivePing(); // regular MQTT Ping of the Adafruit Server
  if ((_VAR_Millis > (_CONST_OpeningDuration + _VAR_OpenerOnMillis)) && (_VAR_OpenerOnMillis != 0))
  {
    digitalWrite(_CONST_Opener_PIN, LOW);
    _VAR_OpenerOnMillis = 0;

#ifdef LogOn
    FUNC_PrintVar("Opener off");
#endif
  }

  if (_VAR_Millis > 4000000000)
  {
    FUNC_SoftwareReset();
  }
}
