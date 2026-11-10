# Modding-Überblick

Die erste Untersuchung gilt für den installierten Steam-Build von Dominion von Temple Gates Games. Dateipfade, EXE-Fingerabdruck und Quellen stehen im [Quellenregister](Sources/Source_Register.md).

| Ansatz | Befund | Ausstehender Nachweis |
| --- | --- | --- |
| Eigene Kingdoms und externer Generator | **Lokal beobachtet und statisch geprüft.** Die Profildatei enthält benannte Kingdoms. Das Spiel berechnet beim Speichern die Erweiterungsmaske. Der Zwischenablageweg ist in der Community beschrieben. | Ein extern ergänztes Set offline laden und spielen. Exporttext in eine neue Spielkonfiguration einfügen und die Auswahl prüfen. [Kingdom-Befund](Kingdom_Sets.md) |
| Varianten mit vorhandenen Karten | **Statisch geprüft.** Die EXE enthält Kampagnen-Setups mit `-k` und `-x`, etwa für den Tausch einer Karte im Startdeck. **Von der Community berichtet.** Solche Kampagnenparameter lassen sich auch im normalen Kingdom-Setup einsetzen. | Die gewünschten Parameter einzeln im Zielbuild und Spielmodus prüfen. [Quellen L01 und W04](Sources/Source_Register.md) |
| Grafik und UI | **Lokal beobachtet.** `Dominion.xpack` beginnt mit `KCPX`. Im Paketindex liegen unter anderem Pfade zu UI-Layouts, Texturen und Kampagnenressourcen. | Containerformat und Ressourcenauflösung prüfen. Für einen Austausch zunächst mit einer separaten Kopie ermitteln, wie der Loader Pakete oder lose Dateien priorisiert. [Quelle L05](Sources/Source_Register.md) |
| Eingebaute Zufallsauswahl und Spiellogik | **Statisch geprüft.** `Dominion.exe` ist eine native Windows-x86-EXE ohne CLR-Header. Sie enthält Diagnosestrings zu `RollKingdom_AddRandomCards` und verwandten Pfaden. Native Patches oder Laufzeithooks wären für Änderungen an dieser Logik ein möglicher Ansatz. | Den konkreten Aufrufpfad, die Datenstrukturen und die Auswirkung im vorgesehenen Spielmodus pro Build prüfen. [Quelle L01](Sources/Source_Register.md) |

Für den [SetGenerator](../SetGenerator/README.md) sind die relevanten Schnittstellen der Kingdom-Text und gegebenenfalls ein zusätzlicher Profileintrag. Eine offizielle Schnittstelle für eigene Spielregeln, neue Karten oder den Austausch von Ressourcen ist in den bisher ausgewerteten Quellen nicht belegt.

## Kingdom-Text und Kampagneneffekte

**Untersuchte Version.** Dominion von Temple Gates Games, Windows-Spielbuild **3315**, Steam-Build **17683350**. `Dominion.exe` hat den SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Bedienung und kopierfertige Testzeilen stehen in der [Steam-Spielanleitung](Steam_Spielanleitung.md). [Quellenregister](Sources/Source_Register.md)

### Bestandteile des Kingdom-Texts

| Form | Bedeutung und Beleg |
| --- | --- |
| `-k …` | Kommagetrennte Auswahl aus internen IDs für Königreichstapel, Landschaften und Setup-Optionen wie `colonies` und `shelters`. Im Profil und in Kampagnenvorlagen beobachtet. Das Einfügen eines normalen `-k`-Texts wurde vom Nutzer bestätigt. |
| `-m Landscapes(0,4)` | In einem lokal gespeicherten Set beobachtet. Die genaue Interpretation der beiden Werte und der übrigen `-m`-Optionen bleibt offen. |
| `-x effekt(ziel), weiterer_effekt` | Zusatzregeln hinter der Kartenauswahl. Diese Schreibweise steht in den eingebauten Kampagnenvorlagen. Ein einzelner Effekt wurde über das normale Kartensuchfeld erfolgreich importiert. Beim Test mit zehn durch Kommas getrennten Effekten wurde nur der erste übernommen. Die EXE trennt und serialisiert die Zusätze als eigenen Teil der Spielkonfiguration. |
| `hauptkarte(ziel)` im `-k`-Teil | Ordnet einer Karte oder Landschaft das konkrete Ziel ihrer Setup-Regel zu. Eingebaute Vorlagen enthalten `young_witch(village)` für den Bannstapel, `ferryman(shop)` für den Stapel neben dem Vorrat, `obelisk(patrician)` für den gewählten Aktionsstapel und `riverboat(imperial_envoy)` für die beiseitegelegte Karte. |

