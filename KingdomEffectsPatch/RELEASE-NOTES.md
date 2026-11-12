# v0.1.0 – Vorabversion

Erste herunterladbare Windows-Version des Dominion Kingdom Effects Patch für **Spielbuild 3315 / Steam-Build 17683350**.

- Auch die F8-Übernahme erhält den privaten Spielmodus; ungültige Entwürfe melden konkrete Fehler und bleiben ohne Teilübernahme.
- „Leer“ erhält den privaten Spielmodus; der übrige native Lösch- und Speicherablauf bleibt bestehen.
- F8-Editor mit 89 Kampagneneffekten, Wirkung, Auslösezeitpunkt, interner ID und dokumentiertem Prüfstand.
- Vollständige Umschaltung Deutsch/English mit separat gespeicherter Sprachwahl; Kartenbilder und Erweiterungsregeln wechseln mit.
- Tatsächliche Mechanik- und Zielvoraussetzungen getrennt von nativen Erweiterungsfiltern und Funktionsprüfungen.
- Taifun mit „Stapel hinzufügen“, Doppelklick/Leertaste und nummerierten Zielen. Taifun und aktive Effekte per Drag-and-drop oder Alt+Pfeiltasten sortierbar, Entf zum Entfernen.
- Helle Checkboxen, Erweiterungsvorschau mit Originalregeln und von Anfang an markierbare Tabellenzellen ohne Formatwechsel beim Anklicken.
- Effektliste mit von Anfang an vollständigem pixelbasiertem Scrollbereich; zusätzlich 13 Kategorien nach „Wann/wie“, kombinierbar mit der Suche. Mehrere Auslöser pro Effekt bleiben berücksichtigt.
- Markierbarer, kopierbarer Tabellentext sowie Rechtsklick-Aktionen für eine Effektzeile oder alle gefilterten Effekte. Kopierte Namen und IDs enthalten keine unsichtbaren Layoutzeichen.
- F8 auch bei der Königreich-Auswahl für Online-Spiele und Privates Spiel. Getrennte Konfigurationen, sichtbarer Hinweis auf die Serverabhängigkeit und deaktivierte lokale Zufallsstapel-Automatik im Online-Editor. Die tatsächliche Online-Wirkung ist weiterhin nicht nachgewiesen.
- Stabile Kartenvorschau mit dauerhaft sichtbarem Text über dem Bild, gepufferter Bildausgabe und zusammengefassten identischen Ladeanfragen.
- Strengere Effektprüfung vor Übernahme und erweiterte Diagnose vom F8-Entwurf bis zum lokalen Spielaufbau; Bericht für alle 89 Effekte. Der im lokalen setupData-Startpfad nachgewiesene Effektverlust durch den nativen Erweiterungsfilter ist korrigiert.
- Höhere Kartenauswahl mit großer Vorschau, anfangs geschlossenen Erweiterungen und Hover ausschließlich über Kartennamen.
- Erweiterungsbezug aus dem Spielcode für alle 89 Effekte, sichtbar und suchbar in F8; 45 Effekte besitzen eine native Vorgabe. Diese ist vom Nachweis einer tatsächlichen Spielvoraussetzung getrennt.
- Karten- und Menügrafiken direkt aus der vorhandenen Dominion.xpack; keine mitgelieferten Spielbilder, keine vorherige Extraktion und kein Grafik-Cache auf der Festplatte. Asynchrone Vorschau mit gemeinsamem 96-MiB-Speicher-Cache.
- Korrekturen für die Kombination, Wiederholung und Speicherung von Effekten in Lokal spielen und Weitergeben, bis zur nativen Grenze von 32 Einträgen.
- Erweiterte Online-Bannlisten über das vorhandene Spielmenü; private Partien mit größeren Listen wurden beobachtet. Die deutsche und englische native Hilfe nennt nun die gemeinsame Grenze von 700 Karten und Querkarten.
- Schuldenkosten-Stempel mit korrigierter nativer Beschriftung; Triumph mit 0/1/2/3 Kopien und 5/3/1/0 Schuldenkosten im Produktions-Patch geprüft.
- Gültige doppelte Start-Projektsteine freigegeben: höchstens zwei insgesamt, bei gleichem Projekt nur mit passender Freigabe für den zweiten Stein. Dritter Startstein bleibt abgesichert.
- Doppelklick-Starter mit fertigen Dateien, Steam-Pfaderkennung und direktem Start der Steam-Installation mit dem normalen Spielprofil. Keine Spielkopie, kein Testprofil. Höchstens drei Profilsicherungen.

**Bekannte Grenzen:** Ungetestete und teilweise geprüfte Effekte sind entsprechend markiert und auswählbar. Bekannte Startabbrüche bleiben abgesichert. Kampagneneffekte funktionieren nicht in privaten Online-Partien. Der native Kampagnenhinweis kann bei vielen Effekten überlaufen und lässt sich ausblenden. Matchmaking und die volle Kapazität von 700 Banns sind ungeprüft. Vereinzelt fehlt eine Kartenillustration. Die sichtbare Prüfung bei 125 % und 150 % Windows-Skalierung steht noch aus.

Installation und Bedienung stehen in der beiliegenden README.md. Das ZIP enthält keine Spielgrafiken, keine Dominion-EXE, kein Xpack und keine Profildaten. Dominion muss separat über Steam installiert sein.
