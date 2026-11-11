# Lokaler Test mit Doppelklick

## Release-Kandidat v0.1.0, 11. November 2026

**Korrektur des Release-Aufbaus:** Der Release-Starter startet jetzt direkt die originale Steam-Installation mit dem normalen Profil. Er legt weder eine Spielkopie noch ein Testprofil an. Der neue Injector-Modus `--installed --apply` verlangt, dass der gestartete Prozess dieselbe Datei wie die gewählte Installation verwendet. Der Diagnose-Modus behält seine getrennte Kopie. Die unten protokollierten Kopiertests beschreiben den verworfenen ersten Kandidaten, nicht den aktuellen Release-Starter.

Der korrigierte Installationsmodus wurde im isolierten nativen Entwickler-Test geprüft: unterschiedliche Prozess- und Quelldateien werden abgelehnt, dieselbe Datei wird erfolgreich gepatcht. Die Bann-Erweiterung ist installiert; Große Ideen wirkt dreifach mit acht Handkarten. Build, Effektprüfungen, Initialisierungsprüfung und PowerShell-Syntaxprüfung bestehen. Der Release-Code enthält keine Kopierfunktion mehr.

Der neue Release-Starter ist **Dominion-mit-Mod-starten.cmd im entpackten Release-Paket**. Er verwendet das normale Profil; der unten beschriebene bisherige Test-Starter verwendet weiterhin sein getrenntes Testprofil. Anleitung und sichtbare Schlussabnahme stehen in [README](README.md) und [RELEASING](RELEASING.md).

**Automatisiert geprüft, Spielbuild 3315 / Steam-Build 17683350, EXE-Hash wie Quellenregister:**

- Staging-Build mit MSVC x86 `/W4` ohne Warnungen; Effektprüfungen bestanden. Launcher-Prüfung bestätigt erfolgreiche Initialisierung, Fehlerfall und Zeitüberschreitung ohne Beenden des Remote-Threads.
- Alle 89 Katalogeinträge, drei Beispiel-IDs, Wirkung, Zeitpunkt und Prüfkennzeichnung vollständig. 27 bislang offene Effekte sind als **Ungetestet** sichtbar; Teilbefunde bleiben als solche markiert. Offene Effekte bleiben auswählbar, bekannte Startabbrüche werden weiterhin abgefangen.
- Das tatsächliche ZIP außerhalb des Projektordners in einen Pfad mit Leerzeichen und Umlaut entpackt; Windows PowerShell 5.1 prüft sämtliche Paketdateien und Hashes. Kein Python und kein Compiler wird vom Release-Starter benötigt.
- Acht erzeugte Profilstände korrekt auf drei Sicherungen rotiert; unverändertes Profil ohne neue Sicherung. Gesperrte Sicherung abgelehnt, neueste vorhandene Sicherung dabei erhalten. Ausschließlich temporäre, erzeugte Profile verwendet.
- Steam-Bibliothek in abweichendem Pfad aus `libraryfolders.vdf` erkannt, konfigurierte Installation wiederverwendet, Spielkopie nur einmal erstellt. Falscher Build, falscher Hash, fehlende Paketdateien und unvollständige Kopie abgelehnt. Exklusive Sitzungssperre geprüft.
- Echter F8-Dialog auf eigenem unsichtbaren Windows-Desktop: 89 Einträge, Prüfkennzeichnungen, Suche nach den drei IDs sowie Zugbeginn/Spielaufbau, leeres Suchergebnis und Abbrechen bestanden. Keine Bedienung des aktiven Desktops.
- Tatsächlicher Injector aus dem entpackten Paket auf eigenem unsichtbaren Desktop: Original/Kopie-Identitätskollision abgelehnt, gültige Arbeitskopie erfolgreich gepatcht, geänderte lokale Startstelle und Online-Bannlimit im Speicher geprüft. Temporäres Offline-Profil mit zwei lokalen Spielern; erste Hand ohne Große Ideen **5**, mit drei Kopien **8** Karten und drei installierte Effekte. Das normale Profil wurde nicht verwendet.

**Noch offen:** sichtbare Schlussabnahme des Release-Starters mit normalem Profil in Lokal spielen und Weitergeben; tatsächliche Windows-Skalierung 125 %/150 %; offene Effekte und Kombinationen gemäß Katalog. Die Veröffentlichung auf GitHub folgt erst nach der sichtbaren Schlussabnahme. Die bisherige GitHub-Anmeldung Serpens66 und das bestehende Konverter-Release wurden lesend geprüft.

## Bisheriger Diagnose-Starter

1. Dominion schließen. Der Verbindungsstatus in Dominion und Steam kann unverändert bleiben.
2. Für den Patch **Lokal-Test-starten.cmd** doppelklicken. Für einen Vergleich im Messmodus den Starter aus dem Patchordner als `Lokal-Test-starten.cmd /menue` aufrufen und **1** wählen. Beim ersten Lauf die Arbeitskopie fertig kopieren lassen.
3. Warten, bis im Starterfenster **„Bereit“** steht. In Dominion **Lokal spielen** wählen, eine Partie gegen die KI mit zwei Spielern anlegen und die **Königreich-Auswahl** öffnen. Den kompletten Text unten mit **Strg+V** ins **Kartensuchfeld** einfügen, mit **Enter** übernehmen und **Spielen** drücken.

