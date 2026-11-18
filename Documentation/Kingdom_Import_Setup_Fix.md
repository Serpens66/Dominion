# Kingdom-Import: ausdrückliche Aufbauoptionen

Stand: 18. November 2026. Dominion 3315 / Steam-Build 17683350. EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Alle nativen Adressen sind RVAs in Dominion.exe.

## Ziel und Abgrenzung

Ein vollständig vorgegebenes Königreich soll beim Einfügen über Kartensuche oder Zwischenablage seine ausdrücklich angegebenen Aufbauoptionen behalten. Unterstützt werden `colonies` und `shelters` im Kartenabschnitt sowie `Colonies`, `Shelters`, `NoColonies`, `NoShelters` und `Landscapes(N,N)` im Modifikatorabschnitt. Nicht angegebene Optionen bleiben wie zuvor eingestellt. Ein einzelner Landschaftswert folgt der nativen Schreibweise als gleiche Unter-/Obergrenze; die sichere Kapazität beträgt vier Landschaften.

Die zunächst untersuchte automatische Erweiterungsaktivierung wird nach der Präzisierung des Nutzerziels nicht umgesetzt. Konkrete freigegebene Karten können bereits nativ aus ausgeschalteten Zufallsquellen übernommen werden. Ein gültiger `-e`-Abschnitt darf im gemeinsamen Code stehen; die Korrektur ersetzt dessen native Zufallspoolbehandlung nicht und setzt keine zusätzlichen Zufallsplätze oder Quoten. `-e` ist insbesondere kein Ersatz für einen ausdrücklichen Kolonien-/Unterschlupfschalter.

