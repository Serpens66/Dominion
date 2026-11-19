# Zusätzliche Startkarten mit eigenem Spielaufbau

## Befund und Grenzen

**Stand 19. November 2026.** Dominion-Spielbuild **3315**, Steam-Build **17683350**, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Der Hash wurde vor der statischen Analyse erneut geprüft. Adressen unten sind RVAs in dieser EXE.

Der Nutzer hat berichtet: `start_add_card(black_market)` gibt die Schwarzmarkt-Karte, erstellt aber kein Schwarzmarkt-Deck. Damit fehlt beim Spielen die Auswahl aus diesem Deck. Die statische Untersuchung erklärt dieses Verhalten: Eine Karteninstanz anzulegen ist nicht dasselbe wie deren zusätzlichen Spielaufbau auszuführen.

**Bekannte Einschränkung, noch nicht korrigiert.** Die vorhandene Kartenfreigabeprüfung bleibt bestehen. Ein sicherer vorhandener Weg ist, Schwarzmarkt regulär unter die Königreichstapel aufzunehmen und zusätzlich den Startkarten-Effekt zu verwenden. Dann führt das Spiel den normalen Schwarzmarkt-Aufbau aus. Die tatsächliche Kombination benötigt noch eine gezielte Spielabnahme; der Aufbau und die Speicherung des regulären Schwarzmarkt-Elternstapels sind nativ belegt.

Gold und Platin benötigen kein eigenes Hilfsdeck. Ihre erfolgreich geprüfte Verwendung als Startkarten belegt deshalb nicht automatisch die Eignung aller Sonderkarten. Weitere Karten mit Zusatzaufbau müssen einzeln geprüft werden.

## Native Ursache

| Bestandteil | Statisch geprüfter Pfad |
| --- | --- |
| Kartenidentitäten | Schwarzmarkt `0x1300`, Schwarzmarkt-Deck `0x1301`. Die ältere Vermutung zum Sonderfall `0xD00` betrifft nicht Schwarzmarkt. |
| Startkarte | Der Startkarten-Wrapper bei `0x10FF80` ruft `0x10FC20` auf. Diese Routine erstellt die Karte mit `CardCreate` bei `0x1727E0`, ohne den allgemeinen Karten-Aufbau `0x1735A0` auszuführen. |
| Schwarzmarkt-Metadaten | CardDef bei `0x6F3C40`: Aufbau-Datensatz vom Typ `0xD`, Zusatzkartenindex **24**, bis zu **60** Karten, Generierungsfunktion `0x150640`, Aufbau-Funktion `0x150790`, Kennzeichen `0x4000`. |
| Generierung | `0x150640` zieht geeignete Karten aus dem nativen Auswahlvorrat. `0x18E890` verarbeitet Abhängigkeiten und behandelt Schwarzmarkt verzögert; der bestehende verzögerte Durchgang liegt bei `0x1906C0`. |
| Aufbau | `0x150790` erwartet mindestens **15** Einträge, legt den Hilfsstapel `0x1301` an und ruft für dessen Karten weitere native Aufbaupfade auf. Ein nachträglicher Aufruf mit leerer Liste ist daher kein sicherer Fix. |
| Zeitpunkt | Der Zusatzaufbau muss vor der Entscheidung über abhängige Vorräte wie Trank stattfinden. Diese Entscheidung folgt bei `0x18CC8E`; ein Eingriff erst nach dem Aufbau aller Vorräte wäre zu spät. |

Die Liste liegt in der geparsten Konfiguration ab `0x340` als bis zu 60 Einträge zu 16 Bytes, im generierten Königreich ab `0x178` als Karten-IDs und im Spielzustand ab `G+0xEC4`. Ein zukünftiger Fix benötigt sowohl die Generierung dieser Liste als auch den regulären abhängigen Aufbau. Nur eine Karte oder ein leerer Hilfsstapel genügt nicht.

## Native Grammatik und Speicherung

**Im isolierten nativen Parser geprüft**, ohne Spielstart und ohne Eingabegeräte: `startup_effect_test.exe ... --black-market-grammar`, Probephase **80**. Der lokale Rohbericht `black-market-grammar.tsv` gehört nicht zum Release. Jede Variante wurde eingelesen, mit dem nativen Konfigurationsschreiber ausgegeben und erneut eingelesen. Die Kontrollliste enthielt 15 gültige Basisspielkarten.

