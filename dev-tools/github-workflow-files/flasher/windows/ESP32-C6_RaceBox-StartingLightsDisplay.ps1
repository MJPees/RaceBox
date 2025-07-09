##########################################################################
# Sollte Windows die Ausführung des Skriptes verhindern,
# so muss temporär die Skriptausführungsrichtlinie geändert werden!
# Powershell als Administrator öffnen und mit den folgenden
# Befehlen die Berechtigung setzen und wieder entziehen:
# Set-ExecutionPolicy Unrestricted
# Set-ExecutionPolicy Restricted
##########################################################################
Write-Host "##############################################"
Write-Host "#                                            #"
Write-Host "#              Flasher ESP32                 #"
Write-Host "#                                            #"
Write-Host "##############################################"
Write-Host "#    Bitte im Geraetemanager den COM-Port    #"
Write-Host "# bestimmen und die COM-Port Nummer angeben. #"
Write-Host "##############################################`n"
$selectedPortNumber = Read-Host "COM-Port (Nummer) zum Flashen"

Write-Host "Ausgewaehlter COM-Port: COM$comPortToUse"
Write-Host "`nStarte Flashing-Vorgang..."

$command = "./esptool.exe --chip esp32c6 --port COM$selectedPortNumber --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode keep --flash_freq keep --flash_size keep 0x0 ../bin/StartingLightsDisplay.ino.bootloader.bin 0x8000 ../bin/StartingLightsDisplay.ino.partitions.bin 0xe000 ../bin/boot_app0.bin 0x10000 ../bin/StartingLightsDisplay.ino.bin"
Write-Host "`nAusgefuehrter Befehl: $command"

Invoke-Expression $command

Write-Host ""
Write-Host "###################################################"
Write-Host "#                                                 #"
WRITE-Host "# Druecke Return/Enter, um das Skript zu beenden. #"
Write-Host "#                                                 #"
Write-Host "###################################################"
Read-Host "Warte..."