//einbinden der Bibliotheken für das
//ansteuern des MFRC522 Moduls
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <ArduinoJson.h>   
#include "credentials.h"
#include "ownLists.h"
#include "index_htmlWithJS.h" //variable mit dem HTML/JS anteil
/*
 * Leider probleme mit dem selbst gebastelten (nach diversen Tutorials webserver, es kommt zu unklaren file not found pech gehabt meldungen 
 * also den eigenen. 
 * Erster Versuch noch mit eigenem Webserver, jedoch Fehlermeldungen im Betrieb, nach etwas Recherche stelle um
 * auf https://github.com/dvarrel/ESPAsyncWebSrv das ist ein Fork von https://github.com/me-no-dev/ESPAsyncWebServer
 * dahinter steckt dann Hristo Gochkov's ESPAsyncWebServer 
 * Der unterstützt auch Websockets und nachdem ich 2014 oder so damit keinen Erfolg hatte (Browserstress), sollte es inzwischen ja gehen 
 * Tutorial: https://m1cr0lab-esp32.github.io/remote-control-with-websocket/
 * geht sogar gut, stelle großteils darauf um
 * 
 * Außerdem: ArduinoJson -> dort ein Link auf HeapFragmentation, hoffe mal das es kein Thema bei den par Strings ist - die sorgen 
 * für stress, ArduinoJson sorgt jedoch für eine Vermeidung von HeapFragmentation, auch wenn man dynamisch alloziiert. 
 * auf der WebSite findet sich ein calculator für den Speicher
 */

//in credentials.h
//#define mySSID "todo"
//#define myPASSWORD "passwort"


const char * ssid = mySSID;
const char *password = myPASSWORD;


//definieren der Pins  RST & SDA für den ESP32 und MFRC522
#define RST_PIN     22
#define SS_PIN      21

//erzeugen einer Objektinstanz
MFRC522 mfrc522(SS_PIN, RST_PIN);


int cCounter = 0;
const int cCounterMax = 20;
const long timeoutTime = 2000;

//ein wenig was zum OTA Update, nicht getestet, keine Ahnung, ob das mit dem Asynchronous Webserver funktioniert
void handleOTAStart() {
  // Perform any necessary tasks before the update begins, such as saving data or closing file handles
  Serial.println("OTA update starting...");
}
void handleOTAProgress(unsigned int progress, unsigned int total) {
  // Display the progress of the update to the user
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}
void handleOTAEnd() {
  // Perform any necessary tasks after the update is complete, such as restarting the ESP32
  Serial.println("\nOTA update complete!");
  ESP.restart();
}

//---------------------------------------------


//meine Listen
//StringList msgs(50); //Speichere Nachrichten, gebe Sie auf der Webseite aus, doch keine so gute Idee
#define RFID_MAX 8
RfidList rfidsNew(RFID_MAX); // maximal 8 neue, brauche keine Dateinamen
bool rfidsNewChanged = false;
RfidList rfidsOk(RFID_MAX,"/rfidsOk.dat"); 
String newRfidId = "";
String newRfidOwner = "";

//hatte mal irgendein Beispiel gewählt um zu testen
//stelle um analog zu https://werner.rothschopf.net/202001_arduino_webserver_post.htm
//aber der ist mir zu eingeschränkt, limitierte Menge an Post-Data, er verzichtet auf string, außerdem fehler - irdendwo denke ich falsch, wähle den asynchronous Webserver


AsyncWebServer server(80);

//viele Funktionen anonym, bzw. als Lambda Ausdruck, hier der Rest zum Server-Kram
void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found - ich kann das nicht");
}

String processor(const String& var)
{
  /*if(var == "NEW_RFID_LINES")
  {
    String lines = rfidsNew.htmlLines("add");
    return lines;
  }*/
  if (var == "RFID_MAX")
  {
    return String(RFID_MAX);
  }
  return String();
}


//fuer den Websocket
AsyncWebSocket ws("/ws");

