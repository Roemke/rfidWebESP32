# rfidWebESP32

RFIDs lesen und Relais schalten
RFIDs lesen und den Erlaubten hinufügen
RFIDs aus Liste der Erlaubten entfernen 

-> kleiner Webserver (war in c++ lästiger als in python)
-> rfid Bibliothek

Erster Versuch noch mit eigenem Webserver, jedoch Fehlermeldungen im Betrieb, nach etwas Recherche stelle um
auf https://github.com/dvarrel/ESPAsyncWebSrv das ist ein Fork von https://github.com/me-no-dev/ESPAsyncWebServer
dahinter steckt dann Hristo Gochkov's ESPAsyncWebServer

Eine Zeit gebastelt, Details siehe unten und es klappt.


## ein paar Details / Notizen
erstmal nach Heise verdrahtet:
https://www.heise.de/select/make/2022/3/2211713103126463800
ESP32 D1 Mini genommen jeweils nur die innere Leiste gelötet
das reicht


5V Wandler verwendet da noch 5V relais verfügbar
zuordnung
SDA 	GPIO21
SCK 	GPIO18
MOSI 	GPIO23
MISO 	GPIO19
IRQ 	-
GND 	GND
RST 	GPIO22
3,3V 	3,3V 

Änderung: GPIO12 geht auf Schalter von Relais bei Heise, nehme einen 
anderen vermutlich GPIO10 nehmen. Nein der geht nicht, das Relais 
schaltet wild beim Einschalten des ESP - es steht auch in der Anleitung, das nicht 
empfohlen
versuche 26 - geht gar nicht :-(, aber 16 problemlos :-) (später ermittelt)

Die Bibliothek von Bilboa nach installiert rfid_lesen per arduino hochgeschoben
er zeigt im seriellen monitor aber nix an
-> kann ja nur falsch verkabelt sein doch geht, masse war nicht richtig verbunden


gpio 27 noch ein beinchen gelötet, ist rtc-17 muesste den esp aus dem deep sleep 
holen können, geht auch, verwende ich nicht, gpio15 muesste genauso gehen  

scheint zu gehen, habe gpio parallel zu sck geschaltet und er wacht auf, wenn man 
mit dem rfid teil wedelt aber er wacht auch mal so auf, vielleicht sollte man 
ein anderes teil nehmen 
Die anderen gehen auch nicht (Miso getestet) auch mal per pull down auf masse.
irq kommt nach beschreibung nicht, kann also nicht genutzt werden 
eigentlich sehr seltsam, aber nicht zu ändern. Wenn deepsleep dann ueber touch (das ist 27 auch)
aber dann müsste ich den Kontakt irgendwie nach draußen herstellen, noch nicht implementiert.
irq getestet, scheint nicht zu gehen 

Baue Programm so, dass zumindest Wifi nach einiger Zeit herunter geht. 
Verbindet sich jemand über http, dann bleibt der Server wach 

Beste Quelle fuer Informationen: https://m1cr0lab-esp32.github.io/remote-control-with-websocket/websocket-and-json/


Ota: wenn die Platine eingebaut ist, dann ist ein normales update nicht unbedingt möglich. 

Daher OTA - aber das lief nicht mit dem Async Webserver. Schade. 

Daher AsyncElegantOta -> Achtung, musste im h-File einen include aendern, statt ESPAsyncWebServer.h ESPAsyncWebSrv.h

##Energiebedarf und Kabel
Am 12,8 Volt nimmt er ca 90mA. Bei WebAktivität und schalten natürlich mehr, schwer zu sehen, ich denke so um die 150mA.
WiFi aus -> es sind noch 84mA, da hatte ich mir weniger Energie erhofft. Mal sehen, ob man noch etwas abschalten kann.
Also in dem Zustand ein Watt, finde ich zu viel :-(, bringt mich aber nicht um 
Hoffe, die Einschaltströme sind nicht zu hoch.
Kabelquerschnitte dürften für die Energie recht egal sein. Versuche mal für die langen Kabel zum RFID-Leser 0,25mm²

Sparen lässt sich durch Verwendung eines "reinen ESP". Dann fällt der USB Kram weg, aber ich habe eigentlich keine
Lust mehr dazu. Sinnvoll wäre vielleicht doch der deep-Sleep. Ach nee, alles zuviel Aufwand...