Der Starter verwendet ein eigenes Testprofil. Er benennt dein Originalprofil vorübergehend um und stellt es wieder her, sobald du Dominion schließt. Lass das Starterfenster bis zur Wiederherstellung geöffnet. Falls es vorher geschlossen wurde, Dominion beenden und den Starter erneut öffnen. Er stellt das Originalprofil vor der Modusauswahl wieder her. Die Arbeitskopie und das letzte Protokoll liegen unter `%LOCALAPPDATA%\DominionModTests\3315`.

## Dorf und Markt

Diesen Text zuerst in Modus **1** ausprobieren. Erwartet sind **Dorf 20, Markt 10**. Danach Dominion schließen, den Starter erneut öffnen und Modus **2** wählen. Mit derselben Zeile sollen **Dorf und Markt je 20** haben, Werkstatt weiterhin 10. Ein erneutes Einfügen derselben Zeile im gleichen Setup soll die beiden Stapel bei je 20 belassen.

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(village), start_double_pile(market)
```

## Dasselbe Set ohne Effekte

Speichere die Partie mit den beiden Effekten über das Burgsymbol unten rechts als **A**. Erstelle aus einer frischen Königreich-Auswahl mit der folgenden Zeile ein zweites Set **B** und speichere es auf demselben Weg. Starte mit aktivem Patch anschließend je eine Partie aus **A → B → A**. Dorf und Markt sollen bei **20/20 → 10/10 → 20/20** beginnen.

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop
```

## Zehn Verdopplungen und ein Zusatzkauf

Diese Zeile in einer frischen lokalen Königreich-Auswahl mit aktivem Patch einfügen. Erwartet sind **20 Karten pro Königreichstapel** und **zwei Käufe im ersten Zug**.

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(cellar), start_double_pile(market), start_double_pile(merchant), start_double_pile(militia), start_double_pile(mine), start_double_pile(moat), start_double_pile(remodel), start_double_pile(smithy), start_double_pile(village), start_double_pile(workshop), twist_shoppers
```

Wenn die Arbeitskopie das Hauptmenü nicht erreicht oder eine Erwartung abweicht, melde die Anzeige im Starter oder die beobachteten Stapelgrößen. Nach dem Schließen steht das letzte Patchprotokoll in `%LOCALAPPDATA%\DominionModTests\3315\letzter-test.log`. Es enthält Build, Modus, Effekt-IDs und deren Argumente.

## Bestätigte Einzelspieler-Tests

Am 10. November 2026 mit Dominion 3315, Steam-Build 17683350, separater Arbeitskopie und aktiviertem Offline-Modus. Der Nutzer startete lokale Partien gegen die KI mit der Testzeile für Dorf und Markt. Im Messmodus starteten Dorf und Markt mit 20 und 10 Karten. Mit aktivem Patch starteten beide mit 20 Karten. Das Patchprotokoll zeigt zwei übernommene Ziele. Im gespeicherten Setup stehen beide nativen `start_double_pile`-Einträge.

## Speichern der Effekte im Set

Beim ersten Speichertest am 10. November enthielt Set A in `profiles.xml` zehn Stapel-IDs und keinen `-x`-Abschnitt. Der Spielstand mit demselben Königreich enthielt beide Verdopplungen. `DomSaveSetString` verwendet beim Speichern aus der laufenden Partie an RVA `0x1FCD6D` die Kurzfassung des aufgelösten Kingdoms über `0x0DFB20`. Der Patch übernimmt für eine lokale Partie mit Effekten stattdessen den vollständigen nativen `kingdomData`-Text aus `ActiveGame+0x3C`. Beim Speichern eines vorbereiteten Setups mit Effekten wechselt er an RVA `0x1FCDAE` vom kompakten zum vollständigen Serializer. Beide Call-Sites wurden an der EXE von Build 3315 geprüft. Die neue Version muss im Spiel durch Speichern und Laden von **A2 → B → A2** bestätigt werden. `A2` ist das neu gespeicherte Set mit den beiden Verdopplungen. `B` ist das vorhandene Set ohne Effekte.

Die erste Fassung des Speichereingriffs stürzte beim Speichern aus einer laufenden Partie ab. Das Patchprotokoll endete nach der Auswahl des vollständigen Kingdom-Texts. Die x86-Analyse zeigte, dass `DomSaveSetString` einen noch nicht initialisierten Ausgabeslot erhält. Die erste Fassung verwendete dafür die Zuweisung an einen vorhandenen Spielstring. Die Korrektur verwendet den nativen String-Konstruktor bei RVA `0x23D720`. Ein isolierter x86-Test prüfte Argumentübergabe, den uninitialisierten Ausgabeslot und den Rückfall auf den Originalpfad. Der anschließende Offline-Test im Einzelspieler bestätigte den Set-Wechsel. Der Nutzer konnte das Set mit beiden Effekten speichern und die Effekte waren nur in diesem Set aktiv. Das Patchprotokoll zeigt beim Laden zwei Einträge für das Effektset und null für B. In der Testprofildatei stehen beide Verdopplungen im gespeicherten Effektset.

## Automatische Verdopplung im Einzelspieler

Starte die Arbeitskopie mit **3 – Patch mit automatischer Verdopplung**. Erstelle eine frische lokale Partie gegen die KI. Lass die zehn Königreichplätze zufällig und starte die Partie ohne einzufügenden Text.

- Ein normaler Königreichstapel startet mit **20 Karten**. Bei zwei Spielern beginnt ein gewöhnlicher Königreich-Siegkartenstapel wie Gärten mit **16**, ab drei Spielern mit **24**. Geteilte Stapel sollen beim Leerkaufen ihrer ersten Schicht zur richtigen zweiten Schicht wechseln.
- Die zehn gezogenen Hauptstapel erhalten ihre normalen nativen Effekte. Grundstapel wie Silber und Gold sowie Zusatzstapel werden durch die Automatik nicht als Ziele hinzugefügt. Ein bereits für dieselbe Karten-ID vorhandener Verdopplungs- oder Verdreifachungseffekt wird übernommen.
- Starte eine zweite frische Zufallspartie und vergleiche die Ziehung und Stapelgrößen. `automatic kingdom effects` im Patchprotokoll listet die endgültigen Effekte. Ein gespeichertes Set aus dieser Partie soll die erzeugte Liste auch in Modus 2 enthalten.

Bei zehn Verdopplungen und `twist_shoppers` enthält das gespeicherte lokale Setup elf Effekte. Der Nutzer bestätigte die Wirkung der Kombination am 10. November 2026. Im Dialog **Kampagnen-Features** ragen Zeilen bei dieser Anzahl über den Hintergrund. Laut Spieltest beeinträchtigt das den Ablauf nicht. Der Dialog lässt sich schließen oder über **Nicht wieder anzeigen** ausblenden.

Der Startmodus ist mit Build 3315 statisch und durch x86-Komponententests geprüft. Am 10. November 2026 bestätigte der Nutzer die automatische Verdopplung im Offline-Einzelspieler. Für eine frische Zufallspartie belegen Spielstand und Patchprotokoll genau zehn verschiedene Verdopplungseffekte. Ihre Ziele stimmen mit den zehn gezogenen Hauptstapeln überein. Die ursprüngliche Spielanforderung enthielt keine Effekte. Gezielte Spieltests für geteilte Stapel, Königreich-Siegkarten und erneutes Laden sind offen. Bei Platzmangel für die vollständige Liste startet die Partie mit den ursprünglichen Regeln und zeigt einen Hinweis.

## Privates Online-Spiel mit Patch nur beim Host

1. Schließe Dominion. Starte als Host **Online-Test-starten.cmd** per Doppelklick und warte auf **Bereit**. Der Starter aktiviert den Patch für feste Effekte und verwendet dasselbe separate Testprofil wie der lokale Starter. Lass das Fenster bis zur Wiederherstellung des Originalprofils geöffnet.
2. Schalte im Dominion-Profil den **Offline-Modus** aus, falls er noch aktiv ist, und warte auf die Verbindung. Dein Mitspieler startet Dominion normal über Steam.
3. Füge in der Königreich-Auswahl fürs **Private Spiel** die komplette Zeile mit Strg+V ins Kartensuchfeld ein und bestätige mit Enter. Erstelle das Spiel mit diesem Set und lade den Mitspieler ein.

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(village), start_double_pile(market)
```

