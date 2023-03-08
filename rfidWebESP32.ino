//einbinden der Bibliotheken für das
//ansteuern des MFRC522 Moduls
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include "credentials.h"
#include "ownLists.h"
/*
 * Leider probleme mit dem selbst gebastelten (nach diversen Tutorials webserver, es kommt zu unklaren file not found pech gehabt meldungen 
 * also den eigenen. 
 * Erster Versuch noch mit eigenem Webserver, jedoch Fehlermeldungen im Betrieb, nach etwas Recherche stelle um
 * auf https://github.com/dvarrel/ESPAsyncWebSrv das ist ein Fork von https://github.com/me-no-dev/ESPAsyncWebServer
 * dahinter steckt dann Hristo Gochkov's ESPAsyncWebServer
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

//webserver - ich vermute hier gibt es eine bessere Variante, habe immer wieder probleme mal geht es mal nicht 
//vermute, dass ich etwas uebersehe? 
AsyncWebServer server(80);


//StringList msgs(50); //Speichere Nachrichten, gebe Sie auf der Webseite aus, doch keine so gute Idee
RfidList rfidsNew(8); // maximal 8 neue
RfidList rfidsOk(8); //akzeptierte, sie dürfen öffnen 

//hatte mal irgendein Beispiel gewählt um zu testen
//stelle um analog zu https://werner.rothschopf.net/202001_arduino_webserver_post.htm
//aber der ist mir zu eingeschränkt, limitierte Menge an Post-Data, er verzichtet auf string wahrscheinlich allein aus Gründen des Speicherplatzes

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

  /*
void handleClient(WiFiClient & client)
{
    //String header=""; brauche ich nicht, reagiere nur auf Post 
    enum class Status {REQUEST, CONTENT_LENGTH, EMPTY_LINE, BODY};  
    Status status = Status::REQUEST;
    unsigned long currentTime = millis();
    unsigned long previousTime = currentTime;
    Serial.println("New Client here");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    String uri;
   
    while (client.connected()) 
    {   // loop while the client's connected
      currentTime = millis();
      while (client.available()) 
      {  // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') 
        {                    // if the byte is a newline character
          if (status == Status::REQUEST)         // read the first line
          {
            //Serial.print(F("lineBuffer="));Serial.println(lineBuffer);
            // now split the input
            int index = currentLine.indexOf(' ');
            String method = currentLine.substring(0, index);
            String rest = currentLine.substring(index+1);
            uri = rest.substring(0,rest.indexOf(' '));         
            Serial.print(F("method=")); Serial.println(method);
            Serial.print(F("rest=")); Serial.println(rest);
            if (rest.indexOf('?') != -1) //get geschichten, werden von mir nicht behandelt
            {
            }
            //Serial.print(F("uri=")); Serial.println(uri);
            status = Status::EMPTY_LINE;                   // jump to next status
          }
          else if (status == Status::CONTENT_LENGTH)       // MISSING check for Content-Length
          { //status scheint gar nicht behandelt zu werden 
            status = Status::EMPTY_LINE;
          }
          else if (status > Status::REQUEST && currentLine.length() == 0)      // check if we have an empty line
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          {
            status = Status::BODY;
            Serial.println("Switch to status::body");
          }
          else if (status == Status::BODY)
          { //ich denke, das werde ich nie erreichen, jedenfalls nicht beim firefox 
            //body lesen, eine zeile, die Daten sollten wir unten lesen
            Serial.println("In status::body");
            break; // we have received one line payload and break out
          }
          //Line zurueck setzen   
          currentLine = ""; //koennte ein Problem sein, falls oben doch in status::body Zweig des if, nein, break verlaesst die schleife  
        } // byte ein newline 
        else if (c != '\r') 
        {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }//ende character kann gelesen werden / while client.available
      if (status == Status::BODY && currentLine.length() > 0)
      {
        Serial.println("Have to handle body data: " + currentLine);
      }
      
      Serial.println("\nSend a page, have uri " + uri);
      uri.toLowerCase();
      if (uri.indexOf("favicon") != -1)
        send204(client);//no content
      else if (uri.equals("/"))
        sendPage(client);
      else 
        send404(client); //not found
      // Close the connection 
      client.stop();
      Serial.println("Client disconnected after sending page.");
      Serial.println("");

    }//ende while schleife  
    client.stop();
    Serial.println("Stopped client at end of handle Client");
}*/

