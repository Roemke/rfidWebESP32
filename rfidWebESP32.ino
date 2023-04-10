#include <WiFi.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <AsyncElegantOTA.h>//mist, der braucht den ESPAsyncWebServer, habe mal in der Bibliothek angepasst, so dasss es auch it ESPAsyncWebSrv.h geht
#include <LittleFS.h> //gehoert seit 2.0 zum core, den habe ich, also sollte es kein Thema seinlitt
#include <ArduinoJson.h>   
#include <SPI.h>
#include <MFRC522.h>
#include "credentials.h"
#include "ownLists.h"
#include "index_htmlWithJS.h" //variable mit dem HTML/JS anteil
/*
 * Leider probleme mit dem selbst gebastelten (nach diversen Tutorials) webserver
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

#define RELAIS_PIN 16
#define IRQ_PIN 15 


//erzeugen einer Objektinstanz
MFRC522 mfrc522(SS_PIN, RST_PIN);

//lese / schreibe nur sektor 1 der Karte
byte trailerBlock   = 7; //in Block 7 muesste der Schluessel A AccesBytes Schluessel B für Sektor 1 stehen
byte dataBlock      = 4; //Dort Beginn der Daten von Sektor 1

bool simuliereChipWedelnByTesteintrag = false;

AsyncWebServer server(80);
//fuer den Websocket
AsyncWebSocket ws("/ws");
unsigned long keepWebServerAlive = 0; //240000; //milliseconds also 4 Minuten, danach wird wifi abgeschaltet.
//in dieser Zeit ein client connect -> der Server bleibt aktiv, 0 er bleibt aktiv, lohnt nicht, bringt bei 12 V nur 6mA gewinn
unsigned long startTime;


//ein wenig was zum OTA Update, nicht getestet, keine Ahnung, ob das mit dem Asynchronous Webserver funktioniert
//nein, es geht nicht, es geht mit dem AsyncWebServer einfacher

//---------------------------------------------
//meine Listen
const int RFID_MAX = 8;
ObjectList <Rfid> rfidsNew(RFID_MAX); // maximal 8 neue, brauche keine Dateinamen
ObjectList <Rfid> rfidsOk(RFID_MAX,"/rfidsOk.dat"); 

ObjectList <String> startmeldungen(16); //dient zum Puffern der Meldungen am Anfang

Rfid newRfid;

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
  String result = "";
  if (var == "RFID_MAX")
  {
    result = String(RFID_MAX);
  }
  else if (var == "UPDATE_LINK")
  { 
    result = "<a href=\"http://" + WiFi.localIP().toString() +"/update\"> Update </a>";
  }
  return result;
}



void wsMessage(const char *message)
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
        String message;
        const int capacity = JSON_OBJECT_SIZE(3)+RFID_MAX*JSON_OBJECT_SIZE(3)+JSON_ARRAY_SIZE(RFID_MAX); //
        //bisher ein Objekt mit 3 membern und RFID_MAX Objekte mit 3 membern (die authorisierten rfids, maximal RFID_MAX), aber das ist ein Array mit RFID_MAX Objekten, 
        //lieber auf Nr sicher gehen
        //Serial.println("Rfids1");
        //rfidsOk.serialPrint();
        //Serial.println("new");
        //rfidsNew.serialPrint();
        
        StaticJsonDocument<capacity> doc;
        Serial.println("we have in WebSocketMessage: ");
        Serial.println(cData);
        
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
          if(strcmp(doc["action"],"addNew") == 0) //neuer Eintrag vom WebInterface  in die liste der neuen (zum Testen)
          {
            newRfid = Rfid(doc["rfid"].as<String>(),doc["owner"].as<String>(),doc["extraData"].as<String>());
            wsMessage("neues RFID hinzugefuegt, nicht authorisiert");
            handleNewRFID(false); //false -> kein Zugriff auf die Karte, es ist ja keine da :-)
            //addRfidToNewList(newRfid); wird in handle... mit gemacht
          }
          else if (strcmp(doc["action"],"getStartmeldungen")==0) 
          {
            wsMessage(startmeldungen.htmlLines().c_str());
          }
          else if (strcmp(doc["action"],"keepWebServerAlive")==0) 
          {
            startTime = millis();
            keepWebServerAlive = (unsigned long ) doc["time"]; //0 fuer ewig
            Serial.print("Zeit : ");
            Serial.println((unsigned long ) doc["time"]);
            String msg("Zeit bis zum Stopp des Webservers: ");
            msg += keepWebServerAlive; 
            wsMessage(msg.c_str());
          }
          else if(strcmp(doc["action"],"clearNew") == 0)
          {
            rfidsNew.clear();
            ws.textAll("{\"action\":\"clearNew\"}");
            wsMessage("Neu gelesene geloescht");
          }
          else if (strcmp(doc["action"],"addAuthorized") ==0)
          {
              String id =  (const char *) doc["rfid"];
              String extraData = (const char *) doc["extraData"];
              String owner = (const char *) doc["owner"];
              Rfid rfid(id,owner,extraData);  
              bool result = rfidsOk.addNew(rfid);
              if (result)
              {
                int idToDel = rfidsNew.indexOfOnlyId(rfid); //in meiner Liste ist der owner noch nicht drin
                rfidsNew.deleteAt(idToDel);
                rfidsOk.saveToFile();
                ws.textAll("{\"action\":\"addAuthorized\",\"rfid\":\""+rfid.id +
                        "\",\"extraData\":\""+rfid.extraData + 
                        "\",\"owner\":\""+rfid.owner+"\"}");
                //da er nur von new kommen kann, impliziert das das löschen in der New-Liste
                wsMessage("hinzugefuegt zu  authorisierten");
              }
              else 
                wsMessage("Versuch zu viele in die authorized Liste einzufuegen oder nicht eindeutig, abgelehnt");
          }
          else if (strcmp(doc["action"],"removeAuthorized") ==0)
          {
              Serial.println("Try to delete from authorized " ); 
              String id =  (const char *) doc["rfid"];
              String owner = (const char *) doc["owner"];
              String extraData = (const char *) doc["extraData"];
              Serial.println( id + " " + owner + " " + extraData + " sind die daten");
              message = "{\"action\":\"removeAuthorized\",\"rfid\":\"" +id;
              message += "\",\"extraData\":\""+extraData;
              message += "\",\"owner\":\""+owner+"\"}";
              ws.textAll(message);
              
              Rfid rfid(id,owner,extraData);               
              wsMessage("entferne von authorisierten");
              rfidsOk.deleteEntry(rfid);
              rfidsOk.saveToFile();
              //fueger zur NewListe hinzu
              addRfidToNewList(rfid);
          }
          /*
          else if (strcmp(doc["action"],"addAuthorizedAll") == 0) //alle uebertragen, wird nicht mehr verwendet
          {//es fehlt auch eine Nachricht an alle Clients
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
          }*/
          else if (strcmp(doc["action"],"getRfidsOk") == 0)
          { //man sollte doc nicht wieder verwenden, das kann memory leaks geben 
            StaticJsonDocument<capacity> ans;
            ans["action"] = "newOkList";
            JsonArray data = ans.createNestedArray("data");
            int anzahl = rfidsOk.getDelimiterPos();
            for (int i = 0; i < anzahl; ++i)
            {
              Rfid rfid = rfidsOk.getAt(i);
              JsonObject o = data.createNestedObject();
              o["rfid"]=rfid.id;
              o["owner"]=rfid.owner;
              o["extraData"] = rfid.extraData;
            }
            //serializeJson(ans,Serial); //das sieht gut aus
            int len = measureJson(ans)+16;//statt +1 +16
            char * buf = new char[len];
            serializeJson(ans,buf,len);
            wsMessage("Send authorized rfids from Server");
            ws.textAll(buf);
            delete [] buf;
          }
          else if (strcmp(doc["action"],"getRfidsNew") == 0)
          { //man sollte doc nicht wieder verwenden, das kann memory leaks geben 
            StaticJsonDocument<capacity> ans;
            ans["action"] = "newNewList";
            JsonArray data = ans.createNestedArray("data");
            int anzahl = rfidsNew.getDelimiterPos();
            for (int i = 0; i < anzahl; ++i)
            {
              Rfid rfid = rfidsNew.getAt(i);
              JsonObject o = data.createNestedObject();
              o["rfid"]=rfid.id;
              o["extraData"] = rfid.extraData;
              o["owner"]=rfid.owner;   
            }            
            //serializeJson(ans,Serial); //das sieht gut aus
            wsMessage("Send new rfids from Server");
            int len = measureJson(ans)+16;//statt +1 +16
            char * buf = new char[len];
            serializeJson(ans,buf,len);
            ws.textAll(buf);
            delete [] buf;
          }          
        }
        //Serial.println("Rfids2");
        //rfidsOk.serialPrint();
        //Serial.println("new");
        //rfidsNew.serialPrint();
        
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
            //bei einem connect werden die Startmeldungen ausgegeben       
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