Beide prüfen zu Spielbeginn **Dorf = 20**, **Markt = 20** und **Werkstatt = 10**. Kauft für eine zusätzliche Prüfung zusammen elf Exemplare aus demselben verdoppelten Stapel. Für diesen Test werden die Effekte ausdrücklich im Set angegeben. Die automatische Verdopplung zufällig gezogener Stapel gehört weiterhin zu Modus 3 des lokalen Starters.

## Diagnose nach dem privaten Spieltest

Am 10. November 2026 erschien laut Nutzer das Effektsymbol im privaten Setup. In der gestarteten Partie fehlten sowohl der Effekthinweis als auch die Verdopplung. Das Verhalten trat mit Patch und im unveränderten Spiel auf. Das Patchprotokoll des Hosts bestätigt beim Import die beiden `start_double_pile`-Einträge. Der damalige Build des Patches erfasste noch keine Netzwerkübergaben.

Wiederhole das private Spiel mit dem oben genannten Set über **Online-Test-starten.cmd**. Der Starter lädt die neue Diagnose automatisch. Dein Mitspieler startet normal über Steam. Sobald ihr die Stapelgrößen gesehen habt, reicht es, Dominion zu schließen. Der Starter legt das Protokoll unter `%LOCALAPPDATA%\DominionModTests\3315\letzter-test.log` ab. Die Diagnose protokolliert die zwei Testeffekte in beiden nativen Setup-Feldern sowie die Effekt-IDs und Argumente, die beim Host tatsächlich in den Spielzustand kopiert wurden.

| Protokoll | Folgerung |
| --- | --- |
| Vor dem Filter fehlen die Effekte | Das Online-Setup des Hosts verliert die Liste bereits vor dem Freischaltungsfilter. |
| Vor dem Filter vorhanden, bei `send create` in beiden Feldern verschwunden | Der Verlust liegt im Host-Client auf dem Weg zur ausgehenden Nachricht. |
| Bei `send create` vorhanden, in der passenden empfangenen Spielbeschreibung und beim Spielstart verschwunden | Die Effekte erreichen den Client nicht in der vom Dienst gelieferten Spielbeschreibung. Die genaue Serverregel ist damit nicht bestimmt. |
| In der passenden Beschreibung vorhanden, beim Laden oder im Spielzustand verschwunden | Der empfangende Client verliert sie bei der Übernahme oder beim Aufbau. |
| Im Spielzustand vorhanden, Vorrat bleibt bei 10 | Die weitere Ausführung der Effekte oder der Vorratsaufbau muss geprüft werden. |

### Ergebnis der Netzwerkmessung am 10. November 2026

