# Dominion auf Steam ohne Mods

In Dominion von Temple Gates Games kannst du eigene Kartenauswahlen speichern und teilen. Die Steam-Version kennt außerdem Kampagneneffekte, die etwa Vorratsstapel vergrößern, Startkarten austauschen oder eine Regel für die ganze Partie ändern. Einige dieser Effekte lassen sich laut Community über das Kartensuchfeld eingeben. Wie weit das in deiner Version funktioniert, steht bei den Beispielen. [Steam-Funktionsübersicht](https://store.steampowered.com/app/1131620/Dominion/), [Kampagnenankündigung](https://steamcommunity.com/app/1131620/announcements/), [Community-Anleitung](https://www.reddit.com/r/dominion/comments/1pnmsph/custom_campaign_features_in_your_kingdoms_tgg/)

## Was im Menü verfügbar ist

- **Lokal spielen.** Spiele gegen die KI auf vier Schwierigkeitsstufen oder wechselt euch am selben PC ab. Empfohlene Sets liefern fertige Kartenauswahlen. [Steam-Funktionsübersicht](https://store.steampowered.com/app/1131620/Dominion/)
- **Online spielen.** Es gibt private Tische, eine Lobby und Spielersuche mit oder ohne Rangliste. Ihr könnt gleichzeitig spielen oder eure Züge zeitversetzt machen. [Steam-Funktionsübersicht](https://store.steampowered.com/app/1131620/Dominion/)
- **Tagesherausforderungen und Kampagnen spielen.** Die Tagesherausforderung bietet ein gemeinsames Königreich gegen die KI. Erweiterungskampagnen bestehen aus vorbereiteten Königreichen. Die Große Kampagne erzeugt eine thematische Reihe aus den gewählten Erweiterungen. Der Kampagnenzugang hängt vom Erweiterungsbesitz ab. [Steam-Funktionsübersicht](https://store.steampowered.com/app/1131620/Dominion/), [Kampagnenankündigung](https://steamcommunity.com/app/1131620/announcements/)

### Ein eigenes Königreich zusammenstellen und teilen

1. Beginne eine benutzerdefinierte Partie und öffne die Königreich-Auswahl. Lege die gewünschten Kartenstapel fest oder lasse Plätze zufällig füllen. [Community-FAQ](https://www.reddit.com/r/dominion/comments/1ahhoec/dominion_app_faq/)
2. Klicke in einer laufenden Partie mit der rechten Maustaste auf das Burgsymbol unten rechts. Dort kannst du das Königreich als Set speichern oder in die Zwischenablage kopieren. Gespeicherte Sets kannst du später in der Königreich-Auswahl laden. [Community-FAQ](https://www.reddit.com/r/dominion/comments/1ahhoec/dominion_app_faq/)
3. Füge kopierten Königreich-Text ins Kartensuchfeld einer neuen Spielkonfiguration ein und kontrolliere die angezeigten Karten vor dem Start. Der Import eines normalen Kingdom-Texts wurde im Projekt bereits vom Nutzer bestätigt. [Befund zur Zwischenablage](Kingdom_Sets.md#zwischenablage)

**Zufallsplätze auf eine Erweiterung begrenzen.** Gib in der Königreich-Auswahl den Namen einer Erweiterung ins Kartensuchfeld ein und wähle den passenden Zufallseintrag für einen Platz. Wiederhole das zum Beispiel dreimal mit Intrige, um drei zufällige Stapel aus Intrige zu erhalten. Die übrigen Plätze kannst du separat festlegen. Laut Community geht das auch für Landschaftstypen wie Ereignis oder Projekt. [Community-FAQ](https://www.reddit.com/r/dominion/comments/1ahhoec/dominion_app_faq/)

## Was über das Kartensuchfeld zusätzlich geht

Mit `-k` beginnt die Kartenauswahl. Hinter `-x` stehen die Kampagneneffekte. Verwende die internen Karten-IDs wie `village` für Dorf. Beim Start wirkt sich ein Vorratseffekt auf die kaufbaren Karten aus. Ein Startdeckeffekt kann zum Beispiel ein Startkupfer austauschen. Ein Stempel ändert eine bestimmte Karte oder Landschaft. Eine Spielvariation wie ein zusätzlicher Kauf verändert den Ablauf für alle Spieler. [Deutsche Kartentitel und IDs](Card_Titles_de.json), [Technische Referenz](Modding_Overview.md#kingdom-text-und-kampagneneffekte)

Die folgenden Zeilen sind **Beispiele für eine neue lokale Partie**. Beginne mit einer frischen Königreich-Auswahl, kontrolliere zuerst die Karten und prüfe anschließend die Wirkung im Spiel. Der Hinweis auf ein Kampagnenfeature allein bestätigt die Wirkung nicht. Für private Online-Partien ist die Übernahme der Effekte offen. [Community-Anleitung und Rückmeldungen](https://www.reddit.com/r/dominion/comments/1pnmsph/custom_campaign_features_in_your_kingdoms_tgg/)

### Ein Dorfstapel mit 20 Karten

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(village)
```

**Im Spiel bestätigt.** In einer lokalen Partie mit Build 3315 startete Dorf mit 20 Karten.

### Testzeile für zehn Verdopplungen

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(cellar), start_double_pile(market), start_double_pile(merchant), start_double_pile(militia), start_double_pile(mine), start_double_pile(moat), start_double_pile(remodel), start_double_pile(smithy), start_double_pile(village), start_double_pile(workshop)
```

**Im Spiel geprüft.** Bei diesem Import verdoppelte Build 3315 nur den zuerst genannten Stapel, Keller. Der lokale Spielstand enthält passend dazu nur `start_double_pile(cellar)`. Zwei `-x`-Blöcke in derselben Eingabe, mit oder ohne Komma, verdoppelten laut Nutzer keinen Stapel. [Technischer Befund zum Spieltest](Modding_Overview.md#spieltests-zur-stapelverdopplung)

### Mehrere Stapel über ein gespeichertes Set verdoppeln

**Im Spiel bestätigt für Build 3315.** Ein einzelnes gespeichertes Set `test` mit den [zehn Verdopplungen](#testzeile-für-zehn-verdopplungen) reicht. Der erste Klick auf **Spielen** gehört zum Ablauf.

1. Wähle `test` und klicke **Spielen**. In dieser ersten Partie sind die Stapel noch regulär groß.
2. Gib die Partie auf und kehre zur Setwahl zurück. Entlade `test`, ohne die Auswahl zu leeren. Der Marker für die Extraregeln bleibt sichtbar.
3. Wähle erneut `test` und klicke **Spielen**. Jetzt beginnen alle zehn Königreichstapel mit je 20 Karten.

Der Nutzer hat auch den Wechsel zu `test2` bestätigt. Dieses zweite Set enthält dieselben Karten und kommt im aktuellen Profil ohne eigene Effekte aus. Die zweite Partie übernahm trotzdem alle zehn Verdopplungen aus `test`. [Technischer Befund](Modding_Overview.md#warum-die-effekte-beim-set-wechsel-hinterherhinken)

Die [kommagetrennte Nachschlageliste](Start_Double_Pile_Karten.txt) enthält die 498 Königreichstapel aus dem Kartenkatalog sowie die sieben Grundstapel und vier optionale Vorratsstapel. Wähle die passenden Befehle und setze sie in einen einzigen `-x`-Abschnitt. Der Parser verarbeitet höchstens 32 Effekte pro Liste. Ein Ziel muss in der Partie beim Aufbau als Stapel vorhanden sein.

### Zufällige Königreiche mit doppelten Stapeln

Ein gespeichertes Set kann laut Spieltest ausschließlich `-x`-Effekte enthalten. Der Client würfelt die Kartenauswahl dann separat aus. Beim Laden einer Liste mit mehr als 32 Effekten stürzte Build 3315 ab. Die Nachschlageliste dient zum Auswählen der passenden Ziele. [Technischer Befund](Modding_Overview.md#effektsets-und-zufallsauswahl)

Für Zufallspartien aus einem festen Pool genügt eine einmal vorbereitete Effektliste. Die folgende Zeile deckt die 26 Königreichstapel des Basisspiels in zweiter Edition ab. Kopiere sie in `<strKingdom>` des gespeicherten Effektsets und beschränke die Zufallsauswahl im Spiel auf dieses Basisspiel. Wähle das Effektset, starte und gib die erste Partie auf. Entlade das Set ohne die Auswahl zu leeren, wähle dasselbe Effektset erneut und starte die eigentliche Partie. Der Ablauf kombiniert die bestätigte Effektübernahme mit einem größeren Zielpool. Eine Zufallspartie mit dieser konkreten Liste wurde noch nicht im Spiel geprüft.

```text
-x start_double_pile(artisan), start_double_pile(bandit), start_double_pile(bureaucrat), start_double_pile(cellar), start_double_pile(chapel), start_double_pile(council_room), start_double_pile(festival), start_double_pile(gardens), start_double_pile(harbinger), start_double_pile(laboratory), start_double_pile(library), start_double_pile(market), start_double_pile(merchant), start_double_pile(militia), start_double_pile(mine), start_double_pile(moat), start_double_pile(moneylender), start_double_pile(poacher), start_double_pile(remodel), start_double_pile(sentry), start_double_pile(smithy), start_double_pile(throne_room), start_double_pile(vassal), start_double_pile(village), start_double_pile(witch), start_double_pile(workshop)
```

Damit sind alle zehn gezogenen Königreichstapel abgedeckt, sofern die Auswahl auf diesen Pool beschränkt bleibt. Siegkartenstapel wie Gärten haben abhängig von der Spielerzahl eine andere Ausgangsgröße. Der Effekt verdoppelt die vorhandene Menge.

Für einen größeren Erweiterungspool zuerst die zehn Stapel auslosen und dann eine Effektliste für genau diese Auswahl erstellen. Der [vorhandene Konverter](../SetGenerator/README.md#web-namenslisten-umwandeln) kann die gezogenen Kartennamen aus dem Web-Randomizer in Dominion-IDs umwandeln. Trage jede der zehn IDs einmal als `start_double_pile(ID)` hinter `-x` im Effektset ein und spiele die gezogene Auswahl nach der Effektübernahme. Falls auch die sieben Grundstapel doppelt sein sollen, kannst du ihre IDs in die Liste für das konkrete Königreich aufnehmen. Die 26 Königreichstapel des gesamten Basisspiels plus die sieben Grundstapel ergäben dagegen schon 33 Effekte.

### Ein zusätzlicher Kauf pro Zug

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x twist_shoppers
```

**Erwartung.** Jeder Spieler bekommt zu Beginn seines Zuges einen zusätzlichen Kauf. Der Effektname und seine Beschreibung sind in Build 3315 vorhanden. Ein lokaler Spieltest mit diesem Import steht aus. [Beschreibung der Spielvariation](https://wiki.dominionstrategy.com/index.php/Campaigns_%28Temple_Gates_Games%29#Twists)

### Alle 27 Spielvariationen mit `twist_`

Die Tabelle zeigt alle in Build 3315 registrierten `twist_`-Varianten. Für eine eigene Partie ersetzt du `twist_shoppers` in der Beispielzeile durch den gewünschten Befehl. Die Varianten mit Klammern brauchen zusätzlich eine Karten-ID oder eine Liste von IDs. Die Wirkungen stammen aus den Kampagnenbeschreibungen. Ob die Varianten im eigenen Königreich genauso greifen, ist hier noch nicht im Spiel geprüft. [Kampagneneffekte im Dominion-Wiki](https://wiki.dominionstrategy.com/index.php/Campaigns_%28Temple_Gates_Games%29#Twists), [Community-Syntax](https://www.reddit.com/r/dominion/comments/1pnmsph/custom_campaign_features_in_your_kingdoms_tgg/)

| Was sich ändert | Befehl | Wirkung |
| --- | --- | --- |
| Zugbeginn | `twist_big_ideas` | Ziehe eine zusätzliche Karte. |
| Zugbeginn | `twist_busy_busy` | Bekomme eine zusätzliche Aktion. |
| Zugbeginn | `twist_extra_oomph` | Bekomme eine zusätzliche Münze. |
| Zugbeginn | `twist_forever_favored` | Bekomme einen Gefallen. |
| Zugbeginn | `twist_garbage_heap` | Du darfst eine Handkarte entsorgen. |
| Zugbeginn | `twist_penny_party` | Erhalte ein Kupfer. |
| Zugbeginn | `twist_shoppers` | Bekomme einen zusätzlichen Kauf. |
| Hand und Aktionen | `twist_cramped` | Die normale Handgröße sinkt auf vier Karten. |
| Hand und Aktionen | `twist_from_the_throne` | Spiele deine erste Aktionskarte im Zug zweimal aus. |
| Hand und Aktionen | `twist_landfill` | Wenn du eine eigene Karte entsorgst, ziehst du eine Karte. |
| Hand und Aktionen | `twist_so_much_stuff` | Von der ersten Karte, die du in deinem Zug ausspielst, erhältst du eine zusätzliche Kopie von außerhalb des Vorrats. |
| Karten erhalten | `twist_double_up` | Von der ersten Aktions- oder Geldkarte, die du im Zug erhältst, bekommst du eine zusätzliche Kopie von außerhalb des Vorrats. |
| Karten erhalten | `twist_fast_track` | Lege eine erhaltene Dauer- oder Reservekarte beiseite und spiele sie zu Beginn des nächsten Aufräumens aus. |
| Karten erhalten | `twist_favored_landlords` | Für jede erhaltene Siegkarte bekommst du einen Gefallen. |
| Karten erhalten | `twist_favored_wealthy` | Für jede erhaltene Geldkarte bekommst du einen Gefallen. |
| Karten erhalten | `twist_incoming` | Erhaltene Aktions- und Geldkarten kommen direkt oben auf deinen Nachziehstapel. |
| Karten erhalten | `twist_rebates` | Für jede erhaltene Karte bekommst du eine Münze. |
| Kaufen und Vorräte | `twist_beakers` | Ungenutzte Tränke bleiben für spätere Züge erhalten. |
| Kaufen und Vorräte | `twist_delayed_joy` | Du kannst Gaben bis zum nächsten Zug aufbewahren. |
| Kaufen und Vorräte | `twist_pennies_saved` | Übrige Münzen werden am Ende der Kaufphase zu Talern. |
| Kaufen und Vorräte | `twist_real_estate_barons` | Du kannst Siegpunktmarker wie Taler ausgeben. |
| Kaufen und Vorräte | `twist_sale` | Karten kosten eine Münze weniger. |
| Aufbau und Auslage | `twist_head_start` | In der Kampagne bekommt der Mensch zwei Extrazüge am Anfang und die KI zehn Siegpunktmarker. |
| Aufbau und Auslage | `twist_mirror_loot` | Alle Karten im Loot-Stapel sind gleich. |
| Aufbau und Auslage | `twist_room_for_more(Karten-ID)` | Fügt einen weiteren Königreichstapel hinzu. |
| Aufbau und Auslage | `twist_turn_turn_turn` | In der Kampagne rotieren geteilte Stapel zu Beginn der menschlichen Züge ab dem zweiten. |
| Aufbau und Auslage | `twist_typhoon(Karten-ID, …)` | In der Kampagne werden im Spielverlauf Königreichstapel ausgetauscht. |

Die anderen Kampagneneffekte findest du in der technischen Referenz, ebenfalls mit kurzer deutscher Erklärung. [Stempel für einzelne Karten und Landschaften](Modding_Overview.md#stempel-für-karten-und-landschaften), [Änderungen beim Spielaufbau](Modding_Overview.md#änderungen-beim-spielaufbau)
