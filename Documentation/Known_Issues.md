# Bekannte Probleme und Grenzen des Kingdom Effects Patch

Prüfstand: 13. November 2026, Dominion 3315 / Steam-Build 17683350, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`.

| Befund | Aktueller Umgang und Grenze |
| --- | --- |
| „Leer“ verändert Privatstatus | Korrigiert; 216 native Vergleiche und Neustarts bestanden. Privates Spiel → Leer vom Nutzer am 13. November bestätigt. Die kombinierte Folge mit F8 und anschließendes Einladen bleiben getrennt offen. |
| F8-Suche war seit der markierbaren Tabelle langsam | Korrigiert: Textfelder, Suchtexte und Zeilenhöhen werden wiederverwendet. Finaler isolierter Test mit 108 warmen Aktualisierungen unter 100 ms; [Messung und Grenzen](F8_Editor_Usability.md#suche-und-filter-wiederverwendung-und-vergleichsmessung). |
| F8-Übernahme verändert Privatstatus | Korrigiert; produktiver Übernahmepfad mit 468 nativen Fällen und Neustarts geprüft. |
| F8 lehnt ungültigen Entwurf ohne konkreten Validierungstext ab | Korrigiert: konkreter Startschutz beziehungsweise lesbare Validierungsmeldung; vorherige Konfiguration bleibt vollständig erhalten. |
| Schuldenkosten-Stempel brach mit Triumph den Start ab | Korrigiert: zwei gezielte Text-Lookups verwenden den vorhandenen lokalisierten Anzeigenamen. Die Effekt-ID bleibt unverändert. Vollständiger Produktions-Patch erreicht mit 0/1/2/3 Kopien 5/3/1/0 Schuldenkosten. Ohne installierte Textkorrektur bleibt der Schutz aktiv. |
| Wiederholter Projektstein wurde zu pauschal gesperrt | Gültige Doppelbelegung ist freigegeben: höchstens 2 Startsteine insgesamt; gleiches Projekt nur mit passender `start_allow_second_cube`-Freigabe. Festzug 0/1 ohne sowie 0/1/2 mit Freigabe im Produktions-Patch bestätigt. Drei Startsteine bleiben wegen eines nachgewiesenen nativen Abbruchs gesperrt. |
| Kampagneneffekte fehlen im privaten Online-Match | Bisheriger Serverbefund unverändert. Der Client-Editor kann Effekte speichern, aber keine serverseitige Verarbeitung nachrüsten. |
| Nativer Bann-Hilfetext nannte weiterhin fünf Karten | Deutsch und Englisch korrigiert: vier native Einträge, Stil und Referenzverwaltung erhalten; native Kontrollen bestanden. Die tatsächliche Aktivierung durch den Produktions-Timer ist in sechs Logs bestätigt. |
| Nativer Dialog „Kampagnen-Features“ läuft bei vielen Effekten über | Bekannter Vanilla-Anzeigefehler, bei elf Effekten beobachtet. Schließen oder „Nicht wieder anzeigen“ ist möglich. Kein freigegebener nativer Layout-Fix vorhanden; [Untersuchung und Prüfgrenze](Campaign_Dialog_Investigation.md). |
| Fehlende Checkboxen in der Taifun-Auswahl | Der gemeldete Screenshot zeigt Taifun; dieser Modus erzeugte absichtlich keine Checkboxen. Er belegt keinen Defekt des neuen Renderers. Taifun verwendet jetzt Checkboxen und eine mit der nummerierten Liste synchronisierte Auswahl; automatisierte Prüfungen bestanden, Darstellung vom Nutzer bestätigt. [Detailbefund](Checkbox_Visibility_Fix.md) |
| Effektziele aus nicht besessenen Erweiterungen über Taifun erreichbar | Vom Nutzer gemeldete Zugriffslücke; v0.1.1-Testpaket prüft jetzt offizielle Kartenfreigaben in Auswahl, Import, Übernahme und Start. Alle 89 Effekte bleiben verfügbar. Automatisiert geprüft, sichtbare Nutzerabnahme offen. [Audit und Tests](Target_Card_Access.md) |
| Kartenfreigabe noch nicht vorbereitet | Der Patch initialisiert den nativen Cache nicht selbst, da der native Getter bei kaltem Cache auch Konfigurationen verändert. Unbekannte Freigabe verhindert betroffene Ziele; normale Sammlung/Königreich-Auswahl erneut öffnen und F8 wiederholen. |
| Einzelne Kartenillustrationen fehlen | Text und Auswahl bleiben verfügbar; eine fertig zusammengesetzte Vorschau ist kein Nachweis dafür, dass jede Illustration vorhanden ist. Keine fremden Bilder werden mitgeliefert. |

Offene Einzel-/Mehrfachprüfungen von Effekten sind keine nachgewiesenen Fehler und bleiben entsprechend markiert. Auch echte Windows-Skalierung bei 125/150 Prozent, Matchmaking und die volle Bannkapazität sind ausstehende Abnahmen, keine bestätigten Defekte. Die automatisierten Tests übernehmen keine physische Maus-/Tastatureingabe.

Die Bestandsprüfung hat neben F8-Privatstatus und Validierung auch den Schuldenstempel-Textfehler, die zu pauschale Projektstein-Sperre und die Bannhilfe korrigiert. Die verbleibenden Punkte sind ausdrücklich nicht als erledigt gekennzeichnet. [Native Effekt- und Textkorrekturen](Native_Effect_Fixes.md), [Leer und F8](Private_Game_Clear.md)
