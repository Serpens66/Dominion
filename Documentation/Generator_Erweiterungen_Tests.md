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

## Zusatzkarten-Modus und Verbunden mit

Stand 11. November 2026. Die folgenden technischen Prüfungen beziehen sich auf den neu ergänzten Modus; frühere Browser-Abnahmen oben ersetzen seine erneute visuelle Abnahme nicht.

| Status | Fall und Ergebnis |
| --- | --- |
| Technisch geprüft | Modus aus: Kleinbauer bleibt bei Angriff 0–0 zulässig. Modus an: Soldat aktiviert den Angriffsfilter und bedingte Abwehr-/Angriffsreaktionsquoten. |
| Technisch geprüft | Flüchtling macht die Kleinbauer-Reihe zu genau einer Kartenzug-Quelle. Mindestwert 1 gelingt, Mindestwert 2 bei dieser einzigen Quelle nicht. |
| Technisch geprüft | Reisenden-Reihen, Champion, Schild, Artefakte, Erbstücke, Gaben, Kostbarkeiten und die Vampir/Fledermaus-Rückverbindung. Verweise zeigen auf gültige IDs; transitive Familien sind eindeutig und enthalten ihre eigene Quelle nicht erneut. |
| Technisch geprüft | Feste Landschaftszusätze: ein Pferd aus Demand erfüllt Kartenzug 1–1 und verletzt 0–0. Kostbarkeiten aus Foray aktivieren über Schwert die Abwehrquote und enthalten Schild. |
| Technisch geprüft | Zufällige Maus-Ziele erfüllen und verletzen keine Eigenschaftsquote. Das Ergebnis enthält den Hinweis auf unberücksichtigte Zufallszuordnungen. |
| Technisch geprüft | Reine Münzkosten und Erweiterungsquoten bleiben auf Hauptstapel bezogen. Ohne Aktionstyp trifft bei einer Familie mit Aktionskarte nicht zu. |
| Technisch geprüft | Kleine Hauptstapel-Pools vollständig durchgezählt und mit den erweiterten Solver-Quoten verglichen. Bestehende Suite, Zeitlimit, Inline-JavaScript-Syntax und eingebetteter Worker-Abgleich laufen erfolgreich. |
| Technisch geprüft | Einstellungsvalidierung akzeptiert den neuen booleschen Modus und passende erweiterte Grenzen. Alte Dateien und Reset setzen ihn aus. Die gemeinsame Tooltip-Steuerung erhält die Elternvorschau beim Öffnen und Schließen eines Kindes; das Schließen des Elternteils schließt beide. |
| Offen | Firefox und Edge: neue Checkbox, Export/Import alter und neuer Einstellungen, Speichern nach Neuladen, Gesamtreset und Ziehfilter-Reset prüfen. Filteränderungen müssen die letzte Ziehung und ihren Export erhalten. |
| Offen | Deutsch/Englisch, Maus/Tab/Enter/Esc, schmale/breite Ansicht: Verbunden mit öffnen, Reisende und Artefakte ansehen, verschachtelte Vorschau festhalten und per Außenklick schließen. Lange Zusatzstapel, Bildfehler und Fensterrand prüfen. |
| Offen | Steam: Maus-Ziel mit und ohne nativen Kartenbann vergleichen. Jeweils Spielbuild, Steam-Build, Modus, Banneinstellung, Kingdom-Eingabe, Soll und gewählte Zusatzkarte notieren. |
| Offen | Steam: gezielter Import einer Maus-Karte. Kampagnenvorlagen getrennt behandeln; erst nach bestätigtem Import eine festgelegte Zufallszuordnung für Filtergarantien verwenden. |

Die Browseroberfläche konnte in diesem Durchlauf die lokale Testseite nicht erreichen; direkte file-URLs werden vom Browserwerkzeug gesperrt. Deshalb sind die neuen Browserfälle ausdrücklich offen. Die Steam-Installation und profiles.xml wurden nicht verändert.

## Bannlisten-Export für profiles.xml

Stand 11. November 2026, native Zuordnung für Spielbuild 3315 / Steam-Build 17683350. Die bestehende Bestätigung des Nutzers zu 25 Online-Banns bleibt ein separater Spieltest.

