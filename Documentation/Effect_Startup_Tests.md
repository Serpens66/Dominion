# Effektverlust im lokalen Spielaufbau – Befund und Korrektur

Geprüft am **12. November 2026**, Dominion **3315**, Steam-Build **17683350**, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`.

## Ursache

Der Nutzertest vom 11. November, 23:53 Uhr, enthielt acht Effekte in F8 und unmittelbar vor dem Start, aber nur vier im aufgebauten Spiel und später in F8. Der erste entfernte Eintrag war `twist_pennies_saved` (ID `0x17DB`).

Die native Normalisierung bei RVA `0x0DBEF0` prüft die Effektdefinition auf Erweiterungsbedingungen vom Typ 7. Bei nicht erfüllter Vorgabe setzt sie die Effekt-ID auf null (RVA `0x0DC488`). Sie verdichtet das Array anschließend nicht. Die Ausgabe bei RVA `0x0E5370` beendet die Liste am ersten Nulleintrag: Dadurch verschwinden auch die nachfolgenden gültigen Effekte. Der geänderte Text wird in die Konfiguration zurückgeschrieben; F8 zeigt danach folgerichtig die verkürzte Konfiguration.

Der frühere isolierte Spielstart verwendete bereits festgelegtes `kingdomData` und umging diesen zusätzlichen Normalisierungspfad. Die 356 Parserprüfungen waren korrekt, deckten den Fehler aber nicht ab. Der neue Test verwendet zusätzlich `setupData` mit nativer Königreich-Erzeugung.

## Gezielte lokale Korrektur

Nur während des lokalen Startaufrufs bei RVA `0x0B1E6B` aktiviert ein Thread-lokaler Zähler die Erhaltung der gewählten Kampagneneffekte. Der Hook bei `0x0DC2CD` überspringt in diesem Kontext ausschließlich die Erweiterungsmetadatenprüfung. Die davor liegende Prüfung der Zielkarte bleibt erhalten. Alle anderen Aufrufer der Normalisierung behalten ihr bisheriges Verhalten; der Online-Diagnosemodus installiert diese zwei Hookstellen nicht. Es werden keine Karten freigeschaltet und keine Effekte nachträglich in laufende Partien eingesetzt.

Alle sechs Aufrufer von `0x0DBEF0` wurden abgeglichen: `0x0B1E6B`, `0x0B3CA6`, `0x0E3A21`, `0x1F6D53`, `0x1FE062`, `0x1FFA03`. Nur der erstgenannte Aufruf aktiviert den lokalen Kontext.

## Vergleichsmessungen

Alle Tests laufen in getrennten Spielprozessen mit temporären Profilen auf einem unsichtbaren Desktop, ohne Maus- oder Tastatureingaben. Die Release-Nutzung verwendet weiterhin Originalinstallation und normales Profil.

| Fall | Ohne Patch, setupData: angekommen / erwartet | Mit Patch, setupData | Mit Patch, kingdomData |
| --- | --- | --- | --- |
| Exakte Achterfolge des Nutzers | 4 / 8 | 8 / 8 | 8 / 8 |
| Gesparte Pennys allein | 0 / 1 | 1 / 1 | 1 / 1 |
| Gesparte Pennys zwischen Doppelstapel und Große Ideen | 1 / 3 | 3 / 3 | 3 / 3 |
| Gesparte Pennys zuerst | 0 / 3 | 3 / 3 | 3 / 3 |
| Gesparte Pennys zuletzt | 2 / 3 | 3 / 3 | 3 / 3 |
| Großeigentümer allein | 0 / 1 | 1 / 1 | 1 / 1 |
| Großeigentümer zwischen zwei Effekten | 1 / 3 | 3 / 3 | 3 / 3 |
| Vorsprung allein | 0 / 1 | 1 / 1 | 1 / 1 |
| Vorsprung zwischen zwei Effekten | 1 / 3 | 3 / 3 | 3 / 3 |
| Rotationskoller allein | 0 / 1 | 1 / 1 | 1 / 1 |
| Rotationskoller zwischen zwei Effekten | 1 / 3 | 3 / 3 | 3 / 3 |
| Müllhaufen, Bechergläser, Große Ideen | 0 / 3 | 3 / 3 | 3 / 3 |

Die native Dispatcher-Aktion 9 schließt die lokale Partie (`0x0B1F60`) und startet erneut (`0x0B1E20`). Nach diesem Übergang enthält die Achterfolge weiterhin alle acht IDs in richtiger Reihenfolge; der von F8 verwendete Konfigurationsauflöser liest acht Einträge. Die sichtbare Navigation über das Spielmenü bleibt Teil der Nutzerabnahme.

Die bisherigen 356 Parser-/Ausgabe-/Konfigurationsprüfungen bleiben zusätzlich bestehen. **Übernahme ist kein vollständiger Spielwirkungstest.** Zum damaligen automatisierten Prüfstand blieben das tatsächliche Ausgeben von Siegpunktmarkern, zusätzliche Züge und Rotation offen. Die spätere Nutzerbestätigung unten ergänzt die einfache Wirkung von Großeigentümer und Vorsprung. Rotationskoller und die Mehrfachwirkung bleiben offen. Bei Rotationskoller benötigt der gezielte Wirkungstest tatsächlich einen passenden rotierbaren Stapel, unabhängig davon, dass der Effekt nun erhalten bleibt.

## Erweiterungsangaben

[Effect_Expansion_References.json](Effect_Expansion_References.json) enthält den aus den initialisierten nativen Definitionen gelesenen Erweiterungsbezug aller 89 Effekte. 45 besitzen eine Vorgabe; mehrere IDs innerhalb einer Gruppe sind Alternativen, keine gleichzeitige Pflicht.

| Effekt | Native Filtervorgabe |
| --- | --- |
| Bechergläser | Alchemisten |
| Müllhaufen | Dark Ages |
| Gesparte Pennys | Gilden |
| Großeigentümer | Empires |
| Vorsprung | Seaside |
| Rotationskoller | Verbündete |

Diese Angaben sind **keine bewiesenen zwingenden Voraussetzungen der Spielwirkung**. Einige Mechaniken lassen sich unabhängig von den Karten der zugehörigen Erweiterung verwenden. Zielkarten und besondere Auslösesituationen bleiben zusätzliche Voraussetzungen. F8 zeigt die separat redaktionell erfasste Mechanik-/Zielvoraussetzung in der Wirkungsspalte. Die native Filtervorgabe bleibt im Detailtext und in der Suche erhalten. Beide Anleitungsfassungen besitzen getrennte Spalten. [Daten und Beleggrenzen](F8_Editor_Usability.md)

## Reproduktion

`build.cmd staging` baut `startup_effect_test.exe` und die ausschließlich zum Test verwendete `roundtrip_probe.dll`. Aufruf:

```powershell
$env:SteamAppId = '1131620'
$env:SteamGameId = '1131620'
.\out\staging\startup_effect_test.exe '<separate Testkopie>\Dominion.exe' '<Projekt>\KingdomEffectsPatch\out\staging\roundtrip_probe.dll' setupData 1
```

`kingdomData` prüft den bisherigen Weg; `0` statt `1` startet die Vergleichsmessung ohne Patch. Ein optionaler letzter Parameter wählt Fall 0 bis 11. Die Testdateien, Profile und Rohprotokolle sind nicht im Release enthalten.

## Nutzerbestätigung vom 12. November 2026

Der Nutzer bestätigt die einfache Spielwirkung der im jüngsten Log enthaltenen Effekte: `twist_garbage_heap`, `twist_from_the_throne`, `twist_landfill`, `twist_so_much_stuff`, `twist_delayed_joy`, `twist_pennies_saved`, `twist_real_estate_barons` und `twist_head_start`. Diese Bestätigung stammt vom Nutzer; das Log allein belegt nur ihre Übernahme. Mehrfachwirkung bleibt offen. `twist_beakers` und `twist_turn_turn_turn` sind in diesem Log nicht enthalten und erhalten durch diese Rückmeldung keinen neuen Wirkungsnachweis.
