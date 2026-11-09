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

Die folgenden Zeilen sind **Testbeispiele für eine neue lokale Partie**. Beginne mit einer frischen Königreich-Auswahl, kontrolliere zuerst die Karten und prüfe anschließend die Wirkung im Spiel. Der Hinweis auf ein Kampagnenfeature allein bestätigt die Wirkung nicht. Für private Online-Partien ist die Übernahme der Effekte offen. [Community-Anleitung und Rückmeldungen](https://www.reddit.com/r/dominion/comments/1pnmsph/custom_campaign_features_in_your_kingdoms_tgg/)

### Ein Dorfstapel mit 20 Karten

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(village)
```

**Erwartung.** Dorf startet mit 20 Karten. Die übrigen neun Königreichstapel starten mit jeweils 10. Der Verdopplungscode ist in Build 3315 geprüft. Der Spieltest dieses einzelnen Beispiels steht aus.

### Zehn Stapel mit je 20 Karten

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(cellar), start_double_pile(market), start_double_pile(merchant), start_double_pile(militia), start_double_pile(mine), start_double_pile(moat), start_double_pile(remodel), start_double_pile(smithy), start_double_pile(village), start_double_pile(workshop)
```

**Ergebnis des bisherigen Spieltests.** Der Hinweis auf ein Kampagnenfeature erschien, die Stapel hatten weiterhin je 10 Karten. Im zeitlich passenden lokalen Spielstand stand eine andere Kartenauswahl und als einziger Effekt die Verdopplung von Keller. Keller fehlte in diesem Vorrat. Falls der Spielstand zu diesem Test gehört, hatte der gespeicherte Effekt damit kein Ziel. Die Zeile bleibt ein Testbeispiel. Eine frei wählbare Stapelgröße von genau 15 Karten ist bisher offen. [Technischer Befund zum Spieltest](Modding_Overview.md#spieltest-mit-zehn-verdopplungen)

### Ein zusätzlicher Kauf pro Zug

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x twist_shoppers
```

**Erwartung.** Jeder Spieler bekommt zu Beginn seines Zuges einen zusätzlichen Kauf. Der Effektname und seine Beschreibung sind in Build 3315 vorhanden. Ein lokaler Spieltest mit diesem Import steht aus. [Beschreibung der Spielvariation](https://wiki.dominionstrategy.com/index.php/Campaigns_%28Temple_Gates_Games%29#Twists)