void mountFileSystem()
{ // versuchen, ein vorhandenes Dateisystem einzubinden
  if (!LittleFS.begin(false)) 
  {
    Serial.println("LittleFS Mount fehlgeschlagen");
    startmeldungen.add("LittleFS Mount fehlgeschlagen, formatiere");
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
  startmeldungen.add("Dateisystem");
  startmeldungen.add("Bytes total: " + String(LittleFS.totalBytes()));
  startmeldungen.add("Bytes used: " + String(LittleFS.usedBytes()));
  
}


bool checkTypeAndAuthorize(byte * extraData, byte & size)
{
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  String typeName = String("Kartentyp: ") + mfrc522.PICC_GetTypeName(piccType);
  Serial.println(typeName);
  String message;
  wsMessage(typeName.c_str());
  // Check for compatibility
  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
    &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
    &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) 
  {//evtl. geht es nur mit der MIFARE_1K
    Serial.println(F("geht nur mit MIFARE Classic cards."));
    wsMessage("klappt nur mit MIFARE Classic");
    return false;
  }

  bool result = true;
  //versuche mich bei der Karte zu authorisieren 
  MFRC522::StatusCode status;  
  MFRC522::MIFARE_Key key;
  const byte standardKey[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  //versuche meinen Schluessel 
  for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) 
        key.keyByte[i] = myKey[i];
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) 
  { 
    message = "PCD_Authenticate() failed mit eigenem key: ";
    message += mfrc522.GetStatusCodeName(status);
    message += " -> versuche Standardkey";
    Serial.println(message);
    wsMessage(message.c_str());
    for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) 
        key.keyByte[i] = standardKey[i];
    // http://arduino.stackexchange.com/a/14316 vor jedem neu authentifizieren 
    while(!mfrc522.PICC_IsNewCardPresent()){ delay(1);}
    while(!mfrc522.PICC_ReadCardSerial()){delay(1);} //neu lesen 
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) 
    {
      message ="Fail auch mit Standardkey, gebe auf: ";
      message += mfrc522.GetStatusCodeName(status);
      Serial.println(message);
      wsMessage(message.c_str());
      result = false; //problem, kannn mich gar nicht authorisieren 
    }
    else 
    {
      Serial.println("ok, mit Standard-key authentifiziert, versuche diesen zu ueberschreiben");
      wsMessage("ok, mit Standard-key authentifiziert, versuche diesen zu ueberschreiben");
      //siehe (sehr kurz) https://stackoverflow.com/questions/28050718/mifare-1k-authentication-keys/28051227#28051227
      //siehe https://web.archive.org/web/20150706115501/http://www.nxp.com/documents/data_sheet/MF1S503x.pdf
      //https://arduino-projekte.webnode.at/meine-projekte/zugangskontrolle-mit-rfid/access-bits/
      //verstehe die details jedoch nicht ganz, calculator: http://calc.gmss.ru/Mifare1k/ -> liefert auch FF 07 80 
      //denke, dass da sector A gesetzt ist nicht mit key B authentifiziert werden kann 
      byte buffer[16]; //kann nur ganze Blöcke schrieben
      for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++)
      { 
        buffer[i] = myKey[i]; //0 bis 5
        buffer[10+i] = myKey[i]; //GP auf FF, key B von 10 bis 15
      }
      buffer[6]=0xFF;
      buffer[7]=0x07;
      buffer[8]=0x80; //access bits, only key A used, dachte ich, aber ich konnte mit keyB lesen und schreiben
      buffer[9]=0xFF;
      //daher setze beide keys gleich 
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(trailerBlock, buffer, 16);
      //beachte: keyA ist nicht lesbar, er liefert immer die 00 beim read
      if (status != MFRC522::STATUS_OK) 
      {
        message = "MIFARE_Write() failed for own key:  ";
        message += mfrc522.GetStatusCodeName(status);        
      }
      else
        message = "Key A von sektor 1 (Block 7) mit eigenem Key ueberschrieben, Key B ebenso, Access gesetzt ";   
      Serial.println(message);
    }        
  }
  else 
  {
    Serial.println("ok, mit eigenem key authentifiziert");
    wsMessage("ok, mit eigenem key authentifiziert");    
  }
  if (result) //lese die extraData
  {
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(dataBlock, extraData, &size);
  }
  return result;  
}


