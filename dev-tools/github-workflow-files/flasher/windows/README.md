Gerne, hier ist der optimierte Markdown-Inhalt für das Flashen unter Windows:

-----

# ESP32 flashen unter Windows

Bevor Sie Ihren ESP32 flashen, stellen Sie sicher, dass Sie alle notwendigen Vorbereitungen getroffen haben.

## Vorbereitung

  * **ESP32 Windows-Treiber installieren:** Laden Sie den Treiber für Ihren ESP32 gegebenenfalls herunter und installieren Sie ihn. Den passenden Treiber finden Sie oft auf der Webseite des Herstellers oder über diesen Link: [USB to UART Bridge VCP Drivers](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads).
  * **Flasher-Skript herunterladen:** Laden Sie die Zip-Datei aus dem `script-flasher`-Ordner herunter und entpacken Sie sie.
  * **Navigieren Sie zum Windows-Ordner:** Öffnen Sie den entpackten Ordner und wechseln Sie in den Unterordner `Windows`.
  * **Kommandozeile/PowerShell öffnen:** Halten Sie die **SHIFT-Taste** gedrückt, klicken Sie mit der **rechten Maustaste** in einen leeren Bereich des geöffneten `Windows`-Ordners und wählen Sie im Kontextmenü die Option "PowerShell hier öffnen" oder "Eingabeaufforderung hier öffnen".

## Flashvorgang

1.  **ESP32 verbinden:** Schließen Sie Ihren ESP32 über ein USB-Kabel an Ihren Computer an.
2.  **COM-Port ermitteln:** Öffnen Sie den **Geräte-Manager** (suchen Sie in der Windows-Suche nach "Geräte-Manager"). Suchen Sie dort unter "Anschlüsse (COM & LPT)" den COM-Port, der Ihrem ESP32 zugewiesen ist. Merken Sie sich die COM-Port-Nummer (z.B. COM3).
3.  **Flash-Skript starten:** Geben Sie in der zuvor geöffneten Kommandozeile oder PowerShell den **Namen des Flash-Skripts** ein und bestätigen Sie mit **Enter**.
4.  **COM-Port eingeben:** Das Skript wird Sie nach der Nummer des COM-Ports Ihres ESP32 fragen. Geben Sie die ermittelte Nummer ein und drücken Sie **Enter**.
5.  **Fortschritt beobachten:** Der Flashvorgang startet nun. Sie sehen Meldungen zum Fortschritt direkt in der Kommandozeile/PowerShell.
6.  **Abschluss:** Nach erfolgreichem Flashvorgang können Sie das Skript durch Drücken der **Enter-Taste** schließen.

-----

### Hinweis zur Skriptausführung unter Windows

Sollte Windows die Ausführung des Skripts blockieren, müssen Sie möglicherweise temporär die Skriptausführungsrichtlinie ändern.

**Gehen Sie dazu wie folgt vor:**

1.  Öffnen Sie die **PowerShell als Administrator** (Rechtsklick auf das Startmenü -\> "Windows PowerShell (Administrator)").
2.  Geben Sie den folgenden Befehl ein, um die Berechtigung zu setzen:
    ```powershell
    Set-ExecutionPolicy Unrestricted
    ```
3.  Bestätigen Sie die Änderung.
4.  Führen Sie den Flashvorgang wie oben beschrieben durch.
5.  **Wichtig:** Setzen Sie die Berechtigung nach dem Flashen unbedingt wieder zurück, um die Sicherheit Ihres Systems zu gewährleisten:
    ```powershell
    Set-ExecutionPolicy Restricted
    ```
6.  Bestätigen Sie die Änderung erneut.

-----