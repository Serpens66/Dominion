# Effektübernahme und Speicherung: isolierter Test

Geprüft am 11. November 2026 mit Dominion **3315**, Steam-Build **17683350**, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`.

**Ergebnis: 356/356 Fälle bestanden.** Alle 89 Effekte einzeln und zwischen `start_double_pile(market)` und `twist_big_ideas`, jeweils ohne Patch und mit dem aktuellen Patch. Verglichen werden Einlesen, Textausgabe, versionsabhängiges Einlesen (Version 23), Auflösung einer benutzerdefinierten Konfiguration, vollständige Effektwerte und Zielparameter. Die Routinen laufen auf dem Spielthread eines unsichtbaren Testfensters. Keine Eingabegeräte und kein normales Profil werden verwendet.

Zusätzlich besteht der Negativtest für vertauschte IDs bei gleicher Anzahl, ein falsches Kartenziel und eine unbekannte ID zwischen gültigen Befehlen.

Ein echter lokaler Spielaufbau enthält in dieser Reihenfolge Müllhaufen, Vom Thron, Deponie, So viel Zeugs, Bechergläser und Große Ideen; die sechs internen IDs stimmen ohne und mit Patch überein. Dies bestätigt die Ankunft der Effekte, nicht jede ihrer Spielwirkungen.

**Ergänzung vom 12. November:** Der Verlust ist inzwischen im zusätzlichen `setupData`-Startpfad reproduziert und gezielt korrigiert. Siehe [Startpfad, Erweiterungsfilter und Vergleichstests](Effect_Startup_Tests.md). Die folgende Tabelle belegt weiterhin die Parser-Rundreise; die tatsächliche Spielwirkung bleibt getrennt zu bewerten.

## Ergebnisse je Effekt

Die beiden Prüfspalten stehen jeweils für Einzelbefehl **und** Dreierfolge. Der Spielwirkungsstand stammt getrennt aus der Anleitung; Mehrfachwirkung bleibt gesondert dokumentiert.

| Effekt-ID | Geprüftes Ziel | Ohne Patch | Mit Patch | Spielwirkung einmal | Spielwirkung mehrfach |
| --- | --- | --- | --- | --- | --- |
| `twist_big_ideas` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_busy_busy` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_extra_oomph` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_forever_favored` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_garbage_heap` | `ohne Ziel` | Bestanden | Bestanden | Teilprüfung: laut Nutzertest nicht übernommen; Ursache offen | Offen |
| `twist_penny_party` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_shoppers` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_cramped` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_from_the_throne` | `ohne Ziel` | Bestanden | Bestanden | Ja (Nutzertest, 1×) | Offen |
| `twist_landfill` | `ohne Ziel` | Bestanden | Bestanden | Ja (Nutzertest, 1×) | Offen |
| `twist_so_much_stuff` | `ohne Ziel` | Bestanden | Bestanden | Ja (Nutzertest, 1×) | Offen |
| `twist_double_up` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_fast_track` | `ohne Ziel` | Bestanden | Bestanden | Beiseitelegen bestätigt | Bei 2× und 3× auf der Ablage |
| `twist_favored_landlords` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_favored_wealthy` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_incoming` | `ohne Ziel` | Bestanden | Bestanden | Ja | Gleiche Wirkung |
| `twist_rebates` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_beakers` | `ohne Ziel` | Bestanden | Bestanden | Teilprüfung: laut Nutzertest nicht übernommen; Ursache offen | Offen |
| `twist_delayed_joy` | `ohne Ziel` | Bestanden | Bestanden | Offen | Offen |
| `twist_pennies_saved` | `ohne Ziel` | Bestanden | Bestanden | Offen | Offen |
| `twist_real_estate_barons` | `ohne Ziel` | Bestanden | Bestanden | Offen | Offen |
| `twist_sale` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `twist_head_start` | `ohne Ziel` | Bestanden | Bestanden | Offen | Offen |
| `twist_mirror_loot` | `ohne Ziel` | Bestanden | Bestanden | Ja | Gleiche Wirkung |
| `twist_room_for_more` | `(village)` | Bestanden | Bestanden | Ja | Einmal im F8-Editor |
| `twist_turn_turn_turn` | `ohne Ziel` | Bestanden | Bestanden | Offen | Offen |
| `twist_typhoon` | `(village,smithy)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_plus_action` | `(village)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_plus_card` | `(village)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_plus_buy` | `(village)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_plus_coin` | `(village)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_chameleonize` | `(village)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_discounted` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `stamp_pay_later` | `(village)` | Bestanden | Bestanden | Ja | Gleiche Wirkung |
| `stamp_two_for_one` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `stamp_offers_insight` | `(village)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_with_horse` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `stamp_with_spoils` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `stamp_with_villager` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `stamp_durationified` | `(village)` | Bestanden | Bestanden | Zusatztyp bestätigt | Gleicher Zusatztyp |
| `stamp_treasurified` | `(village)` | Bestanden | Bestanden | Zusatztyp bestätigt | Gleicher Zusatztyp |
| `stamp_victorified` | `(village)` | Bestanden | Bestanden | Zusatztyp bestätigt | Gleicher Zusatztyp |
| `stamp_reactionified` | `(village)` | Bestanden | Bestanden | Zusatztyp bestätigt | Gleicher Zusatztyp |
| `stamp_shadowified` | `(village)` | Bestanden | Bestanden | Zusatztyp bestätigt | Gleicher Zusatztyp |
| `stamp_cost_2_less` | `(travelling_fair)` | Bestanden | Bestanden | Ja | Bis 3× |
| `stamp_cost_2D_less` | `(travelling_fair)` | Bestanden | Bestanden | Startabbruch mit Triumph | Startabbruch mit Triumph |
| `stamp_cost_zero` | `(travelling_fair)` | Bestanden | Bestanden | Ja | Gleiche Wirkung |
| `stamp_bonus_money` | `(travelling_fair)` | Bestanden | Bestanden | Ja | Bis 3× |
| `stamp_also_silver` | `(travelling_fair)` | Bestanden | Bestanden | Ja | Gleiche Wirkung |
| `stamp_motivational` | `(travelling_fair)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_got_it_already` | `(travelling_fair)` | Bestanden | Bestanden | Ja | Bis 3× |
| `stamp_one_track_mind` | `(travelling_fair)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_fawning_service` | `(travelling_fair)` | Bestanden | Bestanden | Ja | Gleiche Wirkung |
| `stamp_perks_of_wealth` | `(travelling_fair)` | Bestanden | Bestanden | Ja | Gleiche Wirkung |
| `stamp_double_vp` | `(arena)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_action_way` | `(way_of_the_ox)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_coin_way` | `(way_of_the_ox)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_also_debt` | `(approaching_army)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_also_gold` | `(approaching_army)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_also_plague` | `(approaching_army)` | Bestanden | Bestanden | Offen | Offen |
| `stamp_also_respect` | `(approaching_army)` | Bestanden | Bestanden | Offen | Offen |
| `start_10_favor` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_10_villagers` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_5_coffers` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_add_card` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_add_each_pile` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_allow_second_cube` | `(pageant)` | Bestanden | Bestanden | Offen | Offen |
| `start_curse_cost_3` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_double_deck` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_double_heirloom` | `(pixie)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_double_pile` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_double_vp_tokens` | `(arena)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_in_play` | `(amulet)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_journey_facedown` | `ohne Ziel` | Bestanden | Bestanden | Ja | Wechselt pro Kopie |
| `start_make_split_pile` | `(village,smithy)` | Bestanden | Bestanden | Ja | Gleiche Wirkung |
| `start_native_village` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_plus_2_vp_tokens` | `(arena)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_plus_8_vp_tokens` | `(arena)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_replace_copper` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_replace_copper_from_pile` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_replace_estate` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_replace_estate_from_pile` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_same_reward` | `ohne Ziel` | Bestanden | Bestanden | Ja | Gleiche Wirkung |
| `start_shuffle_pile` | `(village)` | Bestanden | Bestanden | Ja | Stapel bis 3× gemischt |
| `start_tavern` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_trash_curses` | `ohne Ziel` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_trash_pile` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_triple_pile` | `(village)` | Bestanden | Bestanden | Ja | Bis 3× |
| `start_with_cube` | `(pageant)` | Bestanden | Bestanden | Ja | Spielstart bricht bei 2× und 3× ab |

## Reproduktion

Die Entwicklungswerkzeuge `roundtrip_test.exe` und `roundtrip_probe.dll` entstehen mit `KingdomEffectsPatch/build.cmd staging`. Aufruf siehe `KingdomEffectsPatch/RELEASING.md`. Der Rohbericht `effect-roundtrip.tsv` bleibt unter `out/staging`; der Download enthält keine Testprogramme, Profile oder Rohprotokolle. Die Release-Nutzung startet weiterhin die Originalinstallation mit dem normalen Profil.
