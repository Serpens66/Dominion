# Offline-Test mit Doppelklick

1. Dominion schließen. Internet und Steam können online bleiben.
2. **Offline-Test-starten.cmd** doppelklicken. **1** wählt den Messmodus, **2** aktiviert den Patch. Beim ersten Lauf die Arbeitskopie fertig kopieren lassen.
3. Warten, bis im Starterfenster **„Bereit“** steht. In Dominion das **Profil** öffnen und auf den **Verbindungsstatus** klicken, bis **„Offline-Modus“** angezeigt wird. Dann **Lokal spielen** wählen, eine Partie gegen die KI mit zwei Spielern anlegen und die **Königreich-Auswahl** öffnen. Den kompletten Text unten mit **Strg+V** ins **Kartensuchfeld** einfügen, mit **Enter** übernehmen und **Spielen** drücken.

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

Starte die Arbeitskopie mit **3 – Patch mit automatischer Verdopplung**. Prüfe, dass im Dominion-Profil **Offline-Modus** aktiv ist. Erstelle eine frische lokale Partie gegen die KI. Lass die zehn Königreichplätze zufällig und starte die Partie ohne einzufügenden Text.

- Ein normaler Königreichstapel startet mit **20 Karten**. Bei zwei Spielern beginnt ein gewöhnlicher Königreich-Siegkartenstapel wie Gärten mit **16**, ab drei Spielern mit **24**. Geteilte Stapel sollen beim Leerkaufen ihrer ersten Schicht zur richtigen zweiten Schicht wechseln.
- Die zehn gezogenen Hauptstapel erhalten ihre normalen nativen Effekte. Grundstapel wie Silber und Gold sowie Zusatzstapel werden durch die Automatik nicht als Ziele hinzugefügt. Ein bereits für dieselbe Karten-ID vorhandener Verdopplungs- oder Verdreifachungseffekt wird übernommen.
- Starte eine zweite frische Zufallspartie und vergleiche die Ziehung und Stapelgrößen. `automatic kingdom effects` im Patchprotokoll listet die endgültigen Effekte. Ein gespeichertes Set aus dieser Partie soll die erzeugte Liste auch in Modus 2 enthalten.

Bei zehn Verdopplungen und `twist_shoppers` enthält das gespeicherte lokale Setup elf Effekte. Der Nutzer bestätigte die Wirkung der Kombination am 10. November 2026. Im Dialog **Kampagnen-Features** ragen Zeilen bei dieser Anzahl über den Hintergrund. Laut Spieltest beeinträchtigt das den Ablauf nicht. Der Dialog lässt sich schließen oder über **Nicht wieder anzeigen** ausblenden.

Der Startmodus ist mit Build 3315 statisch und durch x86-Komponententests geprüft. Am 10. November 2026 bestätigte der Nutzer die automatische Verdopplung im Offline-Einzelspieler. Für eine frische Zufallspartie belegen Spielstand und Patchprotokoll genau zehn verschiedene Verdopplungseffekte. Ihre Ziele stimmen mit den zehn gezogenen Hauptstapeln überein. Die ursprüngliche Spielanforderung enthielt keine Effekte. Gezielte Spieltests für geteilte Stapel, Königreich-Siegkarten und erneutes Laden sind offen. Bei Platzmangel für die vollständige Liste startet die Partie mit den ursprünglichen Regeln und zeigt einen Hinweis.

## Privates Online-Spiel mit Patch nur beim Host

1. Schließe Dominion. Starte als Host **Online-Test-starten.cmd** per Doppelklick und warte auf **Bereit**. Der Starter aktiviert den Patch für feste Effekte und verwendet dasselbe separate Testprofil wie der Offline-Starter. Lass das Fenster bis zur Wiederherstellung des Originalprofils geöffnet.
2. Schalte im Dominion-Profil den **Offline-Modus** aus, falls er noch aktiv ist, und warte auf die Verbindung. Dein Mitspieler startet Dominion normal über Steam.
3. Füge in der Königreich-Auswahl fürs **Private Spiel** die komplette Zeile mit Strg+V ins Kartensuchfeld ein und bestätige mit Enter. Erstelle das Spiel mit diesem Set und lade den Mitspieler ein.

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(village), start_double_pile(market)
```

Beide prüfen zu Spielbeginn **Dorf = 20**, **Markt = 20** und **Werkstatt = 10**. Kauft für eine zusätzliche Prüfung zusammen elf Exemplare aus demselben verdoppelten Stapel. Für diesen Test werden die Effekte ausdrücklich im Set angegeben. Die automatische Verdopplung zufällig gezogener Stapel gehört weiterhin zu Modus 3 des Offline-Starters.

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