//zur liste hinzufuegen, ggf. erstes element löschen, clients informieren 
void addRfidToNewList(Rfid & newRfid)
{
  String removeFirst = "\",\"removeFirst\":false}";
  int found = rfidsNew.indexOf(newRfid);
  int anzahl = rfidsNew.getDelimiterPos();
  if (anzahl == RFID_MAX && found == -1)
  {
    removeFirst = "\",\"removeFirst\":true}";
    Serial.println("remove first");
    wsMessage("neue RFID, Liste voll, erste entfernen");
    rfidsNew.deleteAt(0);
  }
  if (rfidsNew.addNew(newRfid)) //new changed
  {
      ws.textAll("{\"action\":\"addNew\",\"rfid\":\""+newRfid.id+"\",\"extraData\":\""
                    +newRfid.extraData+"\",\"owner\":\""+newRfid.owner + removeFirst);
      //baue mein json objekt selbst, na gut, doch ein wenig laestig
      //die uebertragung der anderen daten wird per post gemacht und dann liefere ich die ganze Seite selbst - ansonsten wuerde sich ArduinoJson wohl doch lohnen
      //nein, ich denke, das mache ich auch über die WebSockets, mal sehen, wie ich sende, also doch websockets
      wsMessage("Neues Element in die Liste der gelesenen aufgenommen");
  }
}


