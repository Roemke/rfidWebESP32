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
anderen vermutlich GPIO10 nehmen

Die Bibliothek von Bilboa nach installiert rfid_lesen per arduino hochgeschoben
er zeigt im seriellen monitor aber nix an
-> kann ja nur falsch verkabelt sein doch geht, masse war nicht richtig verbunden

ota eingebaut, diverse quellen

gpio 27 noch ein beinchen gelötet, ist rtc-17 muesste den esp aus dem deep sleep 
holen können 

scheint zu gehen, habe gpio 27 parallel zu sck geschaltet und er wacht auf, wenn man 
mit dem rfid teil wedelt aber er wacht auch mal so auf, vielleicht sollte man 
ein anderes teil nehmen 
Die anderen gehen auch nicht (Miso getestet) auch mal per pull down auf masse.
irq kommt nach beschreibung nicht, kann also nicht genutzt werden 
eigentlich sehr seltsam, aber nicht zu ändern 