Spielbuild 3315, Steam-Build 17683350. Der Host verwendete den Online-Starter mit Diagnose. Der Mitspieler trat über den regulären Client bei. Die Sitzung enthält mehrere Spielanlagen und genau einen protokollierten Start einer Netzwerkpartie. Für die letzte Anlage und den anschließenden Start steht im Prozessprotokoll:

| Station | `setupData` | `kingdomData` |
| --- | --- | --- |
| Host direkt vor dem Freischaltungsfilter | Nicht gemessen | Dorf und Markt |
| Ausgehende `CreateGameMessage` | Leer | Dorf und Markt |
| Empfangene Lobbybeschreibung | Dorf und Markt | Leer |
| Empfangene Beschreibung der gestarteten Partie | Dorf und Markt | Vorhanden, ohne `-x` |
| Save beim Netzwerk-Spielstart | Dorf und Markt | Vorhanden, ohne `-x` |

Der Host kopierte anschließend null Effekte in den Spielzustand. Der Freischaltungsfilter des Hosts hat die Effekte erhalten. Der Online-Dienst hat sie im ursprünglichen Setup-Text zurückgeliefert. Beim Auflösen des Königreichs fehlen sie in der vom Dienst gelieferten Spielkonfiguration. Der Client lädt bei vorhandenem `kingdomData` den Effektblock aus diesem aufgelösten Feld. Welche Regel auf dem Dienst die Liste entfernt oder beim Auflösen auslässt, geht aus der Clientmessung nicht hervor.

## Effektdialog in der lokalen Königreich-Auswahl