| Variante | Hauptstapel unverändert | Hilfskarten vorher → nachher | Ergebnis |
| --- | --- | --- | --- |
| Zehn Hauptstapel und `start_add_card(black_market)` | Ja | 0 → 0 | Effekt gespeichert, zusätzlicher Aufbau fehlt. |
| Schwarzmarkt als einer der zehn Hauptstapel mit Kartenliste | Ja | 15 → 15 | Regulärer Elternstapel samt Hilfsliste vollständig erhalten. |
| Zusätzlich `black_market_deck(…)` | Ja | 0 → 0 | Keine eigenständige Hilfsliste. |
| Zusätzlich Schwarzmarkt als elfter Hauptstapel | Ja | 0 → 0 | Kein geeigneter Weg für eine reine Hilfsliste. |
| `-m BlackMarket(…)` | Ja | 0 → 0 | Kein bestätigter Aufbaumodifikator. |
| `start_add_card(black_market(…))` | Ja | 0 → 0 | Ziel geht verloren; Ausgabe enthält nur `start_add_card` ohne Ziel. Nicht verwenden. |

Der im Programm vorhandene Text `BlackMarket` bezeichnet hier eine native Smartplay-Einstellung und belegt keinen gleichnamigen Aufbaumodifikator.

Der Königreich-Schreiber `0x0DF830` besucht die zehn Hauptstapel und vier Landschaftseinträge. `0x0DF630` hängt deren Zusatzkarten an den jeweiligen Elternstapel an. Der Konfigurationsschreiber verwendet entsprechend `0x0DF8F0`. Eine Schwarzmarkt-Hilfsliste ohne Schwarzmarkt-Elternstapel wird dadurch nicht ausgegeben. Die Effekt-Ausgabe `0x0E5370` schreibt beim Startkarten-Effekt lediglich die Ziel-ID, keine verschachtelte Hilfsliste.

Diese Lücke ist auch für den ersten Spielstart relevant: Der native Startpfad erzeugt das Königreich nach der Textausgabe erneut und vergleicht das Ergebnis bei `0x1EA080`. Ein nur im Arbeitsspeicher ergänztes Deck wäre damit weder für diesen Vergleich noch für gespeicherte Partien ausreichend abgesichert.

## Warum ein eigener Zufallsseed noch keine Speicherlösung ist

Die native Auswahl verwendet einen 16-Byte-PCG-Zustand über den Generatorzeiger bei `0x8CE9BC`. `0x23ED10` zieht daraus Zufallswerte. Der Kandidatenvorrat umfasst mehrere veränderliche Listen; `0x18DCC0` entnimmt Karten durch Ersetzen mit dem letzten Listeneintrag, und `0x18DD90` führt vorübergehend verworfene Kandidaten zurück.

Ein gleicher Seed allein garantiert daher keine gleiche Hilfsliste: Auch Kandidatenmenge, Reihenfolge und bereits aufgelöste Abhängigkeiten müssen übereinstimmen. Ob eine normalisierte Arbeitskopie des nativen Vorrats zwischen erster Generierung, erneutem Einlesen und späterem Laden übereinstimmen würde, wurde **nicht nachgewiesen**. Änderungen an Freigaben und Auswahlvorgaben müssen ebenfalls berücksichtigt werden. Dieser experimentelle Weg wurde nicht in den Produktionspatch übernommen.

## Anforderungen an einen späteren Fix

- Den zusätzlichen Aufbau zunächst gezielt für `start_add_card` untersuchen; weitere Effekte und Karten nicht pauschal als unterstützt einstufen.
- Die tatsächlich gewählten Hilfskarten mit allen benötigten Zuordnungen speichern und erneut laden können. Die zehn Hauptstapel dürfen dabei unverändert bleiben.
- Native Kandidatenregeln und offizielle Kartenfreigaben einhalten. Weder unbekannte Freigaben als erlaubt behandeln noch Erweiterungen oder Hauptstapel automatisch einschalten.
- Generierung, nativen zweiten Durchgang, gespeicherte Sets und das Laden einer laufenden Partie vergleichen; Identität, Reihenfolge und zusätzliche Abhängigkeiten prüfen.
- Doppelten Aufbau vermeiden, wenn Schwarzmarkt bereits regulär enthalten ist. Zu kleine oder nicht zuverlässig bestimmbare Hilfslisten vor einem unsicheren Aufbau verständlich ablehnen.

Bis diese Punkte belegt sind, bleibt der native Sonderfall dokumentiert und wird nicht durch einen unvollständigen Aufbau-Eingriff ersetzt.