//behandle Rfid, das vom reader gelesen wurde, wird zur liste der neuen hinzugefuegt falls neu, falls vorhanden wird das relais geschaltet
void handleNewRFID(bool rwPhysicalCard) {
    static unsigned long triggerStartTime = 0;
    const unsigned long waitAfterTriggered = 2000; 

    //const int capacity = JSON_OBJECT_SIZE(3); //zu erweitern, wenn ich die liste uebertrage
    //bisher ein Objekt mit 3 membern
    //StaticJsonDocument<capacity> doc;    
    //auf schalten checken, nur wenn nicht vor 2 Sekunden bereits geschaltet wurde
    unsigned long time = millis() - triggerStartTime;
    byte extraData[18]; //16 sollten reichen im readWrite Beispiel werden 18 verwendet
    byte size = sizeof(extraData); 
    String message;
      
    //if (newRfid.valid)
    {
      if (time < waitAfterTriggered) //beim ersten Aufruf sicher nicht 
      {        
        return;
      }
      triggerStartTime = millis(); //aber wenn sehr schnell nochmal gerufen schon

      //pruefe den Kartentyp und schreibe meinen eigenen Schluessel, falls noch der Standardschluessel gesetzt
      //und lesse aus sektor 1 die extra daten (16 Byte)
      if (rwPhysicalCard)
      {
        bool result = checkTypeAndAuthorize(extraData,size);
        if (!result)
        {
          message = "Die Karte kann ich nicht verarbeiten";
          Serial.println(message);
          wsMessage(message.c_str());
          return; 
        }//ansonsten bin ich authorisiert
        newRfid.setExtraData(extraData);
      }
      /* folgende Fälle sind moeglich
        1) Karte ist neu dazugekommen -> sie kann hinzugefuegt werden
        2) Karte ist vorhanden, aber die extraData stimmen nicht -> kopierte Karte verwendet - kritisch?
        3) Karte ist vorhanden, extraData stimmen -> öffnen, extraData ändern, auf Karte und in chip speichern
      */
      
       //if (rfidsOk.indexOf(newRfid) != -1)
      if (rfidsOk.indexOfOnlyId(newRfid) != -1) //authorisierung verbessern
      {
        message = newRfid.id + " ist authorisiert, schalte das Relais";
        wsMessage(message.c_str());
        //triggerStartTime = millis();
        digitalWrite(RELAIS_PIN,HIGH); //200ms auf high müsste zum schalten reichen?
        delay(200);
        digitalWrite(RELAIS_PIN,LOW);
      }
      else
      {//nicht vorhandenes gelesen
        addRfidToNewList(newRfid);
      } 
    }//ende newRfid gueltig, denke das brauche ich nicht
}
void IRAM_ATTR isr() {
  Serial.println("falling on irq pin");
}
void setup() {
  //pin fuer das relais 
  pinMode(RELAIS_PIN, OUTPUT);
  digitalWrite(RELAIS_PIN,LOW);//keine Ahnung, ob das noetig ist 
  //beginn der seriellen Kommunikation mit 115200 Baud
  //pinMode(IRQ_PIN, INPUT_PULLUP); //das geht nicht - nochmal schauen az sagt, dass ein interrupt kommt
  //attachInterrupt(IRQ_PIN, isr, FALLING);
  Serial.begin(115200);
  
  mountFileSystem();
  rfidsOk.loadFromFile(); 
  startmeldungen.add("Rfids geladen");

  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) //waitForConnect lässt sich viel Zeit
  {
    Serial.printf("WiFi Failed! - second try \n");
    startmeldungen.add("WiFi zweiter Versuch");
    WiFi.disconnect(true);  // Disconnect from the network
    ssid = mySSID2;
    password = myPASSWORD2;
    WiFi.begin(ssid, password);
    WiFi.waitForConnectResult();
  }
  //auch zweites Netz nicht da, setze eigenen AP auf - ich hatte zu Hause in Abhängigkeit vom Router massive 
  //Probleme in das Netz zu kommen
  if (WiFi.status() != WL_CONNECTED)
  {
     WiFi.disconnect(true);  // Disconnect from the network
     ssid     = apSSID;     //wieder in credentials.h
     password = apPASSWORD;  
     Serial.printf("WiFi Failed twice! - go to ap mode \n");
     startmeldungen.add("WiFi zweimal fehlgeschlagen, ap mode");
     WiFi.softAP(ssid, password);   
  }
  if (WiFi.status() == WL_CONNECTED) //wifi connected
  {
    Serial.print("Wifi Connected, adress ");
    Serial.println(WiFi.localIP());
    String msg = String("Wifi connected ,adress ") + WiFi.localIP().toString();
    startmeldungen.add(msg.c_str());
  }
  //Der Rest sind doch nur callbacks und der WebSocket-Server sollte gehen 

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
  ws.onEvent(onWSEvent);
  server.addHandler(&ws); //WebSocket dazu 
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  //andernfalls startet er ohne wifi, registrieren von Elementen nicht möglich, nein habe mal einen ap-modus gesetzt  
  //eine kleine Pause von 50ms.
  delay(50);
  //begin der SPI Kommunnikation
  SPI.begin();
  //initialisieren der Kommunikation mit dem RFID Modul
  mfrc522.PCD_Init();
  startTime = millis();
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details

}