Der Nutzer-Kontrolltest verwendet ausschließlich zehn Basisspiel-Hauptstapel und fordert trotzdem Platin/Kolonien:

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, colonies -e base, prosperity -m Landscapes(0,0), NoShelters
```

Laut Nutzerbericht fehlten damit in Vanilla Platin/Kolonien. Die Prüfung geladener Module während dieser konkreten Partie war nicht erfolgt. Der automatisierte Vergleich muss deshalb den ungepatchten und gepatchten nativen Import getrennt prüfen.

## Audit und Eingriff

Der vorhandene Suchfeldpfad parst bei `0x1F6F17` und ruft anschließend bei `0x1F6F2F` die Zusammenführung `0x1F6A90` auf. Der Zwischenablagepfad parst bei `0x22BF93` und führt bei `0x1FE032` dieselbe Zusammenführung aus. Beide vorhandenen Merge-Hooks werden genutzt. Drei eng begrenzte zusätzliche Stellen sichern die ausdrücklichen Aus-Schalter und die Serialisierung der Null-Landschaftsquote ab; die Originalbytes werden vor der transaktionalen Installation geprüft.

Die native Zusammenführung kopiert Karten, Landschaften, Zuordnungen und Effekte, aber nicht die betreffenden Aufbauoptionen. Der Parser schreibt Platin/Kolonien bei Setup `+0xBFC` und Unterschlüpfe bei `+0xC00`: `0` bedeutet nicht ausdrücklich vorgegeben, `1` ein, `2` aus. Die Landschaftsquote liegt bei `+0xBC8`/`+0xBCC`, mit nativer Vorgabe `0..2` auch bei fehlender Angabe. Deshalb genügt ein Vergleich gegen Null nicht zur Erkennung ausdrücklich angegebener Landschaftsquoten.

Die Korrektur erkennt nur das Vorkommen der dokumentierten Optionen auf oberster Ebene des Eingabetexts. Ihre Werte werden anschließend aus dem nativen Parserergebnis übernommen; eine zweite Interpretation von Vorrang oder Zufallsregeln findet nicht statt. Verschachtelte Karten-/Effektargumente und andere Optionsabschnitte lösen keine falsche Übernahme aus. Die Präsenzdaten werden für den unmittelbar folgenden Merge verwendet und danach verworfen.

Vor der nativen Zusammenführung werden alle Prüfungen auf einer Kopie abgeschlossen. Erst danach werden die ausdrücklich angegebenen Optionsfelder in das aktuelle temporäre Setup kopiert. Das erfolgt bereits im vorhandenen `prepare_merge`: Der native Kartenhelfer kann während des Zusammenführens selbst serialisieren und speichern, noch vor dem vorhandenen Effekt-Commit-Hook bei `0x1F6C74`. Ein erst dort gesetzter Schalter käme für diese Zwischenstände zu spät. Auch Importe ohne Effekte durchlaufen daher die Vorbereitung.

Die vorhandene Effektzusammenführung, ihre 32-Einträge-Grenze, die native Karten-/Zuordnungslogik und die Online-/Privatspielbehandlung bleiben bestehen. Die Optionen werden im nativen Kingdom-Text gespeichert; es gibt keine nachträgliche Änderung einer laufenden Partie und keine Änderung der Serververarbeitung.

## Kartenfreigabe

Vor jeder Änderung werden alle konkret importierten Karten über den bestehenden offiziellen Freigabe-Snapshot geprüft: Hauptstapel, Landschaften, positive konkrete Landschaftsziele und typisierte Zusatzkarten-Zuordnungen. Negative numerische Zuordnungskürzel sind keine Karten-IDs. Effektziele bleiben zusätzlich durch den vorhandenen Effektvalidator geschützt.

`colonies` im Ein-Zustand erfordert freigegebenes Platin und Kolonie; eingeschaltete Unterschlüpfe erfordern die drei zugehörigen Karten. Explizites Ausschalten setzt keine Erweiterungsfreigabe voraus. Nicht freigegebene oder nicht zuverlässig prüfbare Inhalte führen vor dem Merge zu einer verständlichen Ablehnung; die vorhandene Auswahl wird nicht still gekürzt. Dies gilt auch für `summon`/Einladung: Fehlende manuelle Freigabe wird nicht durch den Patch ersetzt. Freigabecache, Kaufdaten und normale Profile werden für die Tests nicht geändert.

## Prüfstand

Die native Parserinspektion in einem isolierten Client bestätigt die beschriebenen Felder und die vollständige syntaktische Erfassung des ursprünglichen Zehnkarten-/Vierlandschaften-Beispiels. Beispielsweise landen Bannstapel und Druiden-Gaben in typisierten Zusatzfeldern; die Landschaftsziele bleiben separat. Das ist von Übernahme, Speicherung und tatsächlichem Spielaufbau zu unterscheiden.

Die gebündelte Prüfung läuft über die vorhandenen `effects_test.exe`- und `target_access_native_test.exe`-Prüfroutinen. Zusätzliche eigenständige „setup“-Testprogramme werden nicht benötigt. Ihre Startprobleme wurden nicht als Patchfehler oder notwendige Installation eingeordnet; Windows-/Sound-/Sicherheitseinstellungen bleiben unverändert.

### Native Folgefehler und gezielte Korrekturen

Die native Normalisierung löscht bei fehlender allgemeiner Erweiterungsfreigabe nicht nur Ein-, sondern auch Aus-Zustände: Bei `0x0DBFFC` wird das lokale Kolonienfeld, bei `0x0DC026` das lokale Unterschlupffeld auf 0 gesetzt. Die Hooks erhalten ausschließlich den ausdrücklichen Aus-Wert 2. Jeder andere Wert durchläuft weiterhin exakt die native Rücksetzung; Kartenfreigaben und positive Ein-Schalter werden dadurch nicht erweitert.

| RVA | Originalbytes | Eingriff |
| --- | --- | --- |
| `0x0DBFFC` | `C7 85 5C F2 FF FF 00 00 00 00` | `NoColonies` erhalten; Rückkehr bei `0x0DC006`. |
| `0x0DC026` | `C7 85 60 F2 FF FF 00 00 00 00` | `NoShelters` erhalten; Rückkehr bei `0x0DC030`. |
| `0x0DFA4F` | `85 C9 75 09 39 4B 04` | Nullquote nur für die beiden Landschafts-Ausgabestellen bei Rücksprung `0x0DFE80`/`0x0E2224` ausdrücklich ausgeben. |

Der gemeinsame Zahlenpaar-Formatter `0x0DFA10` lässt gewöhnlich `0,0` weg. Für Landschaften würde das beim erneuten Lesen die Vorgabe `0..2` herstellen. Der dritte Hook umgeht diese Kürzung deshalb ausschließlich für die Landschaftsausgabe der Spiel-/Setup-Serializer. Alle anderen Quoten und Aufrufer behalten die native Behandlung. Die Originalausgabe ab `0x0DFA5C` schreibt weiterhin den Text; es wird keine eigene Stringverwaltung eingeführt. Die Hooks gelten für normale Nutzung und Online-Diagnose, nicht für den Messmodus.

### Bestandene Import- und Startprüfungen

Am 18. November besteht der direkte Vergleich im isolierten, zunächst ungepatchten Spielprozess: **82 Vanilla-Vergleichsfälle** und anschließend **86 Fälle mit Patch**, jeweils ohne Abweichung von der getrennt festgelegten Erwartung. Geprüft sind Suche und Zwischenablage-Einstieg, lokale und beide Online-Konfigurationen, Ein-/Aus-Schalter, ausgelassene Optionen, Quoten `0,0` und `4,4`, Effektübernahme, Privatstatus sowie unveränderte Erweiterungsfilter. Der Vanilla-Vergleich zeigt die ausgelassenen importierten Werte; er wird nicht als erfolgreicher Vanilla-Import beschrieben.

Auch mit ausgeschaltetem Zufallspool werden bereits nativ freigegebene feste Erweiterungskarten übernommen. Dafür wird keine Aktivierungslogik ergänzt. Verfügbare Druiden-/Weg-der-Maus-Zuordnungen behalten ihre Karten-/Zusatzdaten. Nicht freigegebene Zuordnungen werden nicht durch künstliche Kontofreischaltungen für Tests ermöglicht und sind im Bericht ausdrücklich übersprungen. Der ursprüngliche vollständige Vierlandschaften-Code wird nur in einer kopierten Struktur mit Testanbieter semantisch geprüft; daraus folgt kein Freigabenachweis für Einladung.

Nach dem tatsächlichen gepatchten Import wird das Zehn-Basiskarten-Königreich gestartet: **Platin und Kolonien sind im Vorrat vorhanden**, die Anfangsdecks enthalten Anwesen und keine Unterschlüpfe. Die Landschaftsquote `0,0` wird im gespeicherten Setup erhalten. Positive Unterschlüpfe wurden im isolierten Client mangels entsprechender Freigabe nicht als reale Partie erzwungen; ihre Parser-/Übernahmewerte werden in den gemeinsamen Unit-Prüfungen abgedeckt.

Die Prüfhilfe bindet für direkte Aufrufe der nativen Importmethoden kurzzeitig denselben gültigen Client-Kontext, den ein reguläres Spiel-UI-Ereignis bereitstellt, und stellt den vorherigen Zustand anschließend wieder her. Das ist ausschließlich Testcode im nicht ausgelieferten Probe-DLL, keine zusätzliche Laufzeitkorrektur des Patches. Ohne diesen Ereigniskontext würde der native `GetClient`-Aufruf der Prüfhilfe abbrechen; ein solcher fehlender Testkontext wird nicht als Fehler des Importpatches behandelt.

Die bestehende Kartenfreigabeprüfung (853 native Kartenvergleiche, Ablehnungen ohne Konfigurationsverlust und synthetischer Online-Vorfilter ohne Netzwerk) besteht erneut. Auch die bisherigen 356 Parser-/Rundreisefälle, 216 nativen Leer-Vergleiche, 468 F8-Speicherprüfungen samt Privatstatus und die isolierten Editor-/Zielauswahlprüfungen in beiden Sprachen bestehen erneut. Sichtbare Nutzerabnahme und etwaige Veröffentlichung bleiben getrennte Schritte. Das veröffentlichte v0.1.1-Release wird durch diese Entwicklungsarbeit nicht aktualisiert.

## Nutzerabnahme und Release v0.1.2

Der Nutzer bestätigt die sichtbare Funktion des Import-Teststands und beauftragt am 19. November 2026 die Veröffentlichung als neue Version. Die Korrektur ist Bestandteil von v0.1.2; die oben dokumentierten Grenzen und ausstehenden Einzel-Effektprüfungen bleiben bestehen. v0.1.1 bleibt als vorherige Version erhalten.