void wsMessage(char *message)
{ 
  const char * begin = "{\"action\":\"message\",\"text\":\"";
  const char * end = "\"}";
  char * str = new char[strlen(message)+strlen(begin)+strlen(end)+24];//Puffer +24 statt +1 :-)
  strcpy(str,begin);
  strcat(str,message);
  strcat(str,end);
  ws.textAll(str);
  delete [] str;
}
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
    AwsFrameInfo *info = (AwsFrameInfo*)arg; 
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
    {
        data[len] = 0; //sollte ein json object als String sein        
        char *cData = (char *)data;
        //String s = cData; //legt eine Kopie an
        const int capacity = JSON_OBJECT_SIZE(3)+8*JSON_OBJECT_SIZE(2)+JSON_ARRAY_SIZE(8); //
        //bisher ein Objekt mit 3 membern und 8 Objekte mit 2 membern (die authorisierten rfids, maximal 8), aber das ist ein Array mit 8 Objekten, 
        //lieber auf Nr sicher gehen
        StaticJsonDocument<capacity> doc;
        DeserializationError err = deserializeJson(doc, cData); //damit veraendert das JSON-Objekt den Speicher cData
        if (err) //faengt auch Probleme beim Speicherplatz ab. 
        {
          Serial.print(F("deserializeJson() failed with code "));
          Serial.println(err.f_str());
          ws.textAll("Fehler beim Derialisieren");//erhalte die Nachricht, javascript hat dann natuerlich einen json-error, das ist ok, dann sieht man es :-)
          ws.textAll(err.f_str());
        }
        else
        {
          if(strcmp(doc["action"],"insert") == 0)
          {
            newRfidId = doc["rfid"].as<String>();
            newRfidOwner = doc["owner"].as<String>();
            wsMessage("neues RFID hinzugefuegt, nicht authorisiert"); 
          }
          else if(strcmp(doc["action"],"clearNew") == 0)
          {
            rfidsNew.clear();
            ws.textAll("{\"action\":\"clearNew\"}");
            wsMessage("Neu gelesene geloescht");
          }
          else if (strcmp(doc["action"],"addAuthorized") == 0)
          {
            //rfidArray beinhaltet die Daten 
            //Serial.println("in addAuthorized");
            //Serial.println("data: " + s); //daten sind da, also hakt es an den naechsten zeilen, einfach nur vertippt 
            JsonArray arr = doc["rfidArray"].as<JsonArray>();
            rfidsOk.clear();//leeren 
            for (JsonVariant value : arr) { //moderne variante eines iterators
              JsonObject o = value.as<JsonObject>();
              //serializeJson(o,Serial);
              String rfid =  (const char *) o["rfid"];
              String owner = (const char *) o["owner"];
              rfidsOk.add(rfid,owner);
            }
            rfidsOk.saveToFile();
            if (rfidsOk.getDelimiterPos() > 0)
              wsMessage("Authorisierte RFIDs hinzugefuegt / entfernt");   
            else
              wsMessage("Authorisierte RFIDs entfernt, Liste leer");
          }
          else if (strcmp(doc["action"],"sendRfidsOk") == 0)
          { //man sollte doc nicht wieder verwenden, das kann memory leaks geben 
            StaticJsonDocument<capacity> ans;
            ans["action"] = "newOkList";
            JsonArray data = ans.createNestedArray("data");
            int anzahl = rfidsOk.getDelimiterPos();
            for (int i = 0; i < anzahl; ++i)
            {
              String rfid,owner;
              rfidsOk.getAt(i,rfid,owner);
              JsonObject o = data.createNestedObject();
              o["rfid"]=rfid;
              o["owner"]=owner;
            }
            
            serializeJson(ans,Serial); //das sieht gut aus
            wsMessage("Send authorized rfids from Server");
            int len = measureJson(ans)+16;//statt +1 +16
            char * buf = new char[len];
            serializeJson(ans,buf,len);
            ws.textAll(buf);
            delete [] buf;
          }
        }
        
        /*String s = (char * ) data;
        unsigned int pos = s.indexOf("|");
        newRfidId = s.substring(0,pos);
        newRfidOwner = s.substring(pos+1);
        */
        //Serial.print("Received from client: ");
        //Serial.println(s);
        //Serial.println(" have rfid:" +  newRfidId + " and owner: " + newRfidOwner);
    }
}
void onWSEvent(AsyncWebSocket       *server,  //
             AsyncWebSocketClient *client,  //
             AwsEventType          type,    // the signature of this function is defined
             void                 *arg,     // by the `AwsEventHandler` interface
             uint8_t              *data,    //
             size_t                len)    
{
    // we are going to add here the handling of
    // the different events defined by the protocol
     switch (type) 
     {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
     }
}


//wird nur zur liste der neuen hinzugefuegt
void addNewAndNotifyClients() {
    unsigned int pos = rfidsNew.getDelimiterPos();
    String remove = "\"removeFirst\":false";
    //const int capacity = JSON_OBJECT_SIZE(3); //zu erweitern, wenn ich die liste uebertrage
    //bisher ein Objekt mit 3 membern
    //StaticJsonDocument<capacity> doc;    
    if (newRfidId != "")
    {
      if (pos == RFID_MAX)
        remove = "\"removeFirst\":true";
      rfidsNewChanged = rfidsNew.add(newRfidId,newRfidOwner);
      if (rfidsNewChanged)
      {
        Serial.println("Sende Freundliche Nachricht, es hat sich was getan, neuer Rfid " + newRfidId  + " First: " + remove);
        /*
        doc["action"] = "new";
        doc["rfid"]   = newRfidId.c_str();
        doc["owner"]   = newRfidOwner.c_str();
        char buffer[128];
        int len = serializeJson(doc,buffer); 
        ws.textAll(buffer,len); //naja, so viel besser ist das nicht 
        */ 
        ws.textAll("{\"rfid\":\""+newRfidId+"\",\"owner\":\""+newRfidOwner+"\","+remove+",\"action\":\"new\"}");
        //baue mein json objekt selbst, na gut, doch ein wenig laestig
        //die uebertragung der anderen daten wird per post gemacht und dann liefere ich die ganze Seite selbst - ansonsten wuerde sich ArduinoJson wohl doch lohnen
        //nein, ich denke, das mache ich auch über die WebSockets, mal sehen, wie ich sende, also doch websockets
        wsMessage("Neues Element in die Liste der gelesenen aufgenommen");
      }
      newRfidId = "";
      newRfidOwner = "";
    }
}