/*
void sendPage(WiFiClient & client)
{
  // Serial.println("[server] 200 response send");
  client.println( "HTTP/1.0 200 OK\r\n"          // \r\n Header Fields are terminated by a carriage return (CR) and line feed (LF) character sequence
                  "Content-Type: text/html\r\n"  // The media type of the body of the request (used with POST and PUT requests)
                  "\r\n"                         // a blank line to split HTTP header and HTTP body
                  "<!doctype html>\n"            // the start of the HTTP Body - contains the HTML
                  "<html lang='en'>\n"
                  "<head>\n"
                  "<meta charset='utf-8'>\n"
                  "<meta name='viewport' content='width=device-width'>\n"
                  "<title>Webserver RFID</title>\n"
                  "<style>\n"
                  " div { \n"
                  "   border: 1px solid black; \n"
                  "  }\n"
                  "</style>\n"
                  "</head>\n"
                  "<body style='font-family:Helvetica, sans-serif'>\n" // a minimum style to avoid serifs
                  "<h1>Webserver for RFID config</h1>\n"
                  "<form method='post' action='/' name='rfid'>\n"
                  "<p>gelesene RFIDs</p>\n");
  String lines =  rfidsNew.htmlLines("new");
  Serial.println(lines);
  client.println(lines);
  client.println("<button type='submit'>Aktualisiere</button>\n"
                  "</form>\n");
                  
   client.println("</body></html>\n");
  // The HTTP response ends with another blank line
  client.println();                
}
*/
//fehlerhafte Anfrage, Seite nicht da
void send404(WiFiClient &client)
{
  client.println( ("HTTP/1.0 404 Not Found\r\n"
                   "Content-Type: text/plain\r\n"
                   "\r\n"
                   "File Fot Found - pech gehabt\n"));
  Serial.println("Serve file not found");
  client.stop();
}
//no content bei Anfrage an favicon
void send204(WiFiClient &client)
{
  client.println( ("HTTP/1.0 204 no content\r\n"));
  client.stop();
}

void setup() {
  //beginn der seriellen Kommunikation mit 115200 Baud
  Serial.begin(115200);

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
  
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      String lines = rfidsNew.htmlLines("new");
      String answer = "<!doctype html>\n"            // the start of the HTTP Body - contains the HTML
                    "<html lang='en'>\n"
                    "<head>\n"
                    "<meta charset='utf-8'>\n"
                    "<meta name='viewport' content='width=device-width'>\n"
                    "<title>Webserver RFID</title>\n"
                    "<style>\n"
                    " div { \n"
                    "   border: 1px solid black; \n"
                    "  }\n"
                    "</style>\n"
                    "</head>\n"
                    "<body style='font-family:Helvetica, sans-serif'>\n" // a minimum style to avoid serifs
                    "<h1>Webserver for RFID config</h1>\n"
                    "<form method='post' action='/' name='rfid'>\n"
                    "<p>gelesene RFIDs</p>\n"
                    + lines
                    + "<button type='submit'>Aktualisiere</button>\n"
                    "</form>\n"
                    "</body></html>\n";
      request->send(200,"text/html",answer);
      Serial.println(lines); 
    });
  
    server.begin();
  }
  
  //eine kleine Pause von 50ms.
  
  delay(50);
  //begin der SPI Kommunikation
  SPI.begin();
  //initialisieren der Kommunikation mit dem RFID Modul
  mfrc522.PCD_Init();
}

void loop() {
  static String lastRfid = "";
  if (cCounter <=cCounterMax)
    ArduinoOTA.handle(); //mehr als 5s delay in der loop und der upload wird nicht funktionieren 

 /*
  WiFiClient client = server.available();   // Listen for incoming clients
  
  if (client) {                             // If a new client connects,
    handleClient(client) ;
  }
  */
  if ( !mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }


  String newRfidId = mfrc522.uid.uidByte[0] < 0x10 ? "0" : "";
  newRfidId.concat(String(mfrc522.uid.uidByte[0], HEX));
  for (byte i = 1; i < mfrc522.uid.size; i++) {
    newRfidId.concat(mfrc522.uid.uidByte[i] < 0x10 ? "-0" : "-");
    newRfidId.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  //alle Buchstaben in Großbuchstaben umwandeln
  //newRfidId.toUpperCase();

  if (!newRfidId.equals(lastRfid)) {
    //überschreiben der alten ID mit der neuen
    lastRfid = newRfidId;
    Serial.println("gelesene RFID-ID: " + newRfidId);
    rfidsNew.add(newRfidId,"");
  }
}
