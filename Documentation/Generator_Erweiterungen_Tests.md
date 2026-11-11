# Abnahme des Dominion-Generators mit Erweiterungen

Spielbuild 3315, Steam-Build 17683350. Bei jedem Steam-Test Eingabe, Modus, Soll, beobachtetes Ergebnis und Build ergänzen. Zuerst lokal und offline testen. Profiltests folgen dem Sicherungsablauf im Projektleitfaden.

| Status | Steam-Import | Erwartung |
| --- | --- | --- |
| Offen | Zehn Hauptstapel aus Basisspiel und Intrige | Dieselben zehn Stapel im Vorrat. |
| Offen | Gemischtes Königreich und Verdopplungs-XML | Nach dem Wechselablauf sind die zehn Zielstapel verdoppelt. |
| Offen | young_witch(village) | Dorf ist die Bannkarte. Kampagnenvorlage vorhanden. |
| Offen | ferryman(shop) | Shop steht neben dem Vorrat. Kampagnenvorlage vorhanden. |
| Offen | riverboat(imperial_envoy) | Imperial Envoy liegt für Flussboot beiseite. Kampagnenvorlage vorhanden. |
| Offen | obelisk(patrician) | Der Stapel mit Patrizier ist das Ziel. Kampagnenvorlage vorhanden. |
| Offen | Merkmal und Weg der Maus mit Ziel | Steam übernimmt das gewählte Ziel. |
| Offen | Druide | Die drei gezogenen Gaben liegen beiseite. |
| Offen | Kontakt und Verbündeter | Steam übernimmt den gewählten Verbündeten. |
| Offen | Omen, Prophezeiung und zusätzlicher Angriffsstapel | Steam übernimmt die beiden Zuordnungen. |
| Offen | Landschaften, Platin/Kolonien und Unterschlüpfe | Die gewählten Optionen stimmen beim Spielstart. |
| Offen | Vollständiger Code und Hauptstapel-Teilcode | Teilcode benennt offene Zuordnungen. Vollständiger Code enthält das bestätigte Setup. |

## Generator

| Status | Fall |
| --- | --- |
| Technisch geprüft | 498 Hauptstapel und 853 kanonische Definitionen, beide Titelsprachen, der Alias Farm/Harem und die Verknüpfungen zu Sonderkarten. |
| Technisch geprüft | Sechs Basisspiel- und vier Intrige-Stapel mit einem festgehaltenen und einem ausgeschlossenen Stapel. Gesperrte Erweiterung mit festgehaltener Karte meldet den Konflikt. |
| Technisch geprüft | Bei fünf Zufallssets mit je 1–3 Stapeln und zwei Lostopfplätzen gelten die Quoten nur für die zwei ausgelosten Sets. Höhere Mindestquoten werden pro Kombination geprüft. Unlösbare Kandidaten werden übersprungen und vollständig unlösbare Lostöpfe melden einen Konflikt. |
| Technisch geprüft | Festgehaltene Karten und Landschaften mit Herkunft Auto belegen einen Lostopfplatz ihrer Erweiterung. Drei erforderliche Sets bei zwei Plätzen melden die festgehaltenen Karten als Ursache. Explizite Landschaftsherkünfte bleiben unabhängig. Ohne Extras gelten die Quoten der Zufallssets nicht. |
| Technisch geprüft | Unlösbares Zusatzsetup, Bannkarte, Fährmann, Flussboot, Druide, Obelisk, Merkmal, Verbündeter und zusätzlicher Angriffsstapel durch eine Prophezeiung. |
| Technisch geprüft | Reine Münzkosten ohne Trank- oder Schuldenkarten. Landschaftsquoten für Typ, Herkunft und Quelle für Kostbarkeiten. |
| Offen | Aufbauhilfe, Material und angewendete Sonderregeln in einer vollständigen Partie abgleichen. |
| Technisch geprüft | Exakte Anzahl gegen Kombinatorik und vollständige Suche im kleinen Pool. Laufzeitgrenze des Solvers mit vollem Katalog und Abbruch eines ersetzten Auftrags. |
| Offen | Fünf-Sekunden-Grenze, rasche Eingaben und Anzeige im echten Browser bei großen Pools prüfen. |
| Technisch geprüft | Einstellungen aus Version 1, Ausschlüsse, Reset, zweisprachige Suche, gekennzeichneter Teilcode und Verdopplungs-XML zur Ziehung. |
| Technisch geprüft | Kartenvorschauen mit Bilddateien und lesbare Textansicht bei fehlenden Bildern in Firefox und Edge. |
| Technisch geprüft | 897 Bilddateien lassen sich dekodieren. Alle 498 Hauptstapel haben eine Illustration. Die 26 Basisspielkarten haben vollständige Vorderseiten auf Deutsch und Englisch. Platin, Trank und Fluch sind über die gemeinsam gespeicherten Ressourcen ergänzt. |

## Browser-Abnahme der Release-Überarbeitung