Für Build 3315 den **Lokal-Test-starten.cmd** doppelklicken. Unter **Lokal spielen** die Königreich-Auswahl öffnen und **F8** drücken. Für einen festen Test lässt sich die folgende Zeile in das Kartensuchfeld kopieren und mit Enter bestätigen.

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop
```

1. **F8** drücken. Links nach **Doppelter Vorratsstapel** suchen und unter der Tabelle **Ziele wählen** öffnen. Im Bereich **Königreich** unter **Basisspiel** Dorf und Markt anhaken. Rechts erscheint die Vorschau. Mit der Maus über einen Namen fahren oder mit den Pfeiltasten dorthin wechseln. Die Leertaste setzt oder entfernt ein Häkchen. **Fertig** drücken. Links **Kauflaune** suchen und **Effekt hinzufügen** wählen. Rechts stehen die drei aktiven Effekte in Ausführungsreihenfolge.
2. Unten im Effektdialog **Fertig** drücken und die Partie starten. Erwartet sind **Dorf 20**, **Markt 20**, **Werkstatt 10** und ein zusätzlicher Kauf zu Zugbeginn. Für den ersten Test bleibt die automatische Verdopplung ausgeschaltet.
3. Das Set über das Burgsymbol speichern. Danach das Set in einer frischen Königreich-Auswahl erneut laden und mit **F8** öffnen. Die drei Einträge sollen in derselben Reihenfolge erscheinen. Einen Eintrag entfernen und **Abbrechen** prüfen. Bei einem zweiten Versuch den Eintrag entfernen und **Fertig** wählen. Das Ergebnis wird zur eigenen Auswahl, die sich erneut als Set speichern lässt.
4. Für einen getrennten Test unten **Alle gezogenen Stapel verdoppeln** aktivieren. Eine frische Zufallspartie soll ihre Hauptstapel verdoppeln. Die Einstellung gilt für die laufende Testsitzung. Beim Doppelklick ist sie anfangs ausgeschaltet. Modus 3 im optionalen Startmenü schaltet sie beim Start ein.
5. Den festen Test auch mit **Weitergeben** auf dem zweiten Spielerplatz wiederholen. Eine bewusste zweite Dorf-Verdopplung mit **Weitere Kopie hinzufügen** prüfen. Erwartet sind **Dorf 40** und **Markt 20**. Einmal in der Kartenauswahl und einmal im Effektdialog abbrechen und die gespeicherte Auswahl danach erneut öffnen.

**Geprüft am 11. November 2026, Spielbuild 3315, Steam-Build 17683350.** Ein isolierter Win32-Dialogtest prüfte Suche über mehrere Begriffe, Hover- und Tastaturvorschau, bewusste Wiederholung, zwei getrennte Zielfelder, die Reihenfolge der Taifun-Ziele und Abbrechen. Von 31 Einträgen aus blieben bei einer Auswahl von zwei weiteren Einträgen alle 31 bestehen. Nach Abwahl eines Ziels ließ sich der 32. Platz belegen.

Die daraus erwarteten Effektbefehle wurden separat im nativen Spielstart mit zwei lokalen Spielerplätzen gemessen. Mit frischen Profilen ergab A → B → A für Dorf und Markt **20/20 → 10/10 → 20/20**. Eine zusätzliche Dorf-Kopie ergab **40/20**. Patrizier und Handelsplatz hatten im verdoppelten Stapel je **10**, Gärten hatte **16** Karten. Das Speichern über das Burgsymbol und erneute Laden über die sichtbare Oberfläche gehören zur manuellen Abnahme.

**Layoutprüfung der kompakten Fassung am 11. November 2026, Spielbuild 3315, Steam-Build 17683350.** Auf einem separaten Windows-Desktop maßen Effektdialog und Kartenauswahl bei 96 DPI **700 × 458** und **676 × 463** Pixel. Gegenüber den vorherigen Dialogvorlagen waren das jeweils knapp 28 % weniger Fläche. Größere Testschriften mit 11, 14 und 24 Punkt prüften Schaltflächenbreiten und die Begrenzung auf die Bildschirmfläche. Der Dialogtest prüfte auch die Vorschau ohne Auswahl, fehlende Bilder, leere Suche, scrollbaren Regeltext, die Kartenauswahl per Leertaste und das Bestätigen oder Abbrechen per Tastatur.

**F8-Tabelle am 11. November 2026.** Die breitere Fassung misst bei 96 DPI **998 × 651 Pixel**. Der isolierte Dialogtest prüfte alle 89 Zeilen auf vollständigen Zeilenumbruch innerhalb ihrer drei Spalten. Name, ID, Wirkung, Zeitpunkt und Teststand waren für alle Einträge vorhanden. Die Suche nach den drei IDs `twist_mirror_loot`, `stamp_two_for_one` und `start_trash_curses` sowie nach **Zugbeginn** und **Spielaufbau** funktionierte. Pfeiltasten, Scrollen, leere Trefferliste und der bekannte Startschutz wurden geprüft.

Dieselbe Dialogprüfung lief mit Testschriften von 11, 14 und 24 Punkt. Dabei blieben Fenster und Bedienelemente innerhalb der verfügbaren Bildschirmfläche und alle Tabellenzellen lesbar. Die Auswahltests bestätigten Dorf und Markt gemeinsam, das Überspringen identischer Einträge, eine bewusste Wiederholung, zwei getrennte Ziele, die geordnete Taifun-Liste, den Wechsel der aktiven Reihenfolge, Abbrechen und den Kapazitätsfall **31 → 33 abgelehnt → 32 übernommen**. Auch teilweise und vollständig zufällige Königreiche wurden geprüft. Die Windows-Skalierung wurde für diese automatisierten Tests unverändert gelassen.

Die Vorschau verwendet 773 lokal vorbereitete Kartenvorderseiten aus dem Bildpaket und den Spieltexturen. Drei Vorderseiten wurden ohne Illustration erzeugt. Im isolierten Dialog erschienen **Geizhals** im Hochformat und **Hochzeit** im Querformat. Der Wechsel zum scrollbaren Regeltext, die Rückkehr zur Karte und der Rückfall auf Text bei fehlender Bilddatei wurden geprüft.

Die sichtbare Abnahme des neuen Layouts erfolgt bei 100 %, 125 % und 150 % Windows-Skalierung in **Lokal spielen** und **Weitergeben**. In **Karte wählen** jeweils ein Einzelziel, mehrere Stapel und die geordnete Taifun-Liste öffnen. Mit Tab und den Pfeiltasten durch die Bedienelemente gehen und die Fokusmarkierung prüfen. In der Vorschau über **Kartentext lesen** einen langen Regeltext öffnen, scrollen und mit **Karte anzeigen** zur Vorderseite zurückkehren. Den Dialog auch am Bildschirmrand öffnen.

## Automatisierte lokale Effektprüfung im Hintergrund

**Im Spiel gemessen am 10. November 2026.** Build 3315, Steam-Build 17683350. Der Nutzer bestätigte vorab drei zusätzliche Karten durch dreimal Große Ideen. Der gespeicherte Spielstand und das Patchprotokoll enthalten jeweils die drei Einträge.

Der x86-Runner startet für jeden Vergleich eine frische Dominion-Arbeitskopie mit zwei menschlichen Spielerplätzen wie in **Weitergeben** auf einem separaten Windows-Desktop. Er erzeugt pro Fall ein eigenes Profil im temporären Verzeichnis, stellt die native Spielstartanforderung ein und liest den Spielzustand im ersten Zug aus. Die Messung benötigt keine Maus- oder Tastatureingaben. Jeder Fall prüft die Anzahl aktiver Effekte und den zum Effekt passenden Messwert. Die automatisierten Messungen prüfen die nativen Regeln nach dem Laden des Sets. Die Bedienung des F8-Editors gehört zum manuellen Test.

Aus dem Verzeichnis `KingdomEffectsPatch` den Runner bauen und mit dem Pfad der zuvor vom lokalen Starter angelegten Arbeitskopie starten.

```powershell
.\build.cmd background
& .\out\background_effect_test.exe "$env:LOCALAPPDATA\DominionModTests\3315\game\Dominion.exe"
```

Der Runner benötigt unter Windows das Recht, einen separaten Desktop zu erzeugen. Er bricht bei fehlendem Recht vor dem Spielstart ab. Die Konsolentabelle gibt den Effekt, die geplante und geladene Anzahl sowie Messwert und Erwartung aus. Ein optionales zweites Argument filtert die Testfälle nach dem Anfang der Effekt-ID, etwa `stamp_`. Spielstände und Protokolle liegen im ausgegebenen temporären Verzeichnis.

| Effekt | Messwert | 0 Kopien | 1 Kopie | 2 Kopien | 3 Kopien |
| --- | --- | ---: | ---: | ---: | ---: |
| Große Ideen | Erste Hand | 5 | 6 | 7 | 8 |
| Eingepfercht | Erste Hand | 5 | 4 | 3 | 2 |
| Penny-Party | Eigenes Kupfer im ersten Zug | 7 | 8 | 9 | 10 |
| Doppelter Vorratsstapel (Dorf) | Vorrat Dorf | 10 | 20 | 40 | 80 |
| Dreifacher Vorratsstapel (Dorf) | Vorrat Dorf | 10 | 30 | 90 | 270 |
| Eine Startkarte von jedem Stapel | Eigene Karten im ersten Zug | 10 | 20 | 30 | 40 |
| Doppeltes Startdeck | Eigene Karten im ersten Zug | 10 | 20 | 40 | 80 |
| Zusätzliche Startkarte (Silber) | Eigenes Silber im ersten Zug | 0 | 1 | 2 | 3 |
| Startkupfer oder Startanwesen gegen Silber, je als neue Kopie oder vom Stapel (vier Fälle) | Eigenes Silber im ersten Zug | 0 | 1 | 2 | 3 |
| Gut beschäftigt (`twist_busy_busy`) | Aktionen zu Beginn des ersten Zugs | 1 | 2 | 3 | 4 |
| Kauflaune (`twist_shoppers`) | Käufe zu Beginn des ersten Zugs | 1 | 2 | 3 | 4 |
| Das gewisse Etwas (`twist_extra_oomph`) | Münzen zu Beginn des ersten Zugs | 0 | 1 | 2 | 3 |
| `start_5_coffers` | Taler zu Spielbeginn | 0 | 5 | 10 | 15 |
| `start_10_villagers` | Dorfbewohner zu Spielbeginn | 0 | 10 | 20 | 30 |
| `start_10_favor` | Gefallen zu Spielbeginn | 0 | 10 | 20 | 30 |
| `twist_forever_favored` | Gefallen zu Beginn des ersten Zugs | 0 | 1 | 2 | 3 |
| `start_journey_facedown` | Seite des Reisemarkers | offen | verdeckt | offen | verdeckt |
| `start_trash_curses` | Flüche im Müll | 0 | 10 | 20 | 30 |
| `start_trash_pile(silver)` | Silber im Müll | 0 | 40 | 80 | 120 |
| `start_native_village(silver)` | Silber auf dem Tableau von Eingeborenendorf | 0 | 1 | 2 | 3 |
| `start_tavern(copper)` | Kupfer auf dem Wirtshaus-Tableau | 0 | 1 | 2 | 3 |
| `start_make_split_pile(village,smithy)` | Schmieden im Dorfstapel | 0 | 5 | 5 | 5 |
| `start_double_heirloom(pixie)` | Ziegen bei Kobold im Königreich | 1 | 2 | 3 | 4 |
| `start_in_play(search)` | Exemplare von Suche im Spielbereich bei Suche im Königreich | 0 | 1 | 2 | 3 |
| `twist_room_for_more(chapel)` | Kapellen im zusätzlichen Vorratsstapel | 0 | 10 | — | — |
| `stamp_got_it_already(travelling_fair)` | Käufe nach der kostenlosen Wanderkirmes | 1 | 3 | 5 | 7 |
| `stamp_bonus_money(travelling_fair)` | Münzen nach einem kostenlosen Kauf | 0 | 1 | 2 | 3 |
| `stamp_also_silver(travelling_fair)` | Eigenes Silber nach einem kostenlosen Kauf | 0 | 1 | 1 | 1 |
| `twist_favored_wealthy` | Gefallen nach dem Silberbonus der Wanderkirmes | 0 | 1 | 2 | 3 |
| `twist_favored_landlords` | Gefallen nach dem Anwesen durch Triumph | 0 | 1 | 2 | 3 |
| `twist_rebates` | Münzen nach dem Silber durch `delve` | 0 | 1 | 2 | 3 |
| `twist_double_up` | Eigenes Silber nach `delve` | 1 | 2 | 3 | 4 |
| `twist_incoming` | Silber auf dem Nachziehstapel nach `delve` | 0 | 1 | 1 | 1 |
| `stamp_with_villager(silver)` | Dorfbewohner nach dem Silber durch `delve` | 0 | 1 | 2 | 3 |
| `stamp_with_horse(silver)` | Eigene Pferde nach dem Silber durch `delve` | 0 | 1 | 2 | 3 |
| `stamp_with_spoils(silver)` | Eigene Beutekarten (`spoils`) nach dem Silber durch `delve` | 0 | 1 | 2 | 3 |
| `stamp_two_for_one(silver)` | Eigenes Silber nach `delve` | 1 | 2 | 3 | 4 |
| `stamp_fawning_service(travelling_fair)` | Käufe nach der Provinz durch `dominate` | 1 | 3 | 3 | 3 |
| `stamp_perks_of_wealth(travelling_fair)` | Käufe nach dem Gold durch `wedding` | 1 | 3 | 3 | 3 |
| `start_double_vp_tokens(arena)` | Siegpunktmarker auf Arena | 12 | 24 | 36 | 48 |
| `start_plus_8_vp_tokens(aqueduct)` | Siegpunktmarker auf dem Silberstapel | 8 | 16 | 24 | 32 |
| `start_plus_2_vp_tokens(defiled_shrine)` | Siegpunktmarker auf dem Dorfstapel | 2 | 4 | 6 | 8 |
| `start_with_cube(pageant)` | Projektsteine auf Festzug | 0 | 1 | Abbruch | Abbruch |
| `twist_mirror_loot` | Verschiedene Karten im 30 Karten großen Kostbarkeiten-Stapel | 15 | 1 | 1 | 1 |
| `start_same_reward` | Verschiedene Belohnungen bei Ritterkampf | 6 | 1 | 1 | 1 |
| `start_shuffle_pile(patrician)` | Die zwei Kartentypen im Stapel sind ineinander gemischt | nein | ja | ja | ja |
| `start_curse_cost_3` | Münzpreis eines Fluchs | 0 | 3 | 6 | 9 |
| `twist_sale` | Münzpreis von Dorf | 3 | 2 | 1 | 0 |
| `stamp_discounted(village)` | Münzpreis von Dorf | 3 | 2 | 1 | 0 |
| `stamp_pay_later(village)` | Preis von Dorf | 3 Münzen | 3 Schulden | 3 Schulden | 3 Schulden |
| `stamp_cost_zero(travelling_fair)` | Münzpreis der Wanderkirmes | 2 | 0 | 0 | 0 |
| `stamp_cost_2_less(dominate)` | Münzpreis von `dominate` | 14 | 12 | 10 | 8 |
| `stamp_two_for_one(village)` | Münzpreis von Dorf | 3 | 4 | 5 | 6 |
| `stamp_treasurified`, `stamp_victorified`, `stamp_durationified`, `stamp_reactionified`, `stamp_shadowified`, jeweils auf Dorf | Der jeweilige Zusatztyp ist laut Spielregel vorhanden | nein | ja | ja | ja |
| `twist_fast_track` | Ablageort des durch `delve` erhaltenen Silbers mit dem Zusatztyp Dauerkarte | Ablage | beiseite | Ablage | Ablage |

Bei den vier Austauschvarianten wurde das erhaltene Silber gezählt. Herkunft des Silbers und Entnahme der ursprünglichen Startkarte waren keine separaten Prüfwerte. **Platz für mehr** hat einen gemeinsamen Zielslot. Der F8-Editor lässt eine Auswahl dieses Effekts zu.

Für die Kaufboni bleibt `stamp_got_it_already(travelling_fair)` in allen Vergleichsfällen aktiv. Die zwei zusätzlichen Käufe belegen den ausgelösten Ereigniskauf. Für die Erhaltensregeln liefern der Silberbonus der Wanderkirmes, das Silber aus dem Vorrat durch `delve` und das Anwesen durch `triumph` gezielte Auslöser. Der Runner prüft, ob der erste Spieler die jeweilige Karte erhalten hat. Bei der Wanderkirmes mit Silberbonus blieben `twist_rebates` und `twist_double_up` in einer Erkundungsreihe ohne messbare Wirkung. Bei `delve` griffen beide.

Für Huldigung und Gold mit Vorzügen lösen kostenlose Käufe von `dominate` und `wedding` den jeweiligen Kartenerhalt aus. Die zwei zusätzlichen Käufe zeigen, dass die Wanderkirmes einmal ausgeführt wurde. Bei `stamp_two_for_one` wurde der Kartenbonus mit Silber und die Preisänderung mit Dorf gemessen.

Die Preis- und Typabfragen verwenden Dominions Regelfunktionen im ersten Zug. Bei den Zusatztypen steht der Spieltest der Folgeaktionen aus. Der Mischtest beobachtet die Stapelreihenfolge. Er wiederholt einen Fall bei Bedarf mit frischer Zufallsziehung, da auch ein gemischter Stapel zufällig wieder zwei zusammenhängende Gruppen haben kann. Für Überholspur wandelt `stamp_durationified(silver)` das mit `delve` erhaltene Silber in eine Dauerkarte um. Das spätere Ausspielen zu Beginn des Aufräumens steht aus.

`stamp_cost_2D_less(triumph)` brach bei einer, zwei und drei Kopien vor dem ersten Zug mit `ToKey(key, NULL) == key` ab. Die kleingeschriebene Variante `stamp_cost_2d_less(triumph)` lud im Vergleich keinen Effekt und ließ die Kosten bei fünf Schulden.

**Lokaler Startschutz für Build 3315.** Der Patch weist den Schuldenstempel in beiden Schreibweisen sowie wiederholte Projektstein-Einträge für dasselbe Projekt mit einer Meldung zurück. In isolierten Partien mit aktivem Patch startete ein Projektstein auf Festzug. Eine Kombination aus je einem Eintrag für Festzug und Straßennetz erreichte ebenfalls den ersten Zug. Zwei Einträge für Festzug wurden sowohl aus einer eigenen Auswahl als auch aus einem bereits gespeicherten Set abgefangen. Auch der Schuldenstempel mit Triumph wurde aus einem gespeicherten Set abgefangen. In diesen Fällen blieb die Partie ungestartet und es entstand kein Spielstand. Die Meldungen im F8-Dialog und im Kartensuchfeld sind durch die gemeinsame Validierung und Komponententests abgedeckt. Die sichtbare Bedienung dieser Fälle steht zur Abnahme aus.

Zwei und drei gleiche `start_with_cube(pageant)`-Einträge brachen vor dem ersten Zug mit der nativen Assertion `id != DATAID_NULL` ab. Beide Fälle erzeugten einen Minidump. Die reguläre Testreihe prüft für diesen Effekt den Ausgangswert und eine Kopie. Ob `start_allow_second_cube` die Kombination zulässt, ist offen.

**Noch offen.** Zwei lokale Erkundungsreihen mit Taifun erreichten den ersten Zug mit aktivem Effekt, aber ohne Kapelle im Vorrat. Ob ein Stapel später wechselt, bleibt offen. Für weitere Effekte braucht der Runner konkrete Spielzüge oder längere Partien, etwa um Karten auszuspielen, zu entsorgen oder die Endwertung zu erreichen.

## Verbliebene Effekte manuell prüfen

**Lokal-Test-starten.cmd** öffnen und unter **Lokal spielen** oder **Weitergeben** in der Königreich-Auswahl **F8** drücken. Die [Spielanleitung](../Documentation/Steam_Spielanleitung.md#katalog-der-kampagneneffekte) markiert offene und teilweise geprüfte Effekte. Für einen Vergleich dieselbe Ausgangslage zuerst ohne Zieleffekt, dann mit einer, zwei und drei Kopien starten. Hilfseffekte bleiben in allen Fällen gleich. Die dokumentierten Startabbrüche sind separat vermerkt.

Für die Stempel beim Ausspielen, Chamäleon und die Effekte auf die erste gespielte Karte das folgende Set ins Kartensuchfeld kopieren. Dadurch liegt Dorf bereits auf der ersten Hand. Den zu prüfenden Effekt danach über F8 hinzufügen und beim Ausspielen von Dorf Handgröße, Aktionen, Käufe oder Münzen beobachten. Für den Zusatztyp Dauerkarte auch den nächsten eigenen Zug prüfen.

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_add_card(village), twist_big_ideas, twist_big_ideas, twist_big_ideas, twist_big_ideas, twist_big_ideas, twist_big_ideas
```

