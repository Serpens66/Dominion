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
| `-x effekt(ziel), weiterer_effekt` | Zusatzregeln hinter der Kartenauswahl. Diese Schreibweise steht in den eingebauten Kampagnenvorlagen. Die Nutzung im normalen Kartensuchfeld ist von der Community berichtet. Die EXE trennt und serialisiert die Zusätze als eigenen Teil der Spielkonfiguration. |
| `hauptkarte(ziel)` im `-k`-Teil | Ordnet einer Karte oder Landschaft das konkrete Ziel ihrer Setup-Regel zu. Eingebaute Vorlagen enthalten `young_witch(village)` für den Bannstapel, `ferryman(shop)` für den Stapel neben dem Vorrat, `obelisk(patrician)` für den gewählten Aktionsstapel und `riverboat(imperial_envoy)` für die beiseitegelegte Karte. |

Weitere Formen für den normalen Suchdialog sind **von der Community berichtet**. Der Name einer Erweiterung belegt einen zufälligen Königreichplatz aus genau dieser Erweiterung. Ein Landschaftstyp wie Ereignis oder Projekt lässt sich ebenfalls für einen Zufallsplatz vorgeben. Für Speicherstruktur, IDs und Setup-Abhängigkeiten siehe [Kingdom-Sets](Kingdom_Sets.md), [deutsche Kartentitel](Card_Titles_de.json), [Karteneigenschaften](Card_Properties.json) und die [Community-FAQ](https://www.reddit.com/r/dominion/comments/1ahhoec/dominion_app_faq/).

### Belegstand des Effektkatalogs

Die Tabellen erfassen **89 registrierte Namen** aus den drei Namensgruppen in Build 3315. „Registriert“ bezeichnet einen Namen, dessen Adresse der Initialisierungscode in das vom Parser abgefragte Namensfeld schreibt. Ein Name an anderer Stelle in der EXE kann etwa zu einer Kampagnenvorlage oder einer UI-Übersetzung gehören. Registrierung belegt die Zuordnung des Namens im Parser. Gültige Zieltypen, die praktische Wirkung und die Übernahme aus dem Suchfeld sind davon getrennt zu prüfen.

Die Klammerargumente geben Zieltypen an. Im Kingdom-Text stehen darin konkrete interne IDs. Wirkungen und Zieltypen beruhen auf Kampagnenvorlagen, lokalisierten Effekttexten oder Community-Beschreibungen. Die explizit genannten Codepfade sind statisch geprüft. Ein bestätigter Spieltest wird mit Build, Modus, Eingabe und beobachtetem Ergebnis erfasst. Die Community-Nutzung in eigenen Kingdoms ist ein eigener Beleg. Für private Online-Partien bleibt die Übernahme offen. [Community-Syntax](https://www.reddit.com/r/dominion/comments/1pnmsph/custom_campaign_features_in_your_kingdoms_tgg/), [Kampagneneffekte im Dominion-Wiki](https://wiki.dominionstrategy.com/index.php/Campaigns_%28Temple_Gates_Games%29)

#### Stempel für Karten und Landschaften

| Registrierte Schreibweise | Ziel und Wirkung |
| --- | --- |
| `stamp_action_way(Weg)` | Der Weg gibt beim Verwenden eine zusätzliche Aktion. |
| `stamp_coin_way(Weg)` | Der Weg gibt beim Verwenden eine zusätzliche Münze. |
| `stamp_also_debt(Prophezeiung)` | Beim Eintreten erhält jeder Spieler fünf Schulden. |
| `stamp_also_gold(Prophezeiung)` | Beim Eintreten erhält jeder Spieler drei Gold außerhalb des Vorrats. |
| `stamp_also_plague(Prophezeiung)` | Beim Eintreten erhält jeder Spieler drei Flüche außerhalb des Vorrats. |
| `stamp_also_respect(Prophezeiung)` | Beim Eintreten erhält jeder Spieler ebenso viele Siegpunktmarker, wie er bereits hat. |
| `stamp_also_silver(Ereignis)` | Beim Kauf erhält der Spieler ein Silber. |
| `stamp_bonus_money(Ereignis)` | Beim Kauf erhält der Spieler eine Münze. |
| `stamp_chameleonize(Karte)` | Tauscht die auf der Karte gewährten Münz- und Kartenziehboni gegeneinander. |
| `stamp_cost_2_less(Ereignis)` | Senkt die Münzkosten um zwei. |
| `stamp_cost_2D_less(Ereignis)` | Senkt die Schuldenkosten um zwei. |
| `stamp_cost_zero(Ereignis)` | Setzt die Ereigniskosten auf null. |
| `stamp_discounted(Karte)` | Senkt die Münzkosten um eins. |
| `stamp_double_vp(Landmarke)` | Verdoppelt die von der Landmarke vergebenen Siegpunkte. |
| `stamp_durationified(Karte)` | Macht sie zusätzlich zur Dauerkarte. Im nächsten Zug gibt sie zu Beginn eine Karte und eine Aktion. |
| `stamp_fawning_service(Ereignis)` | Beim Erhalten einer Provinz wird das Ereignis kostenlos gekauft. |
| `stamp_got_it_already(Ereignis)` | Das Ereignis wird im ersten Zug kostenlos gekauft. |
| `stamp_motivational(Ereignis)` | Nach dem Kauf werden am Zugende zwei Karten gezogen. |
| `stamp_offers_insight(Karte)` | Wer sie erhält, zieht am Zugende eine Karte. |
| `stamp_one_track_mind(Ereignis)` | Das Ereignis wird zu Beginn der Kaufphase kostenlos gekauft. |
| `stamp_pay_later(Karte)` | Wandelt Münzkosten in Schulden um. |
| `stamp_perks_of_wealth(Ereignis)` | Beim Erhalten von Gold wird das Ereignis ausgeführt. |
| `stamp_plus_action(Karte)` | Beim Ausspielen gibt sie eine zusätzliche Aktion. |
| `stamp_plus_buy(Karte)` | Beim Ausspielen gibt sie einen zusätzlichen Kauf. |
| `stamp_plus_card(Karte)` | Beim Ausspielen wird eine zusätzliche Karte gezogen. |
| `stamp_plus_coin(Karte)` | Beim Ausspielen gibt sie eine zusätzliche Münze. |
| `stamp_reactionified(Karte)` | Macht sie zur Reaktion, die beim Erhalten, Entsorgen oder Ablegen außerhalb des Aufräumens ausgespielt werden kann. |
| `stamp_shadowified(Karte)` | Macht sie zum Schatten mit den zugehörigen Regeln fürs Ausspielen aus dem Nachziehstapel. |
| `stamp_treasurified(Karte)` | Gibt ihr zusätzlich den Typ Geld. |
| `stamp_two_for_one(Karte)` | Erhöht den Preis um eine Münze. Beim Erhalten kommt eine zusätzliche Kopie von außerhalb des Vorrats dazu. |
| `stamp_victorified(Karte)` | Gibt ihr zusätzlich den Typ Sieg und den Wert von einem Siegpunkt. |
| `stamp_with_horse(Karte)` | Beim Erhalten bekommt der Spieler zusätzlich ein Pferd. |
| `stamp_with_spoils(Karte)` | Beim Erhalten bekommt der Spieler zusätzlich Beute aus Dark Ages (`spoils`). |
| `stamp_with_villager(Karte)` | Beim Erhalten bekommt der Spieler zusätzlich einen Dorfbewohner. |

#### Spielvariationen für die Partie

| Registrierte Schreibweise | Ziel und Wirkung |
| --- | --- |
| `twist_beakers` | Nicht ausgegebene Tränke bleiben für spätere Züge erhalten. |
| `twist_big_ideas` | Zu Zugbeginn eine Karte mehr ziehen. |
| `twist_busy_busy` | Zu Zugbeginn eine zusätzliche Aktion. |
| `twist_cramped` | Die reguläre Handgröße beträgt vier Karten. |
| `twist_delayed_joy` | Gaben können bis zum nächsten Zug aufbewahrt werden. |
| `twist_double_up` | Die erste erhaltene Aktions- oder Geldkarte des Zuges gibt eine zusätzliche Kopie von außerhalb des Vorrats. |
| `twist_extra_oomph` | Zu Zugbeginn eine zusätzliche Münze. |
| `twist_fast_track` | Erhaltene Dauer- oder Reservekarten werden beiseitegelegt und zu Beginn des nächsten Aufräumens ausgespielt. |
| `twist_favored_landlords` | Beim Erhalten einer Siegkarte gibt es einen Gefallen. |
| `twist_favored_wealthy` | Beim Erhalten einer Geldkarte gibt es einen Gefallen. |
| `twist_forever_favored` | Zu Zugbeginn einen Gefallen erhalten. |
| `twist_from_the_throne` | Die erste Aktion des Zuges wird zweimal ausgespielt. |
| `twist_garbage_heap` | Zu Zugbeginn darf eine Handkarte entsorgt werden. |
| `twist_head_start` | In der Kampagne erhält der Mensch zwei frühe Extrazüge und die KI zehn Siegpunktmarker. |
| `twist_incoming` | Erhaltene Aktions- und Geldkarten kommen auf den Nachziehstapel. |
| `twist_landfill` | Beim Entsorgen einer eigenen Karte wird eine Karte gezogen. |
| `twist_mirror_loot` | Alle Karten des Loot-Stapels sind gleich. |
| `twist_pennies_saved` | Übrige Münzen werden am Ende der Kaufphase in Taler umgewandelt. |
| `twist_penny_party` | Zu Zugbeginn ein Kupfer erhalten. |
| `twist_real_estate_barons` | Siegpunktmarker können wie Taler ausgegeben werden. |
| `twist_rebates` | Beim Erhalten einer Karte gibt es eine Münze. |
| `twist_room_for_more(Königreichstapel)` | Fügt dem Vorrat einen weiteren Königreichstapel hinzu. |
| `twist_sale` | Karten kosten eine Münze weniger. |
| `twist_shoppers` | Zu Zugbeginn ein zusätzlicher Kauf. Name und Beschreibung sind lokal belegt. |
| `twist_so_much_stuff` | Die erste ausgespielte Karte des Zuges gibt eine Kopie von außerhalb des Vorrats. |
| `twist_turn_turn_turn` | In der Kampagne rotieren geteilte Stapel zu Beginn der menschlichen Züge ab dem zweiten. |
| `twist_typhoon(Karte, …)` | Ersetzt in der Kampagne nach und nach Königreichstapel. Eine Zielliste steht in eingebauten Vorlagen. |

#### Änderungen beim Spielaufbau

| Registrierte Schreibweise | Ziel und Wirkung |
| --- | --- |
| `start_10_favor` | Jeder Spieler startet mit zehn Gefallen. |
| `start_10_villagers` | Jeder Spieler startet mit zehn Dorfbewohnern. |
| `start_5_coffers` | Jeder Spieler startet mit fünf Talern. |
| `start_add_card(Karte)` | Jeder Spieler erhält eine Kopie der Karte ins Startdeck. Der Code schreibt in den Kartenbereich für Startkarten. |
| `start_add_each_pile` | Jeder Spieler erhält eine Kopie von jedem Königreichstapel ins Startdeck. Der Code schreibt in den Kartenbereich für Startkarten. |
| `start_allow_second_cube(Projekt)` | Erlaubt einen zweiten Marker auf dem Projekt. |
| `start_curse_cost_3` | Flüche kosten drei Münzen. |
| `start_double_deck` | Verdoppelt das Startdeck. |
| `start_double_heirloom(Karte)` | Für eine Königreichkarte mit Erbstück ersetzen zwei Exemplare des Erbstücks Startkupfer. |
| `start_double_pile(Stapel)` | Verdoppelt den gewählten Vorratsstapel. Der Kartenaufbau ist statisch geprüft. |
| `start_double_vp_tokens(Landmarke)` | Verdoppelt die Startmenge an Siegpunktmarkern der gewählten Landmarke. |
| `start_in_play(Dauerkarte)` | Die gewählte Dauerkarte liegt bei Spielbeginn bereits im Spiel. Die Community nennt nur bestimmte geeignete Ziele. |
| `start_journey_facedown` | Der Reisemarker beginnt verdeckt. |
| `start_make_split_pile(Stapel, Karte)` | Ersetzt die untersten fünf Karten des ersten Stapels durch fünf der zweiten Karte. |
| `start_native_village(Karte)` | Legt eine Kopie auf das Tableau von Eingeborenendorf. Die EXE enthält ein Beispiel mit `silver`. |
| `start_plus_2_vp_tokens(Landmarke)` | Legt beim Setup zusätzliche Siegpunktmarker auf Aktionsstapel, etwa mit `defiled_shrine`. |
| `start_plus_8_vp_tokens(Landmarke)` | Legt beim Setup zusätzliche Siegpunktmarker auf Geldstapel, etwa mit `aqueduct`. |
| `start_replace_copper(Karte)` | Ersetzt ein Startkupfer durch die gewählte Karte. |
| `start_replace_copper_from_pile(Karte)` | Registrierter Gegenpart zu `start_replace_estate_from_pile`. Die Ausführung und zulässigen Ziele sind offen. |
| `start_replace_estate(Karte)` | Ersetzt ein Startanwesen durch die gewählte Karte. |
| `start_replace_estate_from_pile(Karte)` | Ersetzt ein Startanwesen durch eine Karte aus dem gewählten Stapel. Eine eingebaute Vorlage nutzt dreimal `horse`. |
| `start_same_reward(Belohnung)` | Legt fest, dass die Belohnungen gleich sind. Eine Kampagnenvorlage nutzt auch die Form ohne Argument. |
| `start_shuffle_pile(Geteilter Stapel)` | Mischt den gewählten geteilten Stapel beim Aufbau. |
| `start_tavern(Karte)` | Legt eine Kopie auf das Wirtshaus-Tableau. Die EXE enthält ein Beispiel mit `copper`. |
| `start_trash_curses` | Legt beim Aufbau Flüche in den Müll. |
| `start_trash_pile(Karte)` | Legt beim Aufbau einen Stapel der gewählten Karte in den Müll. |
| `start_triple_pile(Stapel)` | Verdreifacht den gewählten Vorratsstapel. |
| `start_with_cube(Projekt)` | Jeder Spieler startet mit einem Marker auf dem gewählten Projekt. |

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

### Spieltest mit zehn Verdopplungen

**Vom Nutzer im Spiel beobachtet am 9. November 2026.** Der installierte Build war 3315. Der Nutzer fügte die [Basisspielzeile mit zehn Verdopplungen](Steam_Spielanleitung.md#zehn-stapel-mit-je-20-karten) ein. Ein Kampagnenhinweis erschien. Laut Nutzer hatten die Stapel weiterhin je zehn Karten. Der genaue Spielmodus und der Zustand der Auswahl vor dem Einfügen sind offen.

**Lokal beobachtet.** Der zeitlich passende jüngste lokale Spielstand enthielt in `GameSetup_Save/config/kingdomData` eine andere Kartenauswahl und nur `start_double_pile(cellar)`. `cellar` kam im gespeicherten Vorrat nicht vor. Falls die Datei zu dieser Partie gehört, passt das zum statisch geprüften Rücksprung bei fehlendem Ziel. Die Ursache für die abweichende Kartenauswahl und die fehlenden übrigen Zusätze ist offen.

**Ausstehender Kontrolltest.** In einer frischen lokalen Spielkonfiguration zuerst die zehn Basisspielstapel und einen einzelnen Effekt für `village` einfügen. Vor Spielbeginn Dorf in der Auswahl und nach Spielbeginn den Stapelzähler prüfen. Danach die lange Zeile separat testen und, wenn der Effekt ausbleibt, den exportierten Text und den lokalen Setup-Eintrag mit der Eingabe vergleichen.