| Status | Fall |
| --- | --- |
| Technisch geprüft | Letzte Ziehung und Export bleiben beim Festhalten im Kartenpool und bei einem Quotenkonflikt erhalten. Festhalten im Ergebnis und im Pool bleibt synchron. Die zehn Stapel stimmen im Kingdom-Code, Verdopplungscode und XML überein. |
| Technisch geprüft | Eine Quotenänderung lässt manuell geschlossene Suchgruppen geschlossen. Ausgeblendete Treffer schließen ihre festgehaltene Vorschau. Die Eigenschaften-Auswahl klappt mit Esc zu und gibt den Fokus an den Button zurück. |
| Technisch geprüft | Einstellungen aus Version 1 laden, ungültige Grenzen mit Filternamen melden, den letzten gültigen Wert während der Eingabe speichern und die Einstellungen zurücksetzen. XML-Anlage und Schreibablauf mit einem Testprofil geprüft. |
| Technisch geprüft | Hell und Dunkel, deutsche und englische Editionsmarkierung, ein- und dreispaltige Eigenschaften-Auswahl, Vorschau bei Bedarf, Illustration aus dem lokalen Bildpaket und Text bei einem defekten Bild. |
| Technisch geprüft | Eingebetteter Worker und Solver-Quelle stimmen überein. Der Pages-Workflow prüft, dass `index.html` bytegenau der Generator-Datei entspricht. |
| Technisch geprüft | Der Spielaufbau nutzt unter Karten und Export die volle Breite. Kurze Gruppen bilden ein responsives Raster. Listen ab fünf Karten verteilen sich in einer vollen Zeile auf mehrere Spalten. |
| Technisch geprüft | Platin und Kolonie öffnen oben ihre Kartenvorschau. Basis-Geld-, Punkte- und Fluchkarten zeigen ihre festen Werte. Unterschlüpfe und Kostbarkeiten haben kurze Hinweise mit ihrer Herkunftserweiterung. |

Die automatisierte Browser-Prüfung lief über einen lokalen Testserver mit temporären Profilen in Edge (390 und 1440 Pixel) und Firefox (500 und 1440 Pixel). Das lokale Bildpaket enthält 842 Illustrationen und je 26 fertige Vorderseiten auf Deutsch und Englisch. Firefox und Edge haben die automatische Bildladung am lokalen Server sowie Ziehung, Sprachwechsel und Einstellungen in der Ausgabe für GitHub Pages geprüft.

Die unveränderte Pages-Ausgabe wurde in Firefox und Edge unter dem lokalen Pfad `/Dominion/` geprüft. Beide Browser zeigten deutsche und englische Karten- und Erweiterungsregeln, die Kartenrahmen und 43 unterschiedliche Spielsymbole. Die Ziehung ergab zehn Stapel. Der Kopiertest verglich den übergebenen Kingdom-Code über eine simulierte Zwischenablage und prüfte die manuelle Textauswahl bei verweigertem Zugriff. Der Link im Generator öffnete den mitgelieferten Konverter.

Die bedingten Quoten der Zufallssets wurden am 11. November 2026 in Firefox und Edge auf Deutsch und Englisch geprüft. Einstellungsdateien der Version 2, gespeicherte Werte nach dem Neuladen, wiederholte Ziehungen und die letzte sichtbare Ziehung bei einem Konflikt wurden dabei mitgeprüft.

Die einzelnen Bilddateien wurden über den Pages-Pfad `/Dominion/` und beim direkten Öffnen der HTML-Datei geprüft. Beim Start lädt die Seite keine Kartenbilder. Die erste Vorschau lädt genau ihr Bild. Der Sprachwechsel lädt die passende Vorderseite. Bei einer fehlenden Vorderseite werden Illustration und Regeltext verwendet. Fehlen beide Bilder, bleibt der Text sichtbar. Ziehung und Export funktionieren dabei weiter. Die erneute Erzeugung der Bilddateien und Bildpfade ergab dieselben Inhalte.

Die erweiterte Aufbauhilfe und die neuen Hinweise wurden in Firefox und Edge bei 390, 900, 1920 und 2560 Pixeln auf Deutsch und Englisch sowie in Hell und Dunkel geprüft. Größenänderungen und Sprachwechsel erhalten dieselben Hauptstapel und Aufbauzuordnungen. Fehlende Bilder lassen die Kartenwerte lesbar. In Firefox wurden zusätzlich native Tab-, Enter- und Esc-Eingaben sowie Hover, Anklicken und Festhalten geprüft. Zwei dabei gefundene ID-Kollisionen bei der Kartenauswahl für Suchen und bei der Überschrift der Effektfilter sind behoben.

Der reproduzierbare Solver-Test läuft aus dem Projektordner mit `node SetGenerator/test_generator_solver.js`.

Die Bilddateien aus der installierten Dominion.xpack umfassen 845 Illustrationen und 52 vollständige Basisspiel-Vorderseiten mit insgesamt 18.992.237 Bytes. Die übrigen acht Definitionen sind Sammelbegriffe oder interne Einträge und verwenden die Textvorschau. Der Generator verwaltet die Dateizuordnungen im HTML-Block `card-image-files`.