void mountFileSystem()
{ // versuchen, ein vorhandenes Dateisystem einzubinden
  if (!LittleFS.begin(false)) 
  {
    Serial.println("LittleFS Mount fehlgeschlagen");
    Serial.println("Kein Dateisystemsystem gefunden; wird formatiert");
    // falls es nicht klappt, erneut mit Neu-Formatierung versuchen
    if (!LittleFS.begin(true)) 
    {
      Serial.println("LittleFS Mount fehlgeschlagen");
      Serial.println("Formatierung nicht möglich");
      return;
    } else {
      Serial.println("Formatierung des Dateisystems erfolgt");
    }
  }
  Serial.println("Informationen zum Dateisystem:");
  Serial.printf("- Bytes total:   %ld\n", LittleFS.totalBytes());
  Serial.printf("- Bytes genutzt: %ld\n\n", LittleFS.usedBytes());
}

void setup() {
  //beginn der seriellen Kommunikation mit 115200 Baud
  Serial.begin(115200);
  mountFileSystem();
  rfidsOk.loadFromFile(); 

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    if (cCounter++ > cCounterMax)
      break;
    Serial.print(".");
  }

  if (cCounter <= cCounterMax) //wifi connected
  {
    Serial.print("Wifi Connected, adress ");
    Serial.println(WiFi.localIP());
    String msg = String("Wifi connected ,adress ") + WiFi.localIP().toString();
    ArduinoOTA.setHostname("Rfid1");//muss vor begin stehen 
    ArduinoOTA.begin();
    ArduinoOTA.onStart(handleOTAStart);
    ArduinoOTA.onProgress(handleOTAProgress);
    ArduinoOTA.onEnd(handleOTAEnd);

    server.onNotFound(notFound);
    //lustig, ich bin alt,  [] leitet einen Lambda Ausdruck ein, also eine anonyme Funktion
    //die gabs frueher nicht :-), dafür mehr Lametta
    server.on("/favicon.ico", [](AsyncWebServerRequest *request)
    {
      request->send(204);//no content
    });

    //normale Anfrage
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      request->send_P(200, "text/html", index_html, processor);      
    });
    //Formular gesendet, klappt prinzipiell
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
    {
      String par = "testEintrag"; //fuer den testeintrag nicht sinnvoll, den werde ich auch per websocket hinzufuegen muessen statt per post 
      if(request->hasParam(par, true)) //ist per websocket erledigt
      {
        newRfidId = request->getParam(par, true)->value();
        Serial.println("Found Param "+ par+" with: " + newRfidId );
      }
      Serial.println("All parameters");
      //List all parameters
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isFile()){ //p->isPost() is also true
          Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else if(p->isPost()){
          Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } else {
          Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send_P(200, "text/html", index_html, processor);      
    });
    
    ws.onEvent(onWSEvent);
    server.addHandler(&ws); //WebSocket dazu 
    server.begin();
  }
  
  //eine kleine Pause von 50ms.
  
  delay(50);
  //begin der SPI Kommuonnikation
  SPI.begin();
  //initialisieren der Kommunikation mit dem RFID Modul
  mfrc522.PCD_Init();
}

void loop() {
  static String lastRfid = "";
  if (cCounter <=cCounterMax)
    ArduinoOTA.handle(); //warnhinweis: mehr als 5s delay in der loop und der upload wird nicht funktionien

  ws.cleanupClients(); // aeltesten client heraus werfen, wenn maximum Zahl von clients ueberschritten, 
                       // manchmal verabschieden sich clients wohl unsauber / gar nicht -> werden wir brutal  
  
  //folgender Block sollte später nach unten, momentan können rfids über einen testeintrag hinzu gefügt werden, später nur durch RFID-Tags wedeln 
  
  
  if(mfrc522.PICC_IsNewCardPresent() &&mfrc522.PICC_ReadCardSerial())
  {
    newRfidId = mfrc522.uid.uidByte[0] < 0x10 ? "0" : "";
    newRfidId.concat(String(mfrc522.uid.uidByte[0], HEX));
    for (byte i = 1; i < mfrc522.uid.size; i++) 
    {
      newRfidId.concat(mfrc522.uid.uidByte[i] < 0x10 ? "-0" : "-");
      newRfidId.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
   
  
    if (!newRfidId.equals(lastRfid)) 
    {
      //überschreiben der alten ID mit der neuen
      lastRfid = newRfidId;
      Serial.println("gelesene RFID-ID: " + newRfidId);
    }
  }
  
  addNewAndNotifyClients(); //wenn neu, dann hinzufügen
    
}
