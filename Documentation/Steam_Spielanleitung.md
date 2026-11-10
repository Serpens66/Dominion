# Dominion auf Steam / Dominion on Steam

[Deutsch](#dominion-auf-steam-ohne-mods) | [English](#dominion-on-steam-without-mods)

**Inhaltsverzeichnis / Contents**

- **Deutsch:** [Königreich speichern und teilen](#königreich-speichern-und-teilen) · [Kampagneneffekte eingeben](#kampagneneffekte-eingeben) · [Effektkatalog](#katalog-der-kampagneneffekte)
- **English:** [Save and share a Kingdom](#save-and-share-a-kingdom) · [Enter campaign effects](#enter-campaign-effects) · [Effect catalog](#campaign-effect-catalog)

## Dominion auf Steam ohne Mods

### Königreich speichern und teilen

1. Öffne die Königreich-Auswahl einer benutzerdefinierten Partie und wähle die Stapel.
2. Klicke in der laufenden Partie mit der rechten Maustaste auf das Burgsymbol unten rechts. Speichere das Königreich als Set oder kopiere es in die Zwischenablage.
3. Lade ein gespeichertes Set in der Königreich-Auswahl oder füge den kopierten Text dort ins Kartensuchfeld ein. [Zwischenablage und Spielbefund](Kingdom_Sets.md#zwischenablage)

Für einen Zufallsplatz aus einer bestimmten Erweiterung suche nach ihrem Namen und wähle den passenden Zufallseintrag. [Community-FAQ](https://www.reddit.com/r/dominion/comments/1ahhoec/dominion_app_faq/)

### Kampagneneffekte eingeben

#### Ein Dorfstapel mit 20 Karten

Füge diese Zeile ins Kartensuchfeld einer neuen lokalen Partie ein. Im Spieltest mit Build 3315 startete Dorf mit 20 Karten.

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(village)
```

Hinter `-k` stehen die IDs der Karten und Landschaften. Hinter `-x` kannst du den Beispielbefehl durch einen Effekt aus dem Katalog ersetzen. Setze für Platzhalter wie `Karten-ID` die Ziel-ID ein. Zielkarten und Landschaften für Stempel müssen in der Partie vorkommen. Trenne mehrere Effekte durch Kommas in einem einzigen `-x`-Abschnitt. [Deutsche Kartentitel und IDs](Card_Titles_de.json)

Das Suchfeld übernimmt jeden Effektbefehl nur einmal, auch bei verschiedenen Zielen. Für mehrere `start_double_pile`-Einträge verwende ein gespeichertes Set. [Befund zum Suchfeld](Modding_Overview.md#warum-das-suchfeld-nur-einen-stapel-verdoppelt)

#### Mehrere Stapel über ein gespeichertes Set verdoppeln

Bereite ein Set mit den gewünschten Verdopplungen vor. Verwende höchstens **32 Effekte** pro Liste. Ab dem 33. Eintrag bricht Build 3315 beim Laden ab. [Effektset vorbereiten](../SetGenerator/README.md#html-konverter-mit-doppelten-stapeln), [Befund zur Grenze](Modding_Overview.md#effektsets-und-zufallsauswahl)

1. Lade das Effektset und klicke auf **Spielen**.
2. Gib die Partie auf und entlade das Set in der Königreich-Auswahl. Lass die Zusatzregeln stehen. Ihr Marker bleibt sichtbar.
3. Lade dasselbe Set erneut oder wähle ein separates Set mit dem gewünschten Königreich. Klicke auf **Spielen**. [Lokal bestätigter Ablauf](Modding_Overview.md#warum-die-effekte-beim-set-wechsel-hinterherhinken)

#### Zufällige Königreiche mit doppelten Stapeln

Ein Set mit ausschließlich `-x` lässt das Spiel das Königreich auswürfeln. Passt der ganze Zufallspool in die Liste, nimm pro möglichem Ziel einen Eintrag aus der Nachschlageliste. Bei größeren Pools ziehe zuerst die zehn Stapel und erstelle das Effektset für diese Auswahl. Verdoppelt werden nur Stapel, die beim Aufbau vorhanden sind. [Nachschlageliste](Start_Double_Pile_Karten.txt), [Spielbefund](Modding_Overview.md#effektsets-und-zufallsauswahl)

#### Zwölf oder mehr Königreichstapel

Im lokalen Test blieb das gespeicherte Set mit `twist_room_for_more(chapel)` bei zehn Stapeln. Die achte Füllhorn-Mission sieht in ihrer Vorlage zwölf Königreichstapel im Vorrat vor. Zu den zehn Hauptstapeln kommen Vorbote als Bann der Jungen Hexe und Gärten durch „Platz für Mehr“. [Spieltest und Kampagnenvorlage](Modding_Overview.md#zusatzstapel-durch-platz-für-mehr)

### Katalog der Kampagneneffekte

Die Tabellen erfassen 89 Befehle und sechs Varianten der Kampagnenerzeugung in Spielbuild **3315** (Steam-Build **17683350**). Namen und Regelbegriffe stammen aus den deutschen Spieltexten. Die Beschreibungen fassen die Kampagnenwirkung zusammen. Im privaten Online-Spiel fehlten die beiden getesteten Verdopplungen in der gestarteten Partie. [Online-Befund](Modding_Overview.md#kampagneneffekte-im-privaten-online-spiel), [Technischer Effektkatalog](Modding_Overview.md#belegstand-des-effektkatalogs), [Lokale Spieltexte](Sources/Source_Register.md#deutsche-kartentitel-aus-l05)

+![1 Geld](Spielzeichen/geld-1.png) erhöht dein Kaufbudget für diesen Zug um 1. Einen Taler kannst du auch in einem späteren Zug ausgeben.

#### Alle 27 Spielvariationen mit `twist_`

Eine **Nicht-Vorrats-Kopie** ist ein zusätzliches Exemplar der Karte, das du von außerhalb des Vorrats nimmst.

| Name im Spiel | Befehl | Wirkung |
| --- | --- | --- |
| Große Ideen | `twist_big_ideas` | Zu Beginn deines Zuges **+1 Karte**. |
| Gut beschäftigt | `twist_busy_busy` | Zu Beginn deines Zuges **+1 Aktion**. |
| Das gewisse Etwas | `twist_extra_oomph` | Zu Beginn deines Zuges +![1 Geld](Spielzeichen/geld-1.png). |
| Für immer Verbündet | `twist_forever_favored` | Zu Beginn deines Zuges **+1 Gefallen**. |
| Müllhaufen | `twist_garbage_heap` | Zu Beginn deines Zuges darfst du eine Handkarte entsorgen. |
| Penny-Party | `twist_penny_party` | Nimm zu Beginn deines Zuges ein Kupfer. |
| Kauflaune | `twist_shoppers` | Zu Beginn deines Zuges **+1 Kauf**. |
| Eingepfercht | `twist_cramped` | Du ziehst deine normale Hand mit 4 statt 5 Karten. |
| Vom Thron | `twist_from_the_throne` | Die erste Aktionskarte, die du pro Zug spielst, wird automatisch zweimal gespielt. |
| Deponie | `twist_landfill` | Wenn du eine deiner Karten entsorgst, **+1 Karte**. Das gilt auch für Karten, die du von einem anderen Ort als deiner Hand entsorgst. |
| So viel Zeugs | `twist_so_much_stuff` | Wenn du deine erste Karte in einem Zug spielst, nimm eine Nicht-Vorrats-Kopie dieser Karte. |
| Verdopplung | `twist_double_up` | Wenn du in einem Zug zum ersten Mal eine Aktions- oder Geldkarte nimmst, nimm zusätzlich eine Nicht-Vorrats-Kopie dieser Karte. |
| Überholspur | `twist_fast_track` | Wenn du eine Dauer- oder Reservekarte nimmst, lege sie zur Seite und spiele sie zu Beginn der kommenden Aufräumphase. |
| Grundbesitzer Verbündet | `twist_favored_landlords` | Wenn du eine Punktekarte nimmst, **+1 Gefallen**. |
| Reich Verbündet | `twist_favored_wealthy` | Wenn du eine Geldkarte nimmst, **+1 Gefallen**. |
| Auf dem Weg | `twist_incoming` | Wenn du eine Aktions- oder Geldkarte nimmst, kommt sie sofort oben auf deinen Nachziehstapel statt auf den Ablagestapel. |
| Bonusauszahlung | `twist_rebates` | Wenn du eine Karte nimmst, +![1 Geld](Spielzeichen/geld-1.png). |
| Bechergläser | `twist_beakers` | Ungenutztes Trank-Guthaben bleibt erhalten, bis du es ausgibst. Du kannst es wie Taler in späteren Zügen zum Bezahlen verwenden. |
| Verzögertes Glück | `twist_delayed_joy` | Du darfst jede Gabe aufbewahren und erst im nächsten Zug ausführen. |
| Gesparte Pennys | `twist_pennies_saved` | Am Ende deiner Kaufphase wird übriges Geld zu Talern. |
| Großeigentümer | `twist_real_estate_barons` | Du kannst Siegpunktmarker als Taler ausgeben. |
| Alles muss raus | `twist_sale` | Karten kosten ![1 Geld](Spielzeichen/geld-1.png) weniger. |
| Vorsprung | `twist_head_start` | In der Kampagne erhältst du zu Beginn 2 Extrazüge. Die KI erhält 10 Siegpunktmarker. |
| Gespiegelte Kostbarkeiten | `twist_mirror_loot` | Der Kostbarkeiten-Stapel besteht aus Exemplaren derselben Kostbarkeit. |
| Platz für Mehr | `twist_room_for_more(Stapel-ID)` | Legt beim Spielaufbau den gewählten Königreichkarten-Stapel in den Vorrat. Die [Übernahme aus einem eigenen Set](#zwölf-oder-mehr-königreichstapel) ist offen. |
| Rotationskoller | `twist_turn_turn_turn` | Zu Beginn jedes menschlichen Zuges nach dem ersten wandert bei jedem rotierbaren Vorratsstapel die oberste Gruppe gleicher Karten unter den Stapel. |
| Taifun | `twist_typhoon(Stapel-ID,Stapel-ID,…)` | Zu Beginn der menschlichen Züge wird ein Königreichkarten-Stapel im Vorrat durch einen anderen ersetzt. Die Liste in Klammern legt die möglichen Ersatzstapel fest und fasst bis zu 20 Stapel-IDs. |

#### Alle 34 Stempel mit `stamp_`

| Name im Spiel | Befehl | Wirkung |
| --- | --- | --- |
| Plus Aktion | `stamp_plus_action(Karten-ID)` | Wenn du eine Karte von diesem Stapel spielst, **+1 Aktion**. |
| Plus Karte | `stamp_plus_card(Karten-ID)` | Wenn du eine Karte von diesem Stapel spielst, **+1 Karte**. |
| Plus Kauf | `stamp_plus_buy(Karten-ID)` | Wenn du eine Karte von diesem Stapel spielst, **+1 Kauf**. |
| Plus Geld | `stamp_plus_coin(Karten-ID)` | Wenn du eine Karte von diesem Stapel spielst, +![1 Geld](Spielzeichen/geld-1.png). |
| Chamäleon | `stamp_chameleonize(Karten-ID)` | Beim Ausspielen werden **+Karten** und Geldboni in gleicher Höhe getauscht. Aus **+2 Karten** wird zum Beispiel +![2 Geld](Spielzeichen/geld-2.png) und umgekehrt. |
| Vergünstigt | `stamp_discounted(Karten-ID)` | Die Karte kostet ![1 Geld](Spielzeichen/geld-1.png) weniger. |
| Später Zahlbar | `stamp_pay_later(Karten-ID)` | Der Geldpreis der Karte wird in gleich viele Schulden umgewandelt. |
| 2 zum Preis von 1 | `stamp_two_for_one(Karten-ID)` | Die Karte kostet ![1 Geld](Spielzeichen/geld-1.png) mehr. Wenn du sie nimmst, nimm zusätzlich eine Nicht-Vorrats-Kopie. |
| Einblick Bietend | `stamp_offers_insight(Karten-ID)` | Wenn du diese Karte nimmst, **+1 Karte** am Ende des Zuges. |
| Pferd Dazu | `stamp_with_horse(Karten-ID)` | Wenn du diese Karte nimmst, nimm ein Pferd. |
| Beute Dazu | `stamp_with_spoils(Karten-ID)` | Wenn du diese Karte nimmst, nimm eine Beute (`spoils`). |
| Dorfbewohner Dazu | `stamp_with_villager(Karten-ID)` | Wenn du diese Karte nimmst, **+1 Dorfbewohner**. |
| Zusatztyp Dauerkarte | `stamp_durationified(Karten-ID)` | Die Karte wird zusätzlich zur Dauerkarte. Wenn du sie spielst, gibt sie dir zu Beginn deines nächsten Zuges **+1 Karte** und **+1 Aktion**. |
| Zusatztyp Geldkarte | `stamp_treasurified(Karten-ID)` | Die Karte hat zusätzlich den Typ Geld. Du kannst sie dadurch auch als Geldkarte in der Kaufphase spielen. |
| Zusatztyp Punktekarte | `stamp_victorified(Karten-ID)` | Die Karte wird zusätzlich zur Punktekarte und ist ![1 Siegpunkt](Spielzeichen/siegpunkt-1.png) wert. |
| Zusatztyp Reaktion | `stamp_reactionified(Karten-ID)` | Die Karte wird zusätzlich zur Reaktion. Du darfst sie spielen, wenn du sie außerhalb der Aufräumphase nimmst, entsorgst oder ablegst. |
| Zusatztyp Schatten | `stamp_shadowified(Karten-ID)` | Die Karte wird zusätzlich zur Schattenkarte. Du kannst sie aus deinem Nachziehstapel spielen, als wäre sie in deiner Hand. Wenn sie eingemischt wird, kommt sie unter den Nachziehstapel. |
| −2 Kosten | `stamp_cost_2_less(Ereignis-ID)` | Das Ereignis kostet ![2 Geld](Spielzeichen/geld-2.png) weniger. |
| −2 Schuldenkosten | `stamp_cost_2D_less(Ereignis-ID)` | Das Ereignis kostet 2 Schulden weniger. |
| Kostenlos | `stamp_cost_zero(Ereignis-ID)` | Das Ereignis kostet ![0 Geld](Spielzeichen/geld-0.png). |
| Extrageld | `stamp_bonus_money(Ereignis-ID)` | Wenn du das Ereignis kaufst, +![1 Geld](Spielzeichen/geld-1.png). |
| Silber Dazu | `stamp_also_silver(Ereignis-ID)` | Wenn du das Ereignis kaufst, nimm ein Silber. |
| Neue Impulse | `stamp_motivational(Ereignis-ID)` | Wenn du das Ereignis kaufst, **+2 Karten** am Ende des Zuges. |
| Hab's Bereits | `stamp_got_it_already(Ereignis-ID)` | Das Ereignis wird in deinem ersten Zug automatisch und kostenlos gekauft. |
| Tunnelblick | `stamp_one_track_mind(Ereignis-ID)` | Das Ereignis wird zu Beginn jeder Kaufphase automatisch und kostenlos gekauft. |
| Huldigung | `stamp_fawning_service(Ereignis-ID)` | Wenn du eine Provinz nimmst, wird das Ereignis automatisch und kostenlos gekauft. |
| Gold mit Vorzügen | `stamp_perks_of_wealth(Ereignis-ID)` | Wenn du ein Gold nimmst, führe das Ereignis aus. |
| 2x SP | `stamp_double_vp(Landmarken-ID)` | Die Landmarke vergibt doppelt so viele Siegpunkte. |
| Plus Aktion | `stamp_action_way(Weg-ID)` | Wenn du eine Karte nach den Anweisungen dieses Wegs spielst, **+1 Aktion**. |
| Plus Geld | `stamp_coin_way(Weg-ID)` | Wenn du eine Karte nach den Anweisungen dieses Wegs spielst, +![1 Geld](Spielzeichen/geld-1.png). |
| Platzregen | `stamp_also_debt(Prophezeiungs-ID)` | Wenn die Prophezeiung eintritt, erhält jeder Spieler 5 Schulden. |
| Geldregen | `stamp_also_gold(Prophezeiungs-ID)` | Wenn die Prophezeiung eintritt, nimmt jeder Spieler 3 Gold außerhalb des Vorrats. |
| Wie eine Plage | `stamp_also_plague(Prophezeiungs-ID)` | Wenn die Prophezeiung eintritt, nimmt jeder Spieler 3 Flüche außerhalb des Vorrats. |
| Punktebonus | `stamp_also_respect(Prophezeiungs-ID)` | Wenn die Prophezeiung eintritt, erhält jeder Spieler mit positivem Punktestand so viele Siegpunktmarker, wie er zu diesem Zeitpunkt Siegpunkte hat. |

#### Alle 28 Änderungen beim Spielaufbau mit `start_`

| Was sich ändert | Befehl | Wirkung und Anwendung |
| --- | --- | --- |
| Gefallen | `start_10_favor` | Jeder Spieler startet mit 10 Gefallen. |
| Dorfbewohner | `start_10_villagers` | Jeder Spieler startet mit 10 Dorfbewohnern. |
| Taler | `start_5_coffers` | Jeder Spieler startet mit 5 Talern. |
| Zusätzliche Startkarte | `start_add_card(Karten-ID)` | Jeder Spieler startet mit einer Kopie dieser Karte zusätzlich im Startdeck. |
| Eine Startkarte von jedem Stapel | `start_add_each_pile` | Jeder Spieler startet mit einer Kopie von jedem Königreichkarten-Stapel zusätzlich im Startdeck. |
| Zweiter Stein gefällig? | `start_allow_second_cube(Projekt-ID)` | Erlaubt einen zweiten Projektstein auf dem gewählten Projekt und verdoppelt damit dessen Effekt. |
| Kosten der Flüche | `start_curse_cost_3` | Flüche kosten ![3 Geld](Spielzeichen/geld-3.png). |
| Doppeltes Startdeck | `start_double_deck` | Jeder Spieler startet mit doppelt so vielen Exemplaren jeder Karte seines Startdecks. |
| Doppeltes Erbstück | `start_double_heirloom(Karten-ID)` | Zwei Exemplare des Erbstücks ersetzen bei jedem Spieler zwei Startkupfer. Gib die zugehörige Königreichkarte an, etwa `start_double_heirloom(secret_cave)` für Geheime Höhle. |
| Doppelter Vorratsstapel | `start_double_pile(Stapel-ID)` | Verdoppelt den Inhalt des Vorratsstapels. `start_double_pile(village)` wurde mit Dorf im Spiel bestätigt. |
| Doppelte Siegpunktmarker | `start_double_vp_tokens(Landmarken-ID)` | Legt pro Spieler 6 zusätzliche Siegpunktmarker auf die gewählte Landmarke. Bei Landmarken mit regulär 6 Markern pro Spieler verdoppelt sich dadurch die Startmenge. |
| Karte bereits im Spiel | `start_in_play(Karten-ID)` | Jeder Spieler startet mit der gewählten Dauerkarte bereits im Spiel. Die Plünderer-Kampagne nutzt `start_in_play(search)` für Suche. |
| Verdeckter Reisemarker | `start_journey_facedown` | Jeder Spieler beginnt mit verdecktem Reisemarker. |
| Gemischter Stapel | `start_make_split_pile(Stapel-ID,Karten-ID)` | Ersetzt die untere Hälfte eines vorhandenen Stapels mit mindestens 10 Karten durch die zweite Karte. Bei einem Zehnerstapel ersetzt `start_make_split_pile(vassal,groundskeeper)` die unteren 5 Vasallen durch Gärtnerinnen. |
| Eingeborenendorf | `start_native_village(Karten-ID)` | Jeder Spieler startet mit der gewählten Karte auf seinem Eingeborenendorf-Tableau. `start_native_village(silver)` legt dort ein Silber ab. |
| Zusätzliche Siegpunktmarker auf Aktionsstapeln | `start_plus_2_vp_tokens(Landmarken-ID)` | Legt beim Aufbau je 2 zusätzliche Siegpunktmarker auf die Aktionsstapel im Vorrat, ausgenommen Stapel vom Typ Sammlung. Die passende Ziel-ID für Entweihter Schrein ist `defiled_shrine`. |
| Zusätzliche Siegpunktmarker auf Geldstapeln | `start_plus_8_vp_tokens(Landmarken-ID)` | Legt beim Aufbau je 8 zusätzliche Siegpunktmarker auf die Geldstapel im Vorrat. Die passende Ziel-ID für Aquädukt ist `aqueduct`. |
| Startkupfer austauschen | `start_replace_copper(Karten-ID)` | Ersetzt pro Spieler ein Startkupfer durch eine neu erzeugte Kopie der gewählten Karte. |
| Startkupfer durch eine Karte vom Stapel ersetzen | `start_replace_copper_from_pile(Stapel-ID)` | Nimmt pro Spieler eine Karte vom gewählten Stapel und legt sie anstelle eines Kupfers ins Startdeck. |
| Startanwesen austauschen | `start_replace_estate(Karten-ID)` | Ersetzt pro Spieler ein Startanwesen durch eine neu erzeugte Kopie der gewählten Karte, etwa mit `start_replace_estate(silver)`. |
| Startanwesen durch eine Karte vom Stapel ersetzen | `start_replace_estate_from_pile(Stapel-ID)` | Nimmt pro Spieler eine Karte vom gewählten Stapel und legt sie anstelle eines Anwesens ins Startdeck. Die Menagerie-Kampagne nutzt `start_replace_estate_from_pile(horse)` dreimal, damit jeder Spieler mit 3 Pferden startet. |
| Gleiche Belohnungen | `start_same_reward` | Das Spiel wählt eine Belohnung zufällig aus und macht alle anderen Belohnungen zu Kopien davon. Die Vorlage mit Ritterkampf (`joust`) verwendet den Befehl ohne Klammern. |
| Stapel mischen | `start_shuffle_pile(Stapel-ID)` | Mischt den gewählten gemischten Vorratsstapel beim Aufbau, etwa `start_shuffle_pile(patrician)` für den Stapel mit Patrizier und Handelsplatz. |
| Wirtshaus | `start_tavern(Karten-ID)` | Jeder Spieler startet mit der gewählten Karte auf seinem Wirtshaus-Tableau. Eine Kampagnenvorlage nutzt zweimal `start_tavern(copper)`. |
| Flüche im Müll | `start_trash_curses` | Erzeugt beim Aufbau 10 Flüche direkt im Müll. |
| Stapel im Müll | `start_trash_pile(Karten-ID)` | Erzeugt beim Aufbau einen Stapel der gewählten Karte direkt im Müll. |
| Dreifacher Vorratsstapel | `start_triple_pile(Stapel-ID)` | Verdreifacht den Inhalt des gewählten Vorratsstapels. |
| Projektstein | `start_with_cube(Projekt-ID)` | Jeder Spieler startet mit einem Projektstein auf dem gewählten Projekt. Die Kampagne nutzt etwa `start_with_cube(pageant)` für Festzug. |

#### Weitere Varianten aus der Kampagnenerzeugung

Für die drei Eigenschaftsvarianten ist die Übernahme in eigene Sets offen. [Quellenregister zum lokalen Build](Sources/Source_Register.md#reproduktion-für-l01)

| Kampagneneintrag | Wirkung und Anwendung |
| --- | --- |
| `landmod_trait_affects_2_piles` | Weist dieselbe Eigenschaft beim Erzeugen der Kampagnenpartie zwei geeigneten Königreichkarten-Stapeln zu. |
| `landmod_trait_affects_gold` | Weist eine Eigenschaft beim Erzeugen der Kampagnenpartie dem Goldstapel zu. |
| `landmod_trait_affects_all_piles` | Wendet dieselbe Eigenschaft in der Kampagnenpartie auf alle dafür geeigneten Stapel an. |
| `landmod_ally_favored_landlords` | Kombiniert einen Verbündeten mit **Grundbesitzer Verbündet**. Für eine eigene Auswahl den Verbündeten hinter `-k` und `twist_favored_landlords` hinter `-x` angeben. |
| `landmod_ally_favored_wealthy` | Kombiniert einen Verbündeten mit **Reich Verbündet**. Für eine eigene Auswahl den Verbündeten hinter `-k` und `twist_favored_wealthy` hinter `-x` angeben. |
| `landmod_ally_forever_favored` | Kombiniert einen Verbündeten mit **Für immer Verbündet**. Für eine eigene Auswahl den Verbündeten hinter `-k` und `twist_forever_favored` hinter `-x` angeben. |

Eine Kampagnenvorlage weist dem Einsamen Schrein sowohl **Inspirierend** als auch **Waghalsig** zu. Beide Eigenschaften stehen hinter `-k`, jeweils mit derselben Stapel-ID in Klammern. Die Schreibweise lautet `inspiring(secluded_shrine), reckless(secluded_shrine)`. [Technische Referenz zur Kingdom-Syntax](Modding_Overview.md#bestandteile-des-kingdom-texts)

## Dominion on Steam without mods

### Save and share a Kingdom

1. Open the Kingdom selection in a custom game and choose the piles.
2. During the game, right-click the castle icon in the lower-right corner. Save the Kingdom as a set or copy it to the clipboard.
3. Load a saved set from the Kingdom selection or paste the copied text into its card search field. [Clipboard and game test](Kingdom_Sets.md#zwischenablage)

For a random slot from a specific expansion, search for the expansion by name and choose its random entry. [Community FAQ](https://www.reddit.com/r/dominion/comments/1ahhoec/dominion_app_faq/)

### Enter campaign effects

#### A Village pile with 20 cards

Paste this line into the card search field of a new local game. In the build 3315 game test, the Village pile started with 20 cards.

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(village)
```

List card and landscape IDs after `-k`. After `-x`, you can replace the example command with an effect from the catalog. Replace placeholders such as `card-ID` with the target ID. Cards and landscapes targeted by stamps must be present in the game. Separate multiple effects with commas in a single `-x` section. [German card names and IDs](Card_Titles_de.json)

The search field accepts each effect command once, even when the targets differ. Use a saved set for multiple `start_double_pile` entries. [Search field findings](Modding_Overview.md#warum-das-suchfeld-nur-einen-stapel-verdoppelt)

#### Double multiple piles with a saved set

Prepare a set with the pile-doubling effects you want. Use up to **32 effects** per list. Build 3315 aborts loading at the 33rd entry. [Prepare an effect set](../SetGenerator/README.md#html-konverter-mit-doppelten-stapeln), [Effect limit findings](Modding_Overview.md#effektsets-und-zufallsauswahl)

1. Load the effect set and click **Play**.
2. Resign and unload the set in the Kingdom selection. Leave the extra rules in place. Their indicator stays visible.
3. Reload the same set or select a separate set with the Kingdom you want. Click **Play**. [Steps verified in a local game](Modding_Overview.md#warum-die-effekte-beim-set-wechsel-hinterherhinken)

#### Random Kingdoms with doubled piles

A set containing only `-x` lets the game choose a random Kingdom. If the entire pool fits in the list, use one entry from the reference list for each possible target. For larger pools, draw the ten piles first and create the effect set for that selection. The effect doubles piles that are present at setup. [Reference list](Start_Double_Pile_Karten.txt), [Game test findings](Modding_Overview.md#effektsets-und-zufallsauswahl)

#### Twelve or more Kingdom piles

In the local test, the saved set with `twist_room_for_more(chapel)` still had ten piles. The eighth Cornucopia campaign mission calls for twelve Kingdom piles in the Supply. Alongside the ten main piles, it adds Harbinger as Young Witch's Bane and Gardens through Room for More. [Game test and campaign template](Modding_Overview.md#zusatzstapel-durch-platz-für-mehr)

### Campaign effect catalog

The tables cover 89 commands and six campaign generation variants in game build **3315** (Steam build **17683350**). Names and rules terms come from the English game text. The descriptions summarize the campaign effects. The two tested pile-doubling effects were absent when the private online game started. [Online test findings](Modding_Overview.md#kampagneneffekte-im-privaten-online-spiel), [Technical effect catalog](Modding_Overview.md#belegstand-des-effektkatalogs), [Local game text](Sources/Source_Register.md#deutsche-kartentitel-aus-l05)

+![1 coin](Spielzeichen/geld-1.png) adds 1 coin to your spending money for this turn. You can spend a Coffers token on a later turn.

#### All 27 twists with `twist_`

A **non-Supply copy** is an extra copy of the card that you gain from outside the Supply.

| In-game name | Command | Effect |
| --- | --- | --- |
| Big Ideas | `twist_big_ideas` | At the start of your turn, **+1 Card**. |
| Busy Busy | `twist_busy_busy` | At the start of your turn, **+1 Action**. |
| Extra Oomph | `twist_extra_oomph` | At the start of your turn, +![1 coin](Spielzeichen/geld-1.png). |
| Forever Favored | `twist_forever_favored` | At the start of your turn, **+1 Favor**. |
| Garbage Heap | `twist_garbage_heap` | At the start of your turn, you may trash a card from your hand. |
| Penny Party | `twist_penny_party` | Gain a Copper at the start of your turn. |
| Shoppers | `twist_shoppers` | At the start of your turn, **+1 Buy**. |
| Cramped | `twist_cramped` | Your regular hand draw is 4 cards instead of 5. |
| From the Throne | `twist_from_the_throne` | The first Action card you play each turn is automatically played twice. |
| Landfill | `twist_landfill` | When you trash one of your cards, **+1 Card**. This also applies when you trash the card from somewhere other than your hand. |
| So much Stuff | `twist_so_much_stuff` | When you play your first card in a turn, gain a non-Supply copy of it. |
| Double Up | `twist_double_up` | The first time you gain an Action or Treasure card in a turn, also gain a non-Supply copy of it. |
| Fast Track | `twist_fast_track` | When you gain a Duration or Reserve card, set it aside and play it at the start of the upcoming Clean-up. |
| Favored Landlords | `twist_favored_landlords` | When you gain a Victory card, **+1 Favor**. |
| Favored Wealthy | `twist_favored_wealthy` | When you gain a Treasure card, **+1 Favor**. |
| Incoming | `twist_incoming` | When you gain an Action or Treasure card, put it straight onto your deck instead of your discard pile. |
| Rebates | `twist_rebates` | When you gain a card, +![1 coin](Spielzeichen/geld-1.png). |
| Beakers | `twist_beakers` | Unspent Potion resources carry over until you spend them. You can use them to pay on later turns, like Coffers. |
| Delayed Joy | `twist_delayed_joy` | You may save any Boon and resolve it on your next turn. |
| Pennies Saved | `twist_pennies_saved` | At the end of your Buy phase, convert unspent coins to Coffers. |
| Real Estate Barons | `twist_real_estate_barons` | You can spend VP tokens as Coffers. |
| Sale | `twist_sale` | Cards cost ![1 coin](Spielzeichen/geld-1.png) less. |
| Head Start | `twist_head_start` | In the campaign, you get 2 extra turns at the start. The AI gets 10 VP tokens. |
| Mirror Loot | `twist_mirror_loot` | The Loot pile consists of copies of the same Loot card. |
| Room for More | `twist_room_for_more(pile-ID)` | Adds the chosen Kingdom pile to the Supply at setup. [Loading it from a custom set](#twelve-or-more-kingdom-piles) remains unverified. |
| Turn Turn Turn | `twist_turn_turn_turn` | At the start of each human turn after the first, move the top group of matching cards to the bottom of each Supply pile that can rotate. |
| Typhoon | `twist_typhoon(pile-ID,pile-ID,…)` | At the start of human turns, the game replaces a Kingdom pile in the Supply. The list in parentheses specifies up to 20 possible replacement pile IDs. |

#### All 34 stamps with `stamp_`

| In-game name | Command | Effect |
| --- | --- | --- |
| Plus Action | `stamp_plus_action(card-ID)` | When you play a card from this pile, **+1 Action**. |
| Plus Card | `stamp_plus_card(card-ID)` | When you play a card from this pile, **+1 Card**. |
| Plus Buy | `stamp_plus_buy(card-ID)` | When you play a card from this pile, **+1 Buy**. |
| Plus Coin | `stamp_plus_coin(card-ID)` | When you play a card from this pile, +![1 coin](Spielzeichen/geld-1.png). |
| Chameleonized | `stamp_chameleonize(card-ID)` | When you play the card, swap **+Cards** and coin bonuses in equal amounts. **+2 Cards** becomes +![2 coins](Spielzeichen/geld-2.png), and vice versa. |
| Discounted | `stamp_discounted(card-ID)` | The card costs ![1 coin](Spielzeichen/geld-1.png) less. |
| Pay Later | `stamp_pay_later(card-ID)` | Convert the card's coin cost to an equal amount of Debt. |
| Two For One | `stamp_two_for_one(card-ID)` | The card costs ![1 coin](Spielzeichen/geld-1.png) more. When you gain it, also gain a non-Supply copy. |
| Offers Insight | `stamp_offers_insight(card-ID)` | When you gain this card, **+1 Card** at the end of the turn. |
| With Horse | `stamp_with_horse(card-ID)` | When you gain this card, gain a Horse. |
| With Spoils | `stamp_with_spoils(card-ID)` | When you gain this card, gain a Spoils (`spoils`). |
| With Villager | `stamp_with_villager(card-ID)` | When you gain this card, **+1 Villager**. |
| Durationified | `stamp_durationified(card-ID)` | This card is also a Duration. When you play it, it gives you **+1 Card** and **+1 Action** at the start of your next turn. |
| Treasurified | `stamp_treasurified(card-ID)` | This card is also a Treasure, so you can play it as a Treasure in your Buy phase. |
| Victorified | `stamp_victorified(card-ID)` | This card is also a Victory card and is worth ![1 victory point](Spielzeichen/siegpunkt-1.png). |
| Reactionified | `stamp_reactionified(card-ID)` | This card is also a Reaction. You may play it when you gain, trash, or discard it outside of Clean-up. |
| Shadowified | `stamp_shadowified(card-ID)` | This card is also a Shadow. You can play it from your deck as if it were in your hand. When you shuffle it, put it on the bottom of your deck. |
| -2 Cost | `stamp_cost_2_less(event-ID)` | The Event costs ![2 coins](Spielzeichen/geld-2.png) less. |
| -2 Debt Cost | `stamp_cost_2D_less(event-ID)` | The Event costs 2 less Debt. |
| Free | `stamp_cost_zero(event-ID)` | The Event costs ![0 coins](Spielzeichen/geld-0.png). |
| Bonus Money | `stamp_bonus_money(event-ID)` | When you buy this Event, +![1 coin](Spielzeichen/geld-1.png). |
| Also Silver | `stamp_also_silver(event-ID)` | When you buy this Event, gain a Silver. |
| Motivational | `stamp_motivational(event-ID)` | When you buy this Event, **+2 Cards** at the end of the turn. |
| Got It Already | `stamp_got_it_already(event-ID)` | You automatically buy this Event for free on your first turn. |
| One Track Mind | `stamp_one_track_mind(event-ID)` | You automatically buy this Event for free at the start of each Buy phase. |
| Fawning Service | `stamp_fawning_service(event-ID)` | When you gain a Province, you automatically buy this Event for free. |
| Perks of Wealth | `stamp_perks_of_wealth(event-ID)` | When you gain a Gold, resolve this Event. |
| 2x VP | `stamp_double_vp(landmark-ID)` | The Landmark awards twice as many victory points. |
| Plus Action | `stamp_action_way(way-ID)` | When you play a card using this Way, **+1 Action**. |
| Plus Coin | `stamp_coin_way(way-ID)` | When you play a card using this Way, +![1 coin](Spielzeichen/geld-1.png). |
| Also Debt | `stamp_also_debt(prophecy-ID)` | When the Prophecy comes true, each player gets 5 Debt. |
| Also Gold | `stamp_also_gold(prophecy-ID)` | When the Prophecy comes true, each player gains 3 Gold from outside the Supply. |
| Also Plague | `stamp_also_plague(prophecy-ID)` | When the Prophecy comes true, each player gains 3 Curses from outside the Supply. |
| Also Respect | `stamp_also_respect(prophecy-ID)` | When the Prophecy comes true, each player with a positive score gets as many VP tokens as they currently have victory points. |

#### All 28 setup changes with `start_`

| What changes | Command | Effect and use |
| --- | --- | --- |
| Favors | `start_10_favor` | Each player starts with 10 Favors. |
| Villagers | `start_10_villagers` | Each player starts with 10 Villagers. |
| Coffers | `start_5_coffers` | Each player starts with 5 Coffers. |
| Extra starting card | `start_add_card(card-ID)` | Each player starts with an extra copy of this card in their starting deck. |
| One starting card from each pile | `start_add_each_pile` | Each player starts with an extra copy of a card from each Kingdom pile in their starting deck. |
| Second Cube | `start_allow_second_cube(project-ID)` | Allows a second cube on the chosen Project, doubling its effect. |
| Cost of Curses | `start_curse_cost_3` | Curses cost ![3 coins](Spielzeichen/geld-3.png). |
| Double starting deck | `start_double_deck` | Each player starts with twice as many copies of every card in their starting deck. |
| Double Heirloom | `start_double_heirloom(card-ID)` | Two copies of the Heirloom replace two starting Coppers per player. Use the associated Kingdom card ID, as in `start_double_heirloom(secret_cave)` for Secret Cave. |
| Double Supply pile | `start_double_pile(pile-ID)` | Doubles the number of cards in the Supply pile. `start_double_pile(village)` was verified with Village in a game test. |
| Double VP tokens | `start_double_vp_tokens(landmark-ID)` | Adds 6 VP tokens per player to the chosen Landmark. This doubles the starting amount for Landmarks that normally have 6 tokens per player. |
| Card already in play | `start_in_play(card-ID)` | Each player starts with the chosen Duration card already in play. The Plunder campaign uses `start_in_play(search)` for Search. |
| Journey token face down | `start_journey_facedown` | Each player starts with their Journey token face down. |
| Split pile | `start_make_split_pile(pile-ID,card-ID)` | Replaces the bottom half of an existing pile of at least 10 cards with the second card. In a ten-card pile, `start_make_split_pile(vassal,groundskeeper)` replaces the bottom 5 Vassals with Groundskeepers. |
| Native Village | `start_native_village(card-ID)` | Each player starts with the chosen card on their Native Village mat. `start_native_village(silver)` puts a Silver there. |
| Extra VP tokens on Action piles | `start_plus_2_vp_tokens(landmark-ID)` | At setup, adds 2 VP tokens to each Action pile in the Supply, excluding Gathering piles. The target ID for Defiled Shrine is `defiled_shrine`. |
| Extra VP tokens on Treasure piles | `start_plus_8_vp_tokens(landmark-ID)` | At setup, adds 8 VP tokens to each Treasure pile in the Supply. The target ID for Aqueduct is `aqueduct`. |
| Replace a starting Copper | `start_replace_copper(card-ID)` | Replaces one starting Copper per player with a newly created copy of the chosen card. |
| Replace a starting Copper with a card from a pile | `start_replace_copper_from_pile(pile-ID)` | Takes one card per player from the chosen pile and puts it in their starting deck in place of a Copper. |
| Replace a starting Estate | `start_replace_estate(card-ID)` | Replaces one starting Estate per player with a newly created copy of the chosen card, as in `start_replace_estate(silver)`. |
| Replace a starting Estate with a card from a pile | `start_replace_estate_from_pile(pile-ID)` | Takes one card per player from the chosen pile and puts it in their starting deck in place of an Estate. The Menagerie campaign uses `start_replace_estate_from_pile(horse)` three times so each player starts with 3 Horses. |
| Identical Rewards | `start_same_reward` | The game picks a Reward at random and makes all the other Rewards copies of it. The Joust (`joust`) template uses the command without parentheses. |
| Shuffle a pile | `start_shuffle_pile(pile-ID)` | Shuffles the chosen split Supply pile at setup. For example, `start_shuffle_pile(patrician)` shuffles the Patrician and Emporium pile. |
| Tavern | `start_tavern(card-ID)` | Each player starts with the chosen card on their Tavern mat. One campaign template uses `start_tavern(copper)` twice. |
| Curses in the trash | `start_trash_curses` | Creates 10 Curses directly in the trash at setup. |
| Pile in the trash | `start_trash_pile(card-ID)` | Creates a pile of the chosen card directly in the trash at setup. |
| Triple Supply pile | `start_triple_pile(pile-ID)` | Triples the number of cards in the chosen Supply pile. |
| Project cube | `start_with_cube(project-ID)` | Each player starts with a cube on the chosen Project. One campaign example is `start_with_cube(pageant)` for Pageant. |

#### Other campaign generation variants

Use of the three Trait variants in custom sets remains unverified. [Local build source register](Sources/Source_Register.md#reproduktion-für-l01)

| Campaign entry | Effect and use |
| --- | --- |
| `landmod_trait_affects_2_piles` | Assigns the same Trait to two eligible Kingdom piles when generating the campaign game. |
| `landmod_trait_affects_gold` | Assigns a Trait to the Gold pile when generating the campaign game. |
| `landmod_trait_affects_all_piles` | Applies the same Trait to every eligible pile in the campaign game. |
| `landmod_ally_favored_landlords` | Combines an Ally with **Favored Landlords**. For a custom selection, put the Ally after `-k` and `twist_favored_landlords` after `-x`. |
| `landmod_ally_favored_wealthy` | Combines an Ally with **Favored Wealthy**. For a custom selection, put the Ally after `-k` and `twist_favored_wealthy` after `-x`. |
| `landmod_ally_forever_favored` | Combines an Ally with **Forever Favored**. For a custom selection, put the Ally after `-k` and `twist_forever_favored` after `-x`. |

One campaign template assigns both **Inspiring** and **Reckless** to Secluded Shrine. Put both Traits after `-k`, each with the same pile ID in parentheses. Use `inspiring(secluded_shrine), reckless(secluded_shrine)`. [Technical reference for Kingdom syntax](Modding_Overview.md#bestandteile-des-kingdom-texts)