Für Effekte am Ende des Zugs den betroffenen Spieler in **Weitergeben** den ersten Zug beenden lassen und den Beginn seines nächsten Zugs prüfen. Bei Müllhaufen und Deponie die Entsorgung einer Handkarte beobachten. Für Wege, Prophezeiungen, Projekte und die Endwertung das jeweilige Ziel im Königreich mitführen und dessen konkreten Auslöser ausspielen. Vorsprung und Rotationskoller haben laut Katalog einen Kampagnenbezug. Die lokale Testreihe deckt diese Kampagnenbedingungen bisher nicht ab.

## Erweiterte Online-Bannliste

**Sichtbare Eingabe und Speicherung bestätigt, 11. November 2026.** Der Nutzer konnte im Spiel mehr als fünf Online-Banns setzen. Nach dem Beenden der Testsitzung enthielt das separate Testprofil 16 Einträge in `bans` und dieselben 16 Einträge in `localBans`. Damit sind die Bedienung über das vorhandene Menü und die Speicherung beider Listen für diesen Test bestätigt.

**Nutzer-Spieltest vom 11. November 2026, Spielbuild 3315, Steam-Build 17683350.** Im regulären Profil wurde die Online-Bannliste auf 25 Einträge vorbereitet. Davon sperrten 22 die Basisspiel-Königreichkarten außerhalb der zehn erlaubten Karten, einschließlich der Karten aus der ersten Edition. Drei vorherige Banns anderer Erweiterungen blieben enthalten. Der Nutzer meldete drei private Partien mit dem Basisspiel, in denen ausschließlich Keller, Markt, Händlerin, Miliz, Mine, Burggraben, Umbau, Schmiede, Dorf und Werkstatt vorkamen. Damit ist die Wirkung einer größeren Online-Bannliste für diesen privaten Aufbau durch Spielbeobachtung bestätigt.

