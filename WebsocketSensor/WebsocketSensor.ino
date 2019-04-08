#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketServer.h>
#include <ArduinoJson.h>
#include <WiFiAccess.h>

#include <OPTIGATrustX.h>
#include <Dps310.h>


#define UID_LENGTH 27
uint8_t  uid[UID_LENGTH];
uint16_t uidLength = UID_LENGTH;
char uid_str[UID_LENGTH*2 + 1];

void OptigaTrustX_UniqueIDToStr(uint8_t *uid, uint16_t uidLength)
{
  for(int i = 0; i < uidLength; i++)
  {
      sprintf(&uid_str[2*i],"%02X", uid[i]);
  }
  uid_str[UID_LENGTH*2] = '\0';
  Serial.printf("%s",uid_str);
}

void OptigaTrustX_Init()
{
  uint32_t ret = 0;
  /*
   * Initialize an OPTIGA™ Trust X Board
   */
  Serial.println("Begin to trust ... ");
  ret = trustX.begin();
  if (ret) {
    Serial.println("Trust X Failed");
    while (true);
  }
  Serial.println("Trust X OK");
}

Dps310 Dps310PressureSensor = Dps310();

void DPS310_Init()
{
  Dps310PressureSensor.begin(Wire);
  int16_t ret = Dps310PressureSensor.startMeasureBothCont(0, 2, 0, 2);

    if (ret != 0)
  {
    Serial.print("DPS310 Init FAILED! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.println(" DPS310 Init complete!");
  }
}

void DPS310_Read(int32_t *press, int32_t *temp)
{
  uint8_t temp_size  = 1;
  uint8_t press_size = 1;

  int16_t ret = Dps310PressureSensor.getContResults(temp, temp_size, press, press_size);

  if (ret != 0)
  {
    Serial.println();
    Serial.println();
    Serial.print(" DSP310 FAIL! ret = ");
    Serial.println(ret);
  }
}

WiFiServer server(80);
WebSocketServer webSocketServer;

const char* ssid = WIFI_SSID;
const char* password =  WIFI_PASS;

#define JSON_API_CMD_GET_DATA_ALL   0x01
#define JSON_API_CMD_GET_DATA       0x02

void handleReceivedMessage(String request, String &reply){
 
  StaticJsonDocument<500> doc;                             //Memory pool
  StaticJsonDocument<500> docOutput;
  DeserializationError err = deserializeJson(doc,request); //Parse message
 
  if (err) 
  {   //Check for errors in parsing
    Serial.println("Parsing failed");
    return;
  }
  int32_t press = 0;
  int32_t temp  = 0;
  DPS310_Read(&press, &temp);

  uint8_t          cmd  = doc["command"];
  JsonObject       root = docOutput.to<JsonObject>();
  docOutput["uniqueID"] = uid_str;

  switch(cmd)
  {
    case JSON_API_CMD_GET_DATA_ALL:
    {

      JsonObject sensor_data = root.createNestedObject("sensors");
      sensor_data["temperature"] = temp;
      sensor_data["pressure"] = press;
      serializeJson(docOutput, reply);
      Serial.println("JSON API Get all data");
    }
    break;

    case JSON_API_CMD_GET_DATA:
     {
      JsonArray array = doc["sensors"];
      uint16_t array_size = array.size();
      JsonObject sensor_data = root.createNestedObject("sensors");
      for(int i = 0; i < array_size; i++)
      {
        if(strcmp(doc["sensors"][i], "temperature") == 0)
        {
          sensor_data["temperature"] = temp;
        }
        else if(strcmp(doc["sensors"][i], "pressure") == 0)
        {
          sensor_data["pressure"] = press;
        }
      }
      serializeJson(docOutput, reply);
      Serial.println("JSON API Get data");
     }
    break;

    default:

      Serial.println("JSON API invalid command");
    break;

    serializeJson(docOutput, reply);
  }
}

void setup() {

  Serial.begin(115200);

  OptigaTrustX_Init();

  trustX.getUniqueID(uid, uidLength);
  OptigaTrustX_UniqueIDToStr(uid,uidLength);

  DPS310_Init();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());

  server.begin();
  delay(100);
}

void loop() {

  WiFiClient client = server.available();
  
  if (client.connected() && webSocketServer.handshake(client)) 
  {

    String request;
    // String reply;

    while (client.connected()) {
      String reply;
      request = webSocketServer.getData();

      if (request.length() > 0) {
         handleReceivedMessage(request, reply);
         webSocketServer.sendData(reply);
      }

      delay(10); // Delay needed for receiving the data correctly
    }
    
   Serial.println("The client disconnected");
   delay(100);
  }

  delay(100);
}
