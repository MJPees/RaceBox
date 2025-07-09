# GhostCar / StartingLight LED
Automatisches Starten/Stoppen des CH-Racers über SmartRace oder CH-Racing-Club.
Es wird ein Game-Controller simuliert, der auf Events von SmartRace oder CH-Racing-Club reagiert. Die eingebaute RGB-LED des ESP32-S3 zeigt den aktiven Modus an.<br> Während der Startphase wird launch-control automatisch aktiviert.<br><br>Der genutzte ESP32-S3 mini kann bei Android Phones direkt als USB-GameController verwendet werden sowie als Bluetooth GameController für Android und IOs.<br>
Wird ein ESP32-C3 genutzt so ist nur Bluetooth möglich!<br>Das GhostCar gibt es in verschiedenen Ausbau-Varianten. NUR ESP32, ESP32 mit zwei Tastern zum starten und stoppen und einem optionalen Potentimeter zur Änderung der Geschwindigkeit des GhostCars (ersetzt Geschwindigkeitsangabe im Webinterface).

Die Konfiguration des GhostCars erfolgt über das Webinterface <a href="http://GhostCar">http://ghostcar</a>
<br><br><a href="./script-flasher/README.md">Flash-Anleitungen</a><br><br>
## Benötigte Hardware

- ESP32-S3 mini (Waveshare) oder ESP32C3 (nur Bluetooth)
- USB-C Stecker auf USB-C Stecker (Duttek-Store)
 von Amazon
 - optional zwei Taster<br>
   ESP32S3:<br>
   - (GND <-> Pin 10) & (GND <-> Pin 11)

   ESP32C3:<br>
   - (GND <-> Pin 6) & (GND <-> Pin 7)
 - optional ein Potentimeter (GND <-> Pin 3 <-> 3.3V)
 - optional für  StartingLight-LED 20 x WS2812B (5 x 4er Kette), <a href="../docs/StartingLights.dxf">Platine</a> zum Aufbau.
<br><br>
<img src="../images/CH-GhostCar-SmartRace.jpg" width=424/>
[<img src="../images/Video_GhostCar-StartingLights-Poti.png">](https://youtu.be/PwxAJHPKN4w)