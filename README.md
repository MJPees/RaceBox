# RFID-SmartRace

RFID-SmartRace ermöglicht eine Zeitnahme für z.B. Carrera Hybrid oder Dr!ft von Sturmkind usw. mit <a href="https://www.smartrace.de/">SmartRace</a>. Hierzu wird der analoge Sensormodus von SmartRace genutzt. Die Anbindung erfolgt über WLAN. Die Zeitnahme ist aktuell für 8 Autos möglich (8 Controller in SmartRace).<br>
Des Weiteren können 2 RFID-Ids für einen Controller definiert werden. Somit sind auch Langstreckenrennen mit jeweils zwei Fahrzeugen pro Team möglich.
Der ESP32 geht bei fehlender WLAN-Konfiguration automatisch in einen Accesspoint-Modus.
Über das Webinterface kann RFID-SmartRace konfiguriert werden.
Neben SSID und Passwort des zu verwendenden Routers muss die IP-Addresse und der Port des SmartRace-Servers für den analogen Sensorbetrieb gesetzt werden.
Der Power Level für den RFID-Empfang kann von 10dbm bis 26dbm eingestellt werden.
Die Ids der RFID-Chips für Controller 1-8 werden bei neu erkannten IDs automatisch gefüllt, wenn sie im Webinterface zuvor "leer" sind.
Die optionale ID2 pro Controller muss für Team-Rennen im Webinterface ausgefüllt werden.<br><br>
Die optionale LAP LED wird mit Anode (+) an 3,3V und Kathode (-) an Pin2 (ESP32-DEV) bzw. Pin D8 (ESP32-C3) angeschlossen. (Vorwiderstand nicht vergessen oder LED mit eingebautem Widerstand verwenden!)<br><br>
Im Quellcode kann die ESP32 Version für den ESP32-C3 über einen define
aktiviert werden.<br>
Standard ist der ESP32-Dev mit externer Antenne.

## Konfiguration über Web-Interface:
<img src="./images/Webinterface.png"/>
<br><br>

Als RFID-Leser wird ein R200 der Firma Inveton verwendet. Dieser kann z.B. über AliExpress bezogen werden und liegt inklusive einer 1dbi Antenne mit Versand aktuell bei ca. 50 Euro. 
Des Weiteren wird ein ESP32 benötigt.
Passende RFID-Aufkleber können bei Amazon bezogen werden.

## Beispielhardware/Bezugsquellen:

AliExpress: (Bitte die richtige Auswahl treffen! Meist ist nur ein Aufklebersatz für unter 10 Euro als default selektiert!)<br>
Mir ist aktuell keine Bezugsquelle aus Deutschland bekannt. Der Chip selber arbeitet mit in der EU zulässigen RFID-Frequenzen (wurde in der Software konfiguriert).
<img src="./images/Invelion_R200_1dbi.png"/>
<br><br>
ESP32-WROOM-32U mit externem Antennenanschluss:<br>
https://amzn.eu/d/12kL505
<br><br>
kleine RFID-Tags (Carrera Hybrid):<br>
https://amzn.eu/d/fBFeS80
<br>Anmerkung:<br>
Die Tags haben im Auslieferungszustand alle die gleiche ID (EPC).<br>
Mit Hilfe des Sketches RFID-Label-Writer.ino  kann die ID neu geschrieben werden. Dazu den Sketch auf den ESP32 laden und danach alle Tags einzeln an die Antenne halten. Die ID wird von 1 automatisch hochgezählt. Textausgabe kann über Serial angesehen werden.
<br><br>

## Aufbau der Hardware
### Zu verdrahtende Anschlüsse (im Beispiel zusätzlich abgeschirmt):<br>
R200 5V <--> ESP32 5V<br>
R200 TX <--> ESP32 16<br>
R200 RX <--> ESP32 17<br>
R200 GND <--> ESP32 GND<br><br>
<img src="./images/Invelion_R200.jpg"/>

### Der Einbauort für den RFID-Reader wurde mit Alufolie ausgekleidet um Funk-Störungen zu minimieren.<br><br>(Es hat sich gezeigt, dass die Abschirmung nicht benötigt wird)
<img src="./images/Abschirmung_Alufolie.jpg"/>

### Montage der 1dbi Antenne als Brücke über Start/Ziel:
<img src="./images/Einbau_Antenne.jpg"/>
<br><br>
<img src="./images/Start_Ziel_Antenne.jpg"/>

### RFID-Reader R200 mit ESP32 und externer Antenne in einem Gehäuse.
<img src="./images/RFID-Empfänger_ESP32.jpg"/>

### RFID-Aufkleber unter Carrera Hybrid Fahrzeugen
<img src="./images/Sensoren_Auto.jpg"/>

### Darstellung in SmartRace
<img src="./images/SmartRace.png"/>

### Version mit ESP32-C3
<img src="./images/ESP32C3_1.jpg"/>
<img src="./images/ESP32C3_2.jpg"/>



