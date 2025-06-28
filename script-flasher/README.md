# RaceBox Script-Flasher

Das Flashen aktuell unterstützen ESP32 erfolgt durch Herunterladen des entsprechenden Zip-Files und ausführen des enthaltenen Skriptes. Es wird aktuell Windows und Linux unterstütz.<br><br>
Es sind folgende "Boxen" verfügbar:
## RaceBox
Der RFID-Empfänger zur Erfassung der Fahrzeuge und Übertragung der Zieldurchfahrten an SmartRace oder CH-Racing-Club. <a href="./RaceBox-ESP32-DEV.zip">RaceBox-ESP32-DEV.zip</a>

## RaceBox-GhostCar

#### ESP32-S3 (Bluetooth-GhostCar und USB-GhostCar)

- ESP32-S3 ohne zusätzliche Verdrahtung oder nur mit Start/Stop-Taster.<br>
<a href="RaceBox-GhostCar-ESP32-S3-NoPoti.zip">RaceBox-GhostCar-ESP32-S3-NoPoti.zip</a>

- ESP32-S3 mit Potentimeter zur Geschwindigkeitsanpassung und optional Start/Stop-Taster.<br>
<a href="RaceBox-GhostCar-ESP32-S3-Poti.zip">RaceBox-GhostCar-ESP32-S3-Poti.zip</a>

#### ESP32-C3 (Nur ein Bluetooth-GhostCar)

- ESP32-C3 ohne zusätzliche Verdrahtung oder nur mit Start/Stop-Taster.<br>
<a href="RaceBox-GhostCar-ESP32-C3-NoPoti.zip">RaceBox-GhostCar-ESP32-C3-NoPoti.zip</a>

- ESP32-C3 mit Potentimeter zur Geschwindigkeitsanpassung und optional Start/Stop-Taster.<br>
<a href="RaceBox-GhostCar-ESP32-C3-Poti.zip">RaceBox-GhostCar-ESP32-C3-Poti.zip</a>

## RaceBox-GhostCar-StartingLights
Diese Version benötigt 20 ws2812b leds!<br>Die Ampel besteht aus 5 LEDs in 4 Reihen.<br>Eine Platine zum "leichteren" Aufbau (aktuell nur CNC-Fräsdaten) wird nachgereicht. Unterstützt wird SmartRace sowie CH-Racing-Club.

#### ESP32-S3 (Startampel + Bluetooth-GhostCar + USB-GhostCar)

- ESP32-S3 mit Startampel, optional Start/Stop-Tastern, aber ohne Potentimeter zur Geschwindigkeitsanpassung.<br>
<a href="RaceBox-GhostCar-StartingLights-ESP32-S3-NoPoti.zip">RaceBox-GhostCar-StartingLights-ESP32-S3-NoPoti.zip</a>

- ESP32-S3 mit Startampel, Potentimeter zur Geschwindigkeitsanpassung und optional Start/Stop-Tastern.<br>
<a href="RaceBox-GhostCar-ESP32-S3-Poti.zip">RaceBox-GhostCar-StartingLights-ESP32-S3-Poti.zip</a>

#### ESP32-C3 (Starampel + Bluetooth-GhostCar)

- ESP32-C3 mit Startampel, optional Start/Stop-Tastern, aber ohne Potentimeter zur Geschwindigkeitsanpassung.<br>
<a href="RaceBox-GhostCar-StartingLights-ESP32-C3-NoPoti.zip">RaceBox-GhostCar-StartingLights-ESP32-C3-NoPoti.zip</a>

- ESP32-C3 mit Startampel, Potentimeter zur Geschwindigkeitsanpassung und optional Start/Stop-Tastern.<br>
<a href="RaceBox-GhostCar-StartingLights-ESP32-C3-Poti.zip">RaceBox-StartingLights-GhostCar-ESP32-C3-Poti.zip</a>

## RaceBox-GhostCar-StartingLightsDisplay
Einfach zu fertigende und optisch sehr ansprechende Startampel basierend auf einem ESP32-C6 mit montiertem kleinen Display. (Inklusive Werbebannern im idle-mode)<br>
<a href="RaceBox-StartingLightsDisplay-ESP32-C6.zip">RaceBox-StartingLightsDisplay-ESP32-C6.zip</a>


## RFID-Label-Writer
Mit dem RFID-Label-Writer können neue RFID-Tags mit gleicher ID einmalig neu beschrieben werden.
Die Firmware kann einfach auf die RaceBox temporär aufgespielt werden. Anschliessend alle Tags <b>nacheinander</b> einmal über die Antenne ziehen.
Die Ids werden von 1 an mit jedem Tag hochgezählt und sind somit eineindeutig.<br><b>Bei Neustart der Box beginnt der Schreibvorgang wieder bei eins!</b><br><a href="./RFID-Label-Writer-ESP32-DEV.zip">RFID-Label-Writer-ESP32-DEV.zip</a>