void loop() {
  static String lastRfid = "";
  long running = millis()-startTime;
  if (keepWebServerAlive && running > keepWebServerAlive) //wenn keepWebServerAlive gesetzt und die Laufzeit größer ist, dann herunter fahren
  {
      wsMessage("esp says: Zeit abgelaufen, stoppe Server, rfid is running");
      ws.closeAll();
      server.end();
      WiFi.disconnect(true);  // Disconnect from the network
      WiFi.mode(WIFI_OFF);    // Switch WiFi off
  }
  
  if (WiFi.status() == WL_CONNECTED)
  {
    ws.cleanupClients(); // aeltesten client heraus werfen, wenn maximum Zahl von clients ueberschritten, 
                       // manchmal verabschieden sich clients wohl unsauber / gar nicht -> werden wir brutal  
  }  
  
  if(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
  {
    String newRfidId = mfrc522.uid.uidByte[0] < 0x10 ? "0" : "";
    newRfidId.concat(String(mfrc522.uid.uidByte[0], HEX));
    for (byte i = 1; i < mfrc522.uid.size; i++) 
    {
      newRfidId.concat(mfrc522.uid.uidByte[i] < 0x10 ? "-0" : "-");
      newRfidId.concat(String(mfrc522.uid.uidByte[i], HEX));
    }     
    //überschreiben der alten ID mit der neuen
    newRfid = Rfid(newRfidId,"");
    String msg = "gelesene RFID-ID: " + newRfid;
    Serial.println(msg);
    //wsMessage(msg.c_str());    // Dump debug info about the card; PICC_HaltA() is automatically called
    //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

    handleNewRFID(true); //wenn neu, dann hinzufügen ermöglichen, wenn vorhanden, dann schalten
                         //true: karte authorisieren, auslesen, ggf. schreiben  
    //newRfid.valid = false;

    // Halt PICC - die folgenden zwei scheinen noetig zu sein, damit er neu lesen kann+
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();

  }

    
}
