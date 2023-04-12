# rfidWebESP32

RFIDs lesen und Relais schalten
RFIDs lesen und den Erlaubten hinufügen
RFIDs aus Liste der Erlaubten entfernen 

-> kleiner Webserver (war in c++ lästiger als in python), aber er kann WebSockets, auch nett, einiges gelernt 
-> rfid Bibliothek

Erster Versuch noch mit eigenem Webserver, jedoch Fehlermeldungen im Betrieb, nach etwas Recherche stelle um
auf https://github.com/dvarrel/ESPAsyncWebSrv das ist ein Fork von https://github.com/me-no-dev/ESPAsyncWebServer
dahinter steckt dann Hristo Gochkov's ESPAsyncWebServer

Eine Zeit gebastelt, Details siehe unten und das schalten geht.


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

Die Bibliothek von Bilboa nach installiert (eine der angeboten, ich glaube nicht das original) 
rfid_lesen per arduino hochgeschoben er zeigt im seriellen monitor aber nix an
-> kann ja nur falsch verkabelt sein doch geht, masse war nicht richtig verbunden


gpio 27 noch ein beinchen gelötet, ist rtc-17 muesste den esp aus dem deep sleep 
holen können, gpio15 muesste genauso gehen - das ist mir nicht gelungen  

scheint zu gehen, habe gpio parallel zu sck geschaltet und er wacht auf, wenn man 
mit dem rfid teil wedelt aber er wacht auch mal so auf, vielleicht sollte man 
ein anderes teil nehmen 
Die anderen gehen auch nicht (Miso getestet) auch mal per pull down auf masse.
irq kommt nach beschreibung nicht, kann also nicht genutzt werden, klappt auch nicht 
eigentlich sehr seltsam, aber nicht zu ändern. Wenn deepsleep dann ueber touch (das ist 27 auch)
aber dann müsste ich den Kontakt irgendwie nach draußen herstellen, noch nicht implementiert.
irq getestet, scheint nicht zu gehen 

Baue Programm so, dass zumindest Wifi nach einiger Zeit herunter geht. 
Verbindet sich jemand über http, dann bleibt der Server wach ... dachte ich, lohnt nicht, 6mA bei 12 V 

Beste Quelle fuer Informationen bzgl. WebSockets: https://m1cr0lab-esp32.github.io/remote-control-with-websocket/websocket-and-json/

Ota: wenn die Platine eingebaut ist, dann ist ein normales update nicht unbedingt möglich. 

Daher OTA - aber das lief nicht mit dem Async Webserver. Schade. 

Daher AsyncElegantOta -> Achtung, musste im h-File einen include aendern, statt ESPAsyncWebServer.h ESPAsyncWebSrv.h

## Energiebedarf und Kabel
Am 12,8 Volt nimmt er ca 90mA. Bei WebAktivität und schalten natürlich mehr, schwer zu sehen, ich denke so um die 150mA.
WiFi aus -> es sind noch 84mA, da hatte ich mir weniger Energie erhofft. Mal sehen, ob man noch etwas abschalten kann.
Also in dem Zustand ein Watt, finde ich zu viel :-(, bringt mich aber nicht um.  
Kabelquerschnitte dürften für die Energie recht egal sein. Versuche mal für die langen Kabel zum RFID-Leser 0,25mm²
Das geht. 

Sparen lässt sich durch Verwendung eines "reinen ESP". Dann fällt der USB Kram weg, aber ich habe eigentlich keine
Lust mehr dazu. Sinnvoll wäre vielleicht doch der deep-Sleep. Ach nee, alles zuviel Aufwand...

Verdrahtung in der Datei platine.txt notiert.

## Änderungen
Das schalten nur über die UID ist natürlich nicht sonderlich sicher. Bei den MIFARE 1k Karten (die sind günstig) gibt es zwar die Möglichkeit
einen Schlüssel zu verwenden, der ist aber auch relativ schnell geknackt, damit ist eine kopierte Karte schon recht leicht zu erstellen. 
Folgendes Vorgehen macht es eventuell etwas sicherer. 

* Wird eine neue Karte gelesen, so kommt sie in die newList (Webserver zeigt sie)
* der Owner wird eingetragen, sie kommt per Knopfdruck in die authorisierte Liste 
* beim erneuten Wedeln vorm Leser wird der owner gesetzt. 
* die extraData sind willkürlich, es handelt sich um Hex-Werte, sie werden in den Speicher der RFID-Karte geschrieben
* nur bei einer Übereinstimmung aller 3 Werte wird geschaltet. 
* Beim Schalten wird ein neuer Eintrag für die extraData erzeugt und gespeichert (im ESP und auf der Karte). 
  Damit ist eine Kopie nur solange gültig, bis die Originalkarte vor das Lesegerät gehalten wurde
  (Das ganze wird über Zufallszahlen gemacht und sicher kann man auch aus einem Wert den nächsten berechnen, aber ich denke mit einem Hammer ist man schneller im Kastenwagen).

## Nette Nebensache 
Ich wollte ursprünglich nur eine Möglichkeit haben, den Kastenwagen auch zu öffnen, wenn der Funkschlüssel nicht mehr funkt. 
Das geht nicht, wenn man die Vordertüren über ein Stahlseil verbunden hat, der normale Schlüssel öffnet nur die Vordertüren. Aber er sorgt 
dafür, dass auch der Schalter an der Mittelkonsole wieder funktioniert. Einmal Drücken schließt ab, nochmal alles wieder auf. Das geht 
dann auch mit dem Relais.  

## Danke, Sonstiges, Probleme 

* an die Autoren unzähliger Codeschnipsel, die ich gefunden habe - wirklich schwierig fand ich nur den Zugriff auf die RFID-Karte, mir 
ist immer noch nicht klar, wann man welche Funktion aufrufen muss - so läuft es auf jeden Fall erst mal :-) 
* an die Menschen aus dem Pösslforum, dort habe ich bei Romeo zum ersten mal von der Idee gelesen und die Bilder von Mats waren extrem hilfreich. 

Natürlich kann das ganze nach Belieben verwendet werden, wenn Fehler auffallen gern eine Nachricht an mich.

Es wird sicher einige Fehler geben, ich habe Jahre kein C/C++ mehr geschrieben, ich bitte um Nachsicht ...

Probleme: Der Leser hängt sich manchmal auf, wenn es einen Fehler gibt, daher einen Knopf in's Webinterface gesetzt, der ein 
Reset des Lesers ermöglicht. Hatte aber einmal den Fall, dass dies nicht reichte - per reset des ESP ging es dann wieder. Hmm, 
Fehler treten natürlich eher auf, wenn die ExtraData jedesmal gelesen und geschrieben werden. Vielleicht gibt es noch eine 
bessere Möglichkeit den Leser zu resetten, so bleibt es erstmal bei der Möglichkeit den ESP per Web-Interface neu zu starten.

Bisher keine Möglichkeit gefunden, den ESP in den Tiefschlaf zu versetzen und ihn daraus wieder mit dem RFID-Leser aufzuwecken.  
 
