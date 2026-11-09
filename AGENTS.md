# Arbeitsanweisungen für Dominion

Dieses Projekt untersucht und erweitert die Steam-Version von **Dominion von Temple Gates Games**. Einstieg für neue Chats ist [Documentation/README.md](Documentation/README.md). Für den regelbasierten Kingdom-Generator zuerst [SetGenerator/README.md](SetGenerator/README.md) und den [Speicherbefund](Documentation/Kingdom_Sets.md) lesen.

## Pfade und Version

| Zweck | Pfad |
| --- | --- |
| Projekt | `D:\CDesktopLink\Unterlagen\Mods\Dominion` |
| Steam-Installation | `E:\ProgrammeE\Steam\steamapps\common\Dominion` |
| Profildaten | `%APPDATA%\TempleGates\Dominion\profiles.xml` |
| Lokale Partiedaten und Log | `%APPDATA%\TempleGates\Dominion\` |
| Steam-Manifest | `E:\ProgrammeE\Steam\steamapps\appmanifest_1131620.acf` |

Vor Spieltests die Spiel- und Steam-Buildnummer ermitteln. Vor einer statischen Analyse oder Wiederverwendung nativer Adressen auch den SHA-256 der EXE mit dem [Quellenregister](Documentation/Sources/Source_Register.md) vergleichen. Befunde nach Build und Prüfart kennzeichnen.

## Werkzeuge und Analyse

- Portables Python unter `D:\CDesktopLink\Portable\Python\WinPy64\python\python.exe` verwenden. Dort waren für die Erstprüfung `pefile` und Capstone verfügbar.
- Rizin, Cutter und Ghidra liegen unter `D:\CDesktopLink\Portable\Native` und in der Werkzeugsammlung `D:\CDesktopLink\Unterlagen\Mods\Stronghold Crusader DE\Meine Mods\.tools`. Werkzeughinweise und Beispiele für die prozesslokale Sleigh-Konfiguration stehen in Strongholds `AGENTS\native-analysis.md` und in `D:\CDesktopLink\Unterlagen\Mods\RON\AGENTS.md`. Die dortigen Wrapper und Projektdateien dienen als Referenz für einen Dominion-spezifischen Aufruf.
- Zuerst den konkreten Datei- oder UI-Einstieg und passende Referenzen in der Binärdatei prüfen. Native Patchentscheidungen brauchen einen Audit des betroffenen Pfads und seiner Aufrufer. Modul, Hash und RVA dokumentieren. Einen RVA klar von virtueller Adresse und Dateioffset unterscheiden.
- Neue Erkenntnisse in `Documentation\` sammeln. Für Spieltests Build, Modus, Vorbereitung und Ergebnis festhalten. Vorhandene Dokumentkodierung erhalten und neue Markdown-Dateien als UTF-8 ohne BOM mit CRLF schreiben.

## Lokale Tests

- Die Installation und die Profildatei zuerst als Quellen behandeln. Ressourcenanalyse und Patchversuche mit Arbeitskopien durchführen.
- Für einen gezielten Profiltest Dominion vollständig schließen, die Originaldatei sichern und nur den vorgesehenen XML-Eintrag ändern. Die erste Spielprüfung offline mit einer lokalen Partie durchführen. Nach dem Test Dominion schließen und das Original wiederherstellen.
- Profildateien, Sicherungen, Logs, Speicherabbilder und Spieldateien lokal außerhalb der Projektdateien halten. In die Dokumentation gehören nur benötigte Strukturen und anonymisierte Beispiele.