| Status | Fall und Ergebnis |
| --- | --- |
| Technisch geprüft | EXE-Hash, 853 eindeutige kanonische Zahlen-IDs und vollständige Abdeckung der 498 Hauptstapel und 155 auswählbaren Landschaften. Eine fremde EXE wird abgelehnt. |
| Technisch geprüft | Keller 512, Dorf 518, Farm/Harem 792, Ritter 2332, Sauna 4869, Weg der Maus 3898 und Brandschatzung 4412 abgeglichen. |
| Technisch geprüft | Gemeinsame Komponente an zwei Stellen im DOM-Modell: Ausschlüsse, importierte Auswahlwerte, Reset und Sprachwechsel liefern dieselben numerisch sortierten Zeilen. Festgehaltene Karten bleiben aus der Ausgabe. Keine Ziehung oder aktive Herkunft erforderlich. |
| Technisch geprüft | Leere Auswahl deaktiviert Kopieren. Doppelte Karten erscheinen einmal. Fehlende und widersprüchliche ausgewählte Zahlen sperren beide Ausgaben. Die Kapazitätsprüfung meldet mehr als 700 Einträge. Alle 653 auswählbaren Einträge gemeinsam exportiert. |
| Technisch geprüft | Echte Kopierfunktion mit simulierter Zwischenablage: Erfolgsmeldung und Textauswahl bei verweigertem Zugriff. |
| Technisch geprüft | Erzeugte Zeilen in synthetische lokale und Online-Profilblöcke eingesetzt und mit XML-Parser geprüft. Vorhandener Beispieleintrag und andere Profilfelder bleiben erhalten. |
| Offen | Firefox und Edge: beide Stellen bei 390 und 1440 Pixeln, in Hell/Dunkel und Deutsch/Englisch prüfen. Kopieren, manuellen Fallback, Einstellungsimport, Neuladen und Ausschluss-Reset im Browser bestätigen. |
| Offen | Neue Generator-Ausgabe im gesicherten Profil ergänzen und in einer lokalen Partie prüfen. Spielbuild, Steam-Build, Eingabe, Modus, Soll und tatsächlich gezogene Karten festhalten. |
| Offen | Neue Generator-Ausgabe für Privates Spiel prüfen. Erweiterte Landschaftsbanns und Matchmaking bleiben getrennte offene Fälle. |

Reproduzierbar mit `node SetGenerator/test_generator_solver.js` und `python SetGenerator/test_generator_catalog.py`. Der Python-Test nutzt für den XML-Test nur synthetische Daten; der zusätzliche native Abgleich wird auf Rechnern ohne lokale Build-3315-EXE übersprungen. Für die neue Ausgabe erfolgte in diesem Durchlauf keine visuelle Browser- oder Steam-Abnahme.

## Konkrete Konflikte mit festgehaltenen Karten

Stand 13. November 2026. Eine Vorprüfung verwendet dieselben Filterprädikate wie die Ziehung und prüft die festgehaltenen Hauptstapel sowie, im Zusatzkarten-Modus, feste Landschaftszusätze gegen die Obergrenzen. Sie nennt die passenden Karten, den Filter, die erlaubte Höchstzahl und den bereits erforderlichen Wert. Es werden keine Einstellungen geändert. Bedingte Schutz-/Angriffsreaktionsquoten werden erst bei einem Angriff aus den festgehaltenen Quellen geprüft.

| Status | Fall und Ergebnis |
| --- | --- |
| Technisch geprüft | Nutzerfall: Forscherin ist festgehalten, Karten entsorgen steht auf 0–0. Die Meldung nennt Forscherin, Entsorgen, das Maximum 0 und den nötigen Bis-Wert 1. |
| Technisch geprüft | Reduzierte anonyme Reproduktion mit drei Immer-Sets, zwei Zufallssets, Renaissance als erforderlichem Zufallsset und 2–4 Landschaften: 0–0 wird vor dem Worker mit Ursache abgelehnt, ausschließlich die Änderung auf 0–1 liefert eine Ziehung mit Forscherin. |
| Technisch geprüft | Mehrere festgehaltene Entsorger werden mit Namen und gemeinsamer Anzahl genannt. Kosten- und Erweiterungsobergrenzen verwenden dieselbe Vorprüfung. |
| Technisch geprüft | Zusatzkarten-Modus: Kleinbauer zählt über Soldat als Angriff. Feste Kostbarkeiten aus Brandschatzung zählen als Landschaftsquelle; Kosten und Herkunft der Landschaft erhöhen keine Hauptstapelquote. |
| Technisch geprüft | Burggraben beziehungsweise Bettler allein lösen bei bedingtem Maximum 0 keinen Konflikt aus. Mit festgehaltener Hexe gilt die jeweilige Quote. Der Weg der Maus aktiviert durch sein zufälliges Ziel keine solche Vorprüfung. |
| Technisch geprüft | Deutsche und englische Meldungen, unveränderte Eingabewerte nach Ablehnung, bestehende Solver-Suite und Gleichheit von eingebettetem Worker und Solver-Quelle. |
| Offen | Firefox/Edge: konkreten Hinweis durch Festhalten und Loslassen auslösen, längere Namenslisten umbrechen lassen und die letzte Ziehung samt Export während des Konflikts sichtbar halten. |

Die allgemeine Unlösbarkeitsmeldung bleibt für gemeinsam unvereinbare Regeln erhalten, wenn keine einzelne Ursache nachgewiesen wurde. Ein einzelner verworfener Suchzweig wird nicht als globaler Konflikt ausgegeben. Die Prüfungen liefen mit `node SetGenerator/test_generator_solver.js`; die Einstellungsdatei des Nutzers und die Spieldateien wurden nicht verändert.