Weitere Formen für den normalen Suchdialog sind **von der Community berichtet**. Der Name einer Erweiterung belegt einen zufälligen Königreichplatz aus genau dieser Erweiterung. Ein Landschaftstyp wie Ereignis oder Projekt lässt sich ebenfalls für einen Zufallsplatz vorgeben. Für Speicherstruktur, IDs und Setup-Abhängigkeiten siehe [Kingdom-Sets](Kingdom_Sets.md), [deutsche Kartentitel](Card_Titles_de.json), [Karteneigenschaften](Card_Properties.json) und die [Community-FAQ](https://www.reddit.com/r/dominion/comments/1ahhoec/dominion_app_faq/).

### Belegstand des Effektkatalogs

Die Tabellen erfassen **89 registrierte Namen** aus den drei Namensgruppen in Build 3315. „Registriert“ bezeichnet einen Namen, dessen Adresse der Initialisierungscode in das vom Parser abgefragte Namensfeld schreibt. Ein Name an anderer Stelle in der EXE kann etwa zu einer Kampagnenvorlage oder einer UI-Übersetzung gehören. Registrierung belegt die Zuordnung des Namens im Parser. Gültige Zieltypen, die praktische Wirkung und die Übernahme aus dem Suchfeld sind davon getrennt zu prüfen.

Die Klammerargumente geben Zieltypen an. Im Kingdom-Text stehen darin konkrete interne IDs. Wirkungen und Zieltypen beruhen auf Kampagnenvorlagen, lokalisierten Effekttexten oder Community-Beschreibungen. Die explizit genannten Codepfade sind statisch geprüft. Ein bestätigter Spieltest wird mit Build, Modus, Eingabe und beobachtetem Ergebnis erfasst. Die Community-Nutzung in eigenen Kingdoms ist ein eigener Beleg. Für private Online-Partien bleibt die Übernahme offen. [Community-Syntax](https://www.reddit.com/r/dominion/comments/1pnmsph/custom_campaign_features_in_your_kingdoms_tgg/), [Kampagneneffekte im Dominion-Wiki](https://wiki.dominionstrategy.com/index.php/Campaigns_%28Temple_Gates_Games%29)

Die Spalten **Einmal** und **Mehrfach** beziehen sich auf gemessene lokale Partien in Build 3315. **Bis 3×** heißt, dass jede Kopie bis zur dritten eine zusätzliche Wirkung zeigte. **Gleiche Wirkung** heißt, dass zwei und drei Kopien denselben Messwert wie eine Kopie lieferten. **Offen** steht für eine ausstehende oder unvollständige Prüfung. [Messwerte](../KingdomEffectsPatch/TEST.md#automatisierte-lokale-effektprüfung-im-hintergrund)

#### Stempel für Karten und Landschaften

| Registrierte Schreibweise | Ziel und Wirkung | Einmal | Mehrfach |
| --- | --- | --- | --- |
| `stamp_action_way(Weg)` | Der Weg gibt beim Verwenden eine zusätzliche Aktion. | Offen | Offen |
| `stamp_coin_way(Weg)` | Der Weg gibt beim Verwenden eine zusätzliche Münze. | Offen | Offen |
| `stamp_also_debt(Prophezeiung)` | Beim Eintreten erhält jeder Spieler fünf Schulden. | Offen | Offen |
| `stamp_also_gold(Prophezeiung)` | Beim Eintreten erhält jeder Spieler drei Gold außerhalb des Vorrats. | Offen | Offen |
| `stamp_also_plague(Prophezeiung)` | Beim Eintreten erhält jeder Spieler drei Flüche außerhalb des Vorrats. | Offen | Offen |
| `stamp_also_respect(Prophezeiung)` | Beim Eintreten erhält jeder Spieler ebenso viele Siegpunktmarker, wie er bereits hat. | Offen | Offen |
| `stamp_also_silver(Ereignis)` | Beim Kauf erhält der Spieler ein Silber. | Ja | Gleiche Wirkung |
| `stamp_bonus_money(Ereignis)` | Beim Kauf erhält der Spieler eine Münze. | Ja | Bis 3× |
| `stamp_chameleonize(Karte)` | Tauscht die auf der Karte gewährten Münz- und Kartenziehboni gegeneinander. | Offen | Offen |
| `stamp_cost_2_less(Ereignis)` | Senkt die Münzkosten um zwei. | Offen | Offen |
| `stamp_cost_2D_less(Ereignis)` | Senkt die Schuldenkosten um zwei. | Offen | Offen |
| `stamp_cost_zero(Ereignis)` | Setzt die Ereigniskosten auf null. | Offen | Offen |
| `stamp_discounted(Karte)` | Senkt die Münzkosten um eins. | Offen | Offen |
| `stamp_double_vp(Landmarke)` | Verdoppelt die von der Landmarke vergebenen Siegpunkte. | Offen | Offen |
| `stamp_durationified(Karte)` | Macht sie zusätzlich zur Dauerkarte. Im nächsten Zug gibt sie zu Beginn eine Karte und eine Aktion. | Offen | Offen |
| `stamp_fawning_service(Ereignis)` | Beim Erhalten einer Provinz wird das Ereignis kostenlos gekauft. | Ja | Gleiche Wirkung |
| `stamp_got_it_already(Ereignis)` | Das Ereignis wird im ersten Zug kostenlos gekauft. | Ja | Bis 3× |
| `stamp_motivational(Ereignis)` | Nach dem Kauf werden am Zugende zwei Karten gezogen. | Offen | Offen |
| `stamp_offers_insight(Karte)` | Wer sie erhält, zieht am Zugende eine Karte. | Offen | Offen |
| `stamp_one_track_mind(Ereignis)` | Das Ereignis wird zu Beginn der Kaufphase kostenlos gekauft. | Offen | Offen |
| `stamp_pay_later(Karte)` | Wandelt Münzkosten in Schulden um. | Offen | Offen |
| `stamp_perks_of_wealth(Ereignis)` | Beim Erhalten von Gold wird das Ereignis ausgeführt. | Ja | Gleiche Wirkung |
| `stamp_plus_action(Karte)` | Beim Ausspielen gibt sie eine zusätzliche Aktion. | Offen | Offen |
| `stamp_plus_buy(Karte)` | Beim Ausspielen gibt sie einen zusätzlichen Kauf. | Offen | Offen |
| `stamp_plus_card(Karte)` | Beim Ausspielen wird eine zusätzliche Karte gezogen. | Offen | Offen |
| `stamp_plus_coin(Karte)` | Beim Ausspielen gibt sie eine zusätzliche Münze. | Offen | Offen |
| `stamp_reactionified(Karte)` | Macht sie zur Reaktion, die beim Erhalten, Entsorgen oder Ablegen außerhalb des Aufräumens ausgespielt werden kann. | Offen | Offen |
| `stamp_shadowified(Karte)` | Macht sie zum Schatten mit den zugehörigen Regeln fürs Ausspielen aus dem Nachziehstapel. | Offen | Offen |
| `stamp_treasurified(Karte)` | Gibt ihr zusätzlich den Typ Geld. | Offen | Offen |
| `stamp_two_for_one(Karte)` | Erhöht den Preis um eine Münze. Beim Erhalten kommt eine zusätzliche Kopie von außerhalb des Vorrats dazu. | Ja | Bis 3× |
| `stamp_victorified(Karte)` | Gibt ihr zusätzlich den Typ Sieg und den Wert von einem Siegpunkt. | Offen | Offen |
| `stamp_with_horse(Karte)` | Beim Erhalten bekommt der Spieler zusätzlich ein Pferd. | Ja | Bis 3× |
| `stamp_with_spoils(Karte)` | Beim Erhalten bekommt der Spieler zusätzlich Beute aus Dark Ages (`spoils`). | Ja | Bis 3× |
| `stamp_with_villager(Karte)` | Beim Erhalten bekommt der Spieler zusätzlich einen Dorfbewohner. | Ja | Bis 3× |

#### Spielvariationen für die Partie

| Registrierte Schreibweise | Ziel und Wirkung | Einmal | Mehrfach |
| --- | --- | --- | --- |
| `twist_beakers` | Nicht ausgegebene Tränke bleiben für spätere Züge erhalten. | Offen | Offen |
| `twist_big_ideas` | Zu Zugbeginn eine Karte mehr ziehen. | Ja | Bis 3× |
| `twist_busy_busy` | Zu Zugbeginn eine zusätzliche Aktion. | Ja | Bis 3× |
| `twist_cramped` | Die reguläre Handgröße beträgt vier Karten. | Ja | Bis 3× |
| `twist_delayed_joy` | Gaben können bis zum nächsten Zug aufbewahrt werden. | Offen | Offen |
| `twist_double_up` | Die erste erhaltene Aktions- oder Geldkarte des Zuges gibt eine zusätzliche Kopie von außerhalb des Vorrats. | Ja | Bis 3× |
| `twist_extra_oomph` | Zu Zugbeginn eine zusätzliche Münze. | Ja | Bis 3× |
| `twist_fast_track` | Erhaltene Dauer- oder Reservekarten werden beiseitegelegt und zu Beginn des nächsten Aufräumens ausgespielt. | Offen | Offen |
| `twist_favored_landlords` | Beim Erhalten einer Siegkarte gibt es einen Gefallen. | Ja | Bis 3× |
| `twist_favored_wealthy` | Beim Erhalten einer Geldkarte gibt es einen Gefallen. | Ja | Bis 3× |
| `twist_forever_favored` | Zu Zugbeginn einen Gefallen erhalten. | Ja | Bis 3× |
| `twist_from_the_throne` | Die erste Aktion des Zuges wird zweimal ausgespielt. | Offen | Offen |
| `twist_garbage_heap` | Zu Zugbeginn darf eine Handkarte entsorgt werden. | Offen | Offen |
| `twist_head_start` | In der Kampagne erhält der Mensch zwei frühe Extrazüge und die KI zehn Siegpunktmarker. | Offen | Offen |
| `twist_incoming` | Erhaltene Aktions- und Geldkarten kommen auf den Nachziehstapel. | Ja | Gleiche Wirkung |
| `twist_landfill` | Beim Entsorgen einer eigenen Karte wird eine Karte gezogen. | Offen | Offen |
| `twist_mirror_loot` | Alle Karten des Loot-Stapels sind gleich. | Ja | Gleiche Wirkung |
| `twist_pennies_saved` | Übrige Münzen werden am Ende der Kaufphase in Taler umgewandelt. | Offen | Offen |
| `twist_penny_party` | Zu Zugbeginn ein Kupfer erhalten. | Ja | Bis 3× |
| `twist_real_estate_barons` | Siegpunktmarker können wie Taler ausgegeben werden. | Offen | Offen |
| `twist_rebates` | Beim Erhalten einer Karte gibt es eine Münze. | Ja | Bis 3× |
| `twist_room_for_more(Königreichstapel)` | Fügt dem Vorrat einen weiteren Königreichstapel hinzu. | Ja | Einmal im F8-Editor |
| `twist_sale` | Karten kosten eine Münze weniger. | Offen | Offen |
| `twist_shoppers` | Zu Zugbeginn ein zusätzlicher Kauf. Name und Beschreibung sind lokal belegt. | Ja | Bis 3× |
| `twist_so_much_stuff` | Die erste ausgespielte Karte des Zuges gibt eine Kopie von außerhalb des Vorrats. | Offen | Offen |
| `twist_turn_turn_turn` | In der Kampagne rotieren geteilte Stapel zu Beginn der menschlichen Züge ab dem zweiten. | Offen | Offen |
| `twist_typhoon(Karte, …)` | Ersetzt in der Kampagne nach und nach Königreichstapel. Eine Zielliste steht in eingebauten Vorlagen. | Offen | Offen |

#### Änderungen beim Spielaufbau

| Registrierte Schreibweise | Ziel und Wirkung | Einmal | Mehrfach |
| --- | --- | --- | --- |
| `start_10_favor` | Jeder Spieler startet mit zehn Gefallen. | Ja | Bis 3× |
| `start_10_villagers` | Jeder Spieler startet mit zehn Dorfbewohnern. | Ja | Bis 3× |
| `start_5_coffers` | Jeder Spieler startet mit fünf Talern. | Ja | Bis 3× |
| `start_add_card(Karte)` | Jeder Spieler erhält eine Kopie der Karte ins Startdeck. Der Code schreibt in den Kartenbereich für Startkarten. | Ja | Bis 3× |
| `start_add_each_pile` | Jeder Spieler erhält eine Kopie von jedem Königreichstapel ins Startdeck. Der Code schreibt in den Kartenbereich für Startkarten. | Ja | Bis 3× |
| `start_allow_second_cube(Projekt)` | Erlaubt einen zweiten Marker auf dem Projekt. | Offen | Offen |
| `start_curse_cost_3` | Flüche kosten drei Münzen. | Offen | Offen |
| `start_double_deck` | Verdoppelt das Startdeck. | Ja | Bis 3× |
| `start_double_heirloom(Karte)` | Für eine Königreichkarte mit Erbstück ersetzen zwei Exemplare des Erbstücks Startkupfer. | Ja | Bis 3× |
| `start_double_pile(Stapel)` | Verdoppelt den gewählten Vorratsstapel. Der Codepfad und ein einzelner Import sind geprüft. | Ja | Bis 3× |
| `start_double_vp_tokens(Landmarke)` | Verdoppelt die Startmenge an Siegpunktmarkern der gewählten Landmarke. | Ja | Bis 3× |
| `start_in_play(Dauerkarte)` | Die gewählte Dauerkarte liegt bei Spielbeginn bereits im Spiel. Die Community nennt nur bestimmte geeignete Ziele. | Ja | Bis 3× |
| `start_journey_facedown` | Der Reisemarker beginnt verdeckt. | Ja | Wechselt pro Kopie |
| `start_make_split_pile(Stapel, Karte)` | Ersetzt die untersten fünf Karten des ersten Stapels durch fünf der zweiten Karte. | Ja | Gleiche Wirkung |
| `start_native_village(Karte)` | Legt eine Kopie auf das Tableau von Eingeborenendorf. Die EXE enthält ein Beispiel mit `silver`. | Ja | Bis 3× |
| `start_plus_2_vp_tokens(Landmarke)` | Legt beim Setup zusätzliche Siegpunktmarker auf Aktionsstapel, etwa mit `defiled_shrine`. | Ja | Bis 3× |
| `start_plus_8_vp_tokens(Landmarke)` | Legt beim Setup zusätzliche Siegpunktmarker auf Geldstapel, etwa mit `aqueduct`. | Ja | Bis 3× |
| `start_replace_copper(Karte)` | Ersetzt ein Startkupfer durch die gewählte Karte. | Ja | Bis 3× |
| `start_replace_copper_from_pile(Karte)` | Registrierter Gegenpart zu `start_replace_estate_from_pile`. Die Ausführung und zulässigen Ziele sind offen. | Ja | Bis 3× |
| `start_replace_estate(Karte)` | Ersetzt ein Startanwesen durch die gewählte Karte. | Ja | Bis 3× |
| `start_replace_estate_from_pile(Karte)` | Ersetzt ein Startanwesen durch eine Karte aus dem gewählten Stapel. Eine eingebaute Vorlage nutzt dreimal `horse`. | Ja | Bis 3× |
| `start_same_reward(Belohnung)` | Legt fest, dass die Belohnungen gleich sind. Eine Kampagnenvorlage nutzt auch die Form ohne Argument. | Offen | Offen |
| `start_shuffle_pile(Geteilter Stapel)` | Mischt den gewählten geteilten Stapel beim Aufbau. | Offen | Offen |
| `start_tavern(Karte)` | Legt eine Kopie auf das Wirtshaus-Tableau. Die EXE enthält ein Beispiel mit `copper`. | Ja | Bis 3× |
| `start_trash_curses` | Legt beim Aufbau Flüche in den Müll. | Ja | Bis 3× |
| `start_trash_pile(Karte)` | Legt beim Aufbau einen Stapel der gewählten Karte in den Müll. | Ja | Bis 3× |
| `start_triple_pile(Stapel)` | Verdreifacht den gewählten Vorratsstapel. | Ja | Bis 3× |
| `start_with_cube(Projekt)` | Jeder Spieler startet mit einem Marker auf dem gewählten Projekt. | Ja | Spielstart bricht bei 2× und 3× ab |

Die Community-Liste nennt außerdem `stamp_allow_second_cube`, `stamp_free`, `start_native_village_mat` und `start_multitrait`. Im Namensregister von Build 3315 stehen für die ersten drei Wirkungen `start_allow_second_cube`, `stamp_cost_zero` und `start_native_village`. Für Multitrait gibt es Hinweise auf `landmod_trait*`-Einträge. Eine passende Importform ist offen. [Community-Liste](https://www.reddit.com/r/dominion/comments/1pnmsph/custom_campaign_features_in_your_kingdoms_tgg/)

### Reproduktion des Stapelbefunds

Die folgenden Adressen sind **RVAs** in `Dominion.exe` mit ImageBase `0x00400000`. Die Datei mit `pefile` öffnen, die Sektionen für die RVA-Zuordnung verwenden und die aufgeführten Codebereiche mit Capstone im x86-32-Modus disassemblieren. Details zu Build und Werkzeugen stehen im [Quellenregister](Sources/Source_Register.md).

| RVA | Befund |
| --- | --- |
| `0x0E5DB0` | Namenssuche im initialisierten Effektregister. Sie durchsucht Datensätze im Abstand von `0x9C0` Bytes mit Namenszeiger bei `+0x04`. Die bevorzugte VA des ersten Datensatzes ist `0x0093E508`. |
| `0x0E5FF0`, `0x0E6084` | `FromExtrasStr` legt `0x280` Bytes für Einträge von `0x14` Bytes an und prüft gegen `0x20` Einträge. Diese Grenze gehört zum Parser. Sie belegt keine vollständige Übernahme durch den Suchdialog. |
| `0x415FB8`, `0x0210E6` | Die Zeichenkette `start_double_pile` wird in den Effekt-Datensatz geschrieben. Dessen Initialisierung hinterlegt die Ausführungsroutine bei RVA `0x110280`. |
| `0x110280` bis `0x110296` | Die Routine holt die Zielkarte und ruft die Vervielfältigung mit festem Faktor 2 auf. Eine Zahl hinter der Zielkarte ändert diesen Faktor im geprüften Pfad nicht. |
| `0x10F980` bis `0x10FA94` | Der Code sucht den Zielstapel. Ohne Treffer kehrt er zurück. Bei einem Treffer durchläuft er die vorhandenen Karten und legt je nach Faktor zusätzliche Karten im Stapel an. Beim Verdoppeln werden so aus 10 Karten 20. Stapel mit anderer Ausgangsgröße werden ebenfalls anhand ihres vorhandenen Inhalts verdoppelt. |

Die festen Kampagnenvorlagen enthalten die Verdopplung unter anderem für Fluch (`0x40DA70`) und Provinz (`0x40E493`). Für genau 15 Exemplare eines normalen Zehnerstapels ist ein eingebauter Parameter bisher offen.

### Spieltests zur Stapelverdopplung

**Im Spiel vom Nutzer bestätigt am 9. November 2026.** Spielbuild 3315, Steam-Build 17683350. Die beiden lokalen Setup-Dateien stammen aus Partien mit je zwei Spielern. Beide enthalten die zehn Basisspielstapel aus der Eingabe.

| Eingabe im Kartensuchfeld | Beobachtung im Spiel | Gespeicherte Zusätze in `GameSetup_Save/config/kingdomData` |
| --- | --- | --- |
| [Einzelner Effekt für Dorf](Steam_Spielanleitung.md#ein-dorfstapel-mit-20-karten) | Dorf startet mit 20 Karten. | Nur `start_double_pile(village)` |
| [Zehn Verdopplungen, durch Kommas getrennt](#testzeile-für-zehn-verdopplungen) | Keller startet mit 20 Karten. Die übrigen Stapel starten mit je zehn. | Nur `start_double_pile(cellar)` |

#### Testzeile für zehn Verdopplungen

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop -x start_double_pile(cellar), start_double_pile(market), start_double_pile(merchant), start_double_pile(militia), start_double_pile(mine), start_double_pile(moat), start_double_pile(remodel), start_double_pile(smithy), start_double_pile(village), start_double_pile(workshop)
```

Beim ersten Versuch mit der langen Zeile meldete der Nutzer einen Kampagnenhinweis und überall zehn Karten. Im zeitlich passenden Spielstand zu diesem früheren Versuch stand eine andere Auswahl, ohne Keller, mit `start_double_pile(cellar)` als einzigem Zusatz. Ohne Zielstapel läuft die Verdopplung ins Leere.

Die jüngeren Spielstände zeigen, dass schon die gespeicherte Konfiguration der Partie nur den ersten Effekt enthält.

Zwei weitere Versuche nutzten `-x start_double_pile(cellar), -x start_double_pile(market)` beziehungsweise `-x start_double_pile(cellar) -x start_double_pile(market)` hinter derselben Kartenauswahl. Beide Formen in **einer** Suchfeldeingabe verdoppelten laut Nutzer keinen Stapel.

Auch zwei nacheinander bestätigte Suchfeldeingaben funktionierten laut Nutzer für Keller und Markt nicht. Der jüngste passende lokale Spielstand enthält nur `start_double_pile(cellar)`.

### Warum das Suchfeld nur einen Stapel verdoppelt

**Statisch bestätigt für Build 3315.** Die Suchfeldeingabe wird vor der Übernahme in die laufende Spielkonfiguration geparst. Der Dialog überspringt beim Zusammenführen einen Eintrag, sobald in der Zielkonfiguration dieselbe Effekt-ID vorhanden ist. Die Karte steht in einem eigenen Feld, das für diesen Vergleich unberücksichtigt bleibt. Beide Stapelverdopplungen haben dieselbe Effekt-ID `0xDB8`. Das erklärt den Befund sowohl mit einer Kommaliste als auch bei zwei getrennten Eingaben. Im [Vorführvideo](https://www.youtube.com/watch?v=nRdrJFtMm3o&t=730s) greifen Verdopplung und Verdreifachung zusammen, weil sie unterschiedliche Effekt-IDs haben.

| RVA in Dominion.exe | Geprüfter Pfad |
| --- | --- |
| `0x1F6F17`, `0x1F6F2F`, `0x1FE032` | Texteingabe wird geparst und an die Zusammenführung bei `0x1F6A90` übergeben. |
| `0x1F6C74` bis `0x1F6CD1`, `0x1F6D6F` | Die Schleife vergleicht ausschließlich das erste DWORD der 20-Byte-Einträge. Bei Gleichheit überspringt sie den neuen Effekt. Ohne Treffer kopiert sie den vollständigen Eintrag in den nächsten freien Slot. |
| `0x0DE780`, `0x0DEA7C`, `0x0E5FF0` | Der Konfigurationsparser verarbeitet einen `-x`-Abschnitt als Kommaliste. Jeder Eintrag speichert seine Effekt-ID. Bei normalen Effekten wie `start_double_pile` speichert er auch sein eigenes Ziel. Wiederholte IDs sind hier zulässig; `twist_room_for_more` nutzt einen [separaten Zielslot](#zusatzstapel-durch-platz-für-mehr). Jeder Aufruf von `FromExtrasStr` initialisiert die Liste neu, weshalb mehrere `-x`-Abschnitte keine Anhängesyntax sind. Eingebaute Kampagnen nutzen dieselbe Kommaliste, unter anderem mit derselben Effekt-ID für mehrere Startkartentausche bei RVA `0x40D342`. |
| `0x20FB10` bis `0x20FB27`, `0x0E48E7`, `0x0E4A41`, `0x0DEDF2` bis `0x0DEE05` | Das Auswählen eines gespeicherten Sets hinterlegt dessen Index. Beim Auflösen liest das Spiel den Kingdom-Text aus dem Profil, parst ihn und übernimmt die ganze Liste mit 32 Einträgen aus dem gespeicherten Set. Dieser Kopierpfad enthält keinen Vergleich der Effekt-IDs. |
| `0x18CD70` bis `0x18CD8A`, `0x11BB20`, `0x11BBFC` | Der Spielaufbau ruft `CampaignApplyExtras` auf. Die Schleife verarbeitet die Effektliste Eintrag für Eintrag. `0x11B630` lädt die Zielkarte aus dem jeweiligen Eintrag. Danach führt `0x110280` die Verdopplung für dieses Ziel aus. |

### Warum die Effekte beim Set-Wechsel hinterherhinken

**Im Spiel vom Nutzer bestätigt am 9. November 2026.** Spielbuild 3315, lokale Partien mit zwei Spielern. In `profiles.xml` enthalten `test` und `test2` dieselben zehn Basisspielstapel und denselben Wert `29732` in `expansionsBitset`. `test` enthält `start_double_pile(cellar), start_double_pile(market)`. `test2` enthält nur `start_double_pile(cellar)`. Laut Nutzer blieben beim ersten direkten Start aus `test` alle Stapel bei zehn. Nach einem Neustart verdoppelte die direkte Auswahl von `test2` nur Keller. Die Reihenfolge `test` → `test2` verdoppelte Keller und Markt. Der neueste lokale Spielstand enthält beide Effekte im tatsächlich gespeicherten `config/kingdomData`. Andere Spielstände aus der Testreihe enthalten keinen Effekt beziehungsweise nur Keller.

**Statisch geprüft.** Beim Set-Wechsel löst der Client zuerst das zuvor ausgewählte Set auf (`0x20FA0C`). Für das neu ausgewählte Set kopiert er ab `0x20FA54` die ersten `0x9C8` Bytes der aufgelösten Konfiguration und einige einzelne Felder. Die Liste der Kampagneneffekte liegt ab Offset `0x1710` und bleibt bei diesem Schritt aus der zuvor aufgelösten Konfiguration erhalten. Danach schreibt der Client den Index des neu ausgewählten Sets (`0x20FB24`) und serialisiert den bereits zusammengesetzten Kingdom-Text (`0x20FB27`, `0x0E4CE0`). Der lokale Startpfad übernimmt diesen Text (`0x0B1E5E`, `0x0DBFA7`, `0x0B1EA8`, `0x0D5E76`). Damit kann der angezeigte Setname bereits `test2` sein, während die Partie die Effekte von `test` erhält.

Der [bestätigte Bedienablauf](Steam_Spielanleitung.md#mehrere-stapel-über-ein-gespeichertes-set-verdoppeln) nutzt diesen Wechsel. Der frühere Vorschlag, das Set mit beiden Effekten direkt auszuwählen und sofort zu starten, ließ die Reihenfolge im Set-Wechsel unberücksichtigt.

**Ergänzender Spieltest am 9. November 2026.** Im aktuellen Profil enthält `test` zehn verschiedene Ziele mit je einem `start_double_pile`. `test2` enthält die gleichen zehn Königreichstapel und keine Kampagneneffekte. Der Nutzer bestätigt zwei funktionierende Abläufe. Nach `test` → **Spielen** → Aufgeben → Set entladen, ohne die Auswahl zu leeren, funktionieren alle zehn Verdopplungen sowohl bei erneuter Auswahl von `test` als auch beim Wechsel zu `test2`. In den beiden jüngsten lokalen Spielständen stehen nacheinander null und zehn Verdopplungen. Das erneute Auswählen allein war als Bedienanleitung unvollständig. Der erste Partiestart und der sichtbare Extraregeln-Marker nach dem Entladen gehören zum bestätigten Ablauf. [Spielanleitung](Steam_Spielanleitung.md#mehrere-stapel-über-ein-gespeichertes-set-verdoppeln)

**Grenzen der Nachschlageliste.** [Start_Double_Pile_Karten.txt](Start_Double_Pile_Karten.txt) enthält die 498 als Königreichstapel markierten IDs aus `Card_Properties.json`, die sieben Grundstapel und `colony`, `platinum`, `potion` und `ruins`. Die Liste verwendet kanonische IDs und lässt den Alias `harem` aus. Sie beschreibt Ziele für den Vorrat, deren einzelne Kombinationen nicht vollständig im Spiel getestet sind. Die Routine bei `0x10F9B6` bis `0x10F9DD` sucht in den aufgebauten Stapeln nach der Ziel-ID und kehrt ohne Treffer zurück. `FromExtrasStr` prüft bei `0x0E6084` gegen 32 Einträge und verzweigt beim 33. erkannten Effekt in den Fehlerpfad. Die vollständige Nachschlageliste ist dadurch kein gültiger einzelner `-x`-Abschnitt. Im registrierten Effektkatalog findet sich kein Sammelbefehl für alle Stapel.

### Zusatzstapel durch Platz für Mehr

**Lokaler Nutzertest am 9. November 2026, Build 3315, Steam-Build 17683350.** In `profiles.xml` stand ein Set mit zehn Basisspielstapeln und `-x twist_room_for_more(chapel)`. Der Nutzer meldete, dass die Kapelle nicht hinzukam. In den dazu passenden lokalen Spielständen steht in `config/kingdomData` nur die Auswahl der zehn Hauptstapel, ohne den Effekt. Die Spielstände zeigen das Ergebnis dieses Tests; sie bestimmen nicht allein, an welcher Stelle der Effekt verloren ging. Der spätere Hintergrundtest mit direkt geladener Effektliste bestätigte einen zusätzlichen Stapel mit zehn Kapellen. [Messwerte](../KingdomEffectsPatch/TEST.md#automatisierte-lokale-effektprüfung-im-hintergrund)

**Statische Analyse derselben EXE.** `FromExtrasStr` behandelt Effekt-ID `0x17D8` separat: Bei RVA `0x0E60D4` bis `0x0E60EF` ruft der Parser den Kartenparser mit Slot `0x6A` und maximal einer Karte auf. Bei `0x0E5F58` schreibt er das Ziel in den gemeinsamen Slot; das Zielfeld des einzelnen Effekteintrags bleibt auf null. Weitere Vorkommen mit einem Kartenziel überschreiben denselben Slot. Der Setup-Callback liest bei `0x112A0B` aus `G+0x100C` und ruft bei `0x112A1B` die Stapel-Einfügeroutine auf.

Der Parser für gespeicherte Kingdom-Texte überträgt die Zusatzkarte bei `0x0DEA90` bis `0x0DEAA8` in die Kartenauswahl (Ziel-ID bei Offset `0x864`). Die Effektliste beginnt bei Offset `0x1710`. Beim Set-Wechsel ersetzt der Client die Kartenauswahl, einschließlich dieses Ziels, mit der neu gewählten Konfiguration (`0x20FA65`) und behält an diesem Schritt die bisherige Effektliste. Bei der Ausgabe liest der Client beide Teile getrennt (`0x0E357B` bis `0x0E35F9`, `0x0E5510` bis `0x0E552B`). Ein vorhandenes Ziel und der zugehörige Effekt können dadurch beim Wechsel auseinanderfallen. Die mit Verdopplungen bestätigte Bedienfolge belegt keine Übernahme von „Platz für Mehr“.

**Kampagnenvorlage.** Die eingebauten Texte für Füllhorn enthalten dieselbe Syntax, darunter die achte Mission mit `young_witch(harbinger)` und `twist_room_for_more(gardens)` (RVA `0x40EBA0` und `0x40EBBA`). [Build und EXE-Hash](Sources/Source_Register.md#reproduktion-für-l01)

### Effektsets und Zufallsauswahl

**Im Spiel vom Nutzer bestätigt am 9. November 2026.** Build 3315, lokale Partien mit zwei Spielern. Das Profilset `test` bestand nur aus einem `-x`-Abschnitt mit zehn `start_double_pile`-Einträgen. Der erste Partiestart enthielt ein zufällig gezogenes Königreich und keine Verdopplungen. Nach der Übernahme in ein weiteres Set enthielt die lokale Partie alle zehn Effekte. Beim Versuch, ein wesentlich längeres Effektset zu laden, stürzte das Spiel laut Nutzer ab. Das lokale Log enthält die fehlgeschlagene Assertion `numExtras < maxExtras` in `FromExtrasStr`. Der Abgleich mit RVA `0x0E6084` erklärt den Abbruch ab dem 33. erkannten Effekt.

Ein Effektset ist damit unabhängig von der Kartenauswahl wiederverwendbar. Jedes Ziel belegt einen Eintrag, auch wenn der entsprechende Stapel in der Partie fehlt. Beim Spielaufbau werden nur vorhandene Zielstapel verdoppelt. Eine feste Liste kann alle Zufallsziehungen abdecken, wenn der Pool höchstens 32 Ziele umfasst. Die [Spielanleitung](Steam_Spielanleitung.md#zufällige-königreiche-mit-doppelten-stapeln) beschreibt den Ablauf für kleine und große Zufallspools.

Für eine Mod mit unveränderter Zufallsauswahl über alle Erweiterungen ist der relevante Eingriffspunkt der Aufbau der bereits gezogenen Königreichstapel. Die Schleife ab `0x18CC16` initialisiert die zehn Hauptstapel anhand der aufgelösten Auswahl. Der Effektpfad wird anschließend bei `0x18CD70` bis `0x18CD8A` aufgerufen. Der implementierte Patch ergänzt die zehn nativen Verdopplungseffekte beim Auflösen einer neuen lokalen Zufallspartie. Der Einzelspieler-Test bestätigte die Automatik. Gezielte Spieltests für Sonderstapel und Wiederaufnahme stehen aus. [Testprotokoll](../KingdomEffectsPatch/TEST.md#automatische-verdopplung-im-einzelspieler)

### Kampagneneffekte im privaten Online-Spiel

**Im Spiel gemessen am 10. November 2026, Spielbuild 3315, Steam-Build 17683350.** Der gepatchte Host sandte `start_double_pile(village)` und `start_double_pile(market)` nach dem lokalen Freischaltungsfilter. Der Online-Dienst lieferte beide Effekte im ursprünglichen `setupData` zurück. Im aufgelösten `kingdomData` der gestarteten Partie fehlte der `-x`-Abschnitt. Der Client übernimmt beim Start den Effektblock aus `kingdomData` und installierte null Effekte. Das erklärt das Effektsymbol in der Lobby und die unveränderten Vorräte in der Partie. Der Befund gilt für diese Kombination. Welche Serverregel sie beim Auflösen auslässt, ist offen. [Netzwerkmessung](../KingdomEffectsPatch/TEST.md#ergebnis-der-netzwerkmessung-am-10-november-2026)

**Statisch geprüft.** Beitritt, Spielstart und Revanche bieten im untersuchten Client keinen zweiten Übergabepunkt für den Effekttext. Der Dienst und die Clients tauschen während der Partie Zugprotokolle aus. Ein Wiedereinfügen der Effekte in beiden Clients könnte die lokalen Vorräte ändern. Ob der Dienst Züge mit diesen Vorräten akzeptiert, ist offen. Die installierte Steam-Version bietet in den untersuchten Pfaden keinen nutzbaren eigenen Server. [Aufrufpfade im Patchplan](../KingdomEffectsPatch/PLAN.md#online-spielzüge-und-lokales-mehrspielerspiel)

**Offener Spieltest.** Mit **Lokal spielen → Weitergeben** und **Steam Remote Play Together** können zwei Personen eine lokale Spielinstanz über das Internet teilen. Der Patch läuft auf dem Host. Beide sehen denselben Spielbildschirm. Ob Steam aus der gepatchten Arbeitskopie eine Einladung anbietet und die Effekte im Modus Weitergeben wie im lokalen Einzelspieler greifen, ist noch zu testen. [Dominion auf Steam](https://store.steampowered.com/app/1131620/) · [Steam Remote Play Together](https://store.steampowered.com/remoteplay#together)

### Native Effektmessung ohne Fensterfokus

**Im Spiel gemessen am 10. November 2026, Build 3315, Steam-Build 17683350.** Die geprüfte Arbeitskopie lässt sich mit einem temporären Profil auf einem separaten Windows-Desktop starten. Der Runner legt die lokale Partie über Dominions Startlogik an und misst den ersten Zug aus dem nativen Spielzustand. In 184 lokalen Vergleichspartien mit zwei menschlichen Spielerplätzen wurde die Wirkung von 47 Effekten bestätigt. Zwei zusätzliche Fälle mit mehrfachen Projektsteinen brachen beim Start ab. Automatische Ereigniskäufe machen auch einige Kauf- und Erhaltensregeln im ersten Zug messbar. Die Tabellen geben den Stand für einfache und mehrfache Wirkung an. [Testergebnisse und Aufruf](../KingdomEffectsPatch/TEST.md#automatisierte-lokale-effektprüfung-im-hintergrund)