Für die neue Eingabe über das Spielmenü den **Lokal-Test-starten.cmd** öffnen und auf **Bereit** warten. In der Sammlung eine Karte mit Rechtsklick öffnen und **Verbieten → Online** verwenden. Mindestens sechs Karten derselben Kategorie hinzufügen, eine entfernen und erneut hinzufügen. Nach Beenden und Neustarten die Liste prüfen. Für Querkarten denselben Ablauf mit mindestens sechs Einträgen wiederholen. Die Zähleranzeige soll die jeweilige Anzahl ohne Nenner 5 zeigen. Die gemeinsame native Kapazität beträgt 700 Einträge. Der ursprüngliche Hilfetext nennt weiterhin fünf Online-Banns.

Der Starter verwendet sein separates Testprofil. Der vorherige Nutzer-Spieltest verwendete das reguläre Profil. Eine erneute private Online-Partie prüft die über das Menü erstellte Liste. Matchmaking und die volle Kapazität sind noch offen. [Codepfade und Eingriffspunkte](../Documentation/Modding_Overview.md#erweiterte-banns-über-die-vorhandene-galerie)

**Automatisierter nativer Test vom 11. November 2026.** Eine verifizierte Arbeitskopie wurde auf einem separaten Windows-Desktop mit einem temporären Offline-Profil gestartet. Der normale Patch wurde jeweils vor der Bedienprüfung installiert. Eine temporäre Testbibliothek leitete die Bannbearbeitung auf den Fensterthread des Spiels. Die Prüfung verwendete die Grenzargumente der tatsächlichen Online-Schaltfläche und die originale native Listenbearbeitung. Die Netzwerksynchronisierung wurde im Testaufruf ausgelassen.

Im Messmodus blieb die sechste Karte abgelehnt, beide Aufrufstellen und das Zählerformat blieben unverändert. Mit `--local --apply` und `--online --apply` wurden jeweils zwölf Karten und sechs Querkarten angenommen. Entfernen und erneutes Hinzufügen funktionierten. Die lokale Bannliste blieb leer. Nach einem neuen Prozessstart lud die originale Profilverwaltung alle 18 Online-Banns wieder. Die drei geänderten Speicherstellen wurden in beiden Patchmodi geprüft. Eine neue Online-Partie mit einer über die gepatchte Oberfläche angelegten Liste gehört zur sichtbaren Abnahme.
