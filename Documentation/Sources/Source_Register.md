# Quellenregister

Erste Untersuchung am **9. November 2026**, Zeitzone Europe/Berlin. Die lokale Installation meldet Spielbuild **3315** und Steam-Build **17683350**, Steam-App-ID **1131620**.

## Lokale Quellen

`<Spiel>` steht für `E:\ProgrammeE\Steam\steamapps\common\Dominion`. `<Profil>` steht für `%APPDATA%\TempleGates\Dominion`.

| ID | Quelle | Prüfart und Ergebnis |
| --- | --- | --- |
| L01 | `<Spiel>\Dominion.exe` | **Statisch geprüft.** PE-Maschine `0x14C`, CLR-Datendirectory leer. Profilserialisierung, Set-Zugriff und Set-Erstellung anhand von Strings, Querverweisen und gezielter x86-Disassemblierung untersucht. |
| L02 | `<Profil>\profiles.xml` | **Lokal beobachtet.** XML gelesen und geparst. Zwei Set-Einträge mit `name`, `strKingdom` und `expansionsBitset`. |
| L03 | `<Profil>\gamelocal_*.xml` und gleichnamige `.bin`-Dateien | **Lokal beobachtet.** Die XML-Dateien haben die Wurzel `GameSetup_Save` und enthalten `kingdomData`. |
| L04 | `<Spiel>\build_number.txt` und `E:\ProgrammeE\Steam\steamapps\appmanifest_1131620.acf` | **Lokal beobachtet.** Zuordnung von Spielbuild, Steam-Build und App-ID. |
| L05 | `<Spiel>\Dominion.xpack` und `<Profil>\log.txt` | **Lokal beobachtet.** Paketheader `KCPX`, Ressourcenpfade im Index und protokollierte Paketladevorgänge. |
| L06 | Stronghold `Meine Mods\AGENTS.md`, `AGENTS\native-analysis.md` und RoN `AGENTS.md` unter `D:\CDesktopLink\Unterlagen\Mods` | **Lokal beobachtet.** Pfade zu portablem Python, Rizin, Cutter und Ghidra sowie Hinweise zum gezielten Vorgehen bei nativen Analysen. |

### Reproduktion für L01

SHA-256 von `Dominion.exe`

```text
56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877
```

Mit portablem Python und `pefile` PE-Sektionen und ImageBase (`0x00400000`) bestimmen. In den Daten nach `profiles.xml`, `DomSaveSetGet`, `saveSets`, `strKingdom` und `expansionsBitset` suchen. Die Referenzen in Code und XML-Deskriptoren verfolgen. Die folgenden Stellen wurden mit Capstone im x86-32-Modus geprüft.

| RVA | Prüfpunkt |
| --- | --- |
| `0x0D93AB` | Referenz auf `profiles.xml` im Ladepfad |
| `0x0D8AD0` | Profilspeicherpfad mit Referenz auf `%sprofiles.xml` |
| `0x1FD600` | Set-Zugriff auf das aktive Profil |
| `0x1FCE60` | Set-Erstellung mit Prüfung gegen `0x40`, Maskenberechnung und Aufruf des Profilspeicherpfads |
| `0x0E4560` | Berechnung der Erweiterungsmaske für die geparste Auswahl |

Für die Datenprüfung L02 und L03 mit einem XML-Parser öffnen und nur die Kingdom-Felder und ihre Position in der Struktur erfassen. Die Erstuntersuchung war statisch. Der [Spieltest](../Kingdom_Sets.md) steht aus.

### Deutsche Kartentitel aus L05

**Lokal extrahiert und statisch abgeglichen für Spielbuild 3315.** [Card_Titles_de.json](../Card_Titles_de.json) enthält unter `titleToId` 855 Zuordnungen vom deutschen Titel zur internen ID für Kingdom-Texte. Die Titel stammen unverändert aus `res/translation/de`. Dieselben IDs stehen in `res/translation/en-us` und als referenzierte Zeichenketten in der EXE. Alle zehn Karten-IDs aus `Generator-Test.txt` sind enthalten.

Die Liste umfasst auch Landschaften und Sonderkarten. Ob eine ID im jeweiligen Setup auswählbar ist, hängt vom Typ und Spielmodus ab. `colonies` und `shelters` sind im Testset Setup-Optionen und stehen außerhalb der Kartentiteltabelle.

SHA-256 von `Dominion.xpack`

```text
e3cf4242c2756f9120901c28f2dbdbe636b1d7235cbec9616f0c622d65d90047
```

Zur Reproduktion den KCPX-Index der Version 7 ab Dateioffset 12 in 168-Byte-Einträgen lesen. Ressourcendateioffset und Länge stehen bei Byte 12 und 16 des Eintrags als Little-Endian-`uint32`. Der nullterminierte Pfad beginnt bei Byte 40. In der Übersetzungsressource stehen der Tabellenoffset bei Byte 8 und die Anzahl der Zeilen bei Byte 16 als Little-Endian-`uint64`. Pro 40-Byte-Zeile enthalten die ersten vier `uint64` die Grenzen der beiden UTF-8-Strings relativ zum Ressourcenanfang. Die ID ist der Teil des Schlüssels hinter `dom_card_title_`.

### Karteneigenschaften aus L01 und L05

**Statisch extrahiert für Spielbuild 3315.** [Card_Properties.json](../Card_Properties.json) ordnet dieselben 855 IDs wie die deutsche Titelliste zu. Unter `cards` stehen 853 Kartendefinitionen. `harem` verweist als Alias auf `farm`. Beide lösen intern die Kartennummer `0x318` aus. Für den Lokalisierungseintrag `shadow` gibt es in den ausgewerteten Kartentabellen keine eigene Definition, daher steht dort `null`.

- `expansion`, `types` und `cost` geben die interne Erweiterungsgruppe, die Kartentypen und die Grundkosten in Münzen, Tränken und Schulden an. `cost: null` kennzeichnet Einträge ohne Kosten in diesen Einheiten. `edition` unterscheidet gemeinsame Karten und Karten der ersten oder zweiten Ausgabe.
- `kingdomPile` markiert mögliche Königreichstapel. Die Auswahl muss zusätzlich zu den freigegebenen Erweiterungen, Promos und Editionen passen. Bei geteilten und gemischten Stapeln gehören die Eigenschaften jeweils zur konkreten ID. Beispielsweise ist `knights` ein auswählbarer Stapel und `sir_martin` eine Einzelkarte daraus.
- `nativeFilters` enthält die vier ausgewerteten Spielfilter `ExtraActions`, `ExtraCards`, `ExtraBuys` und `Trashing`. Das Spiel zählt bei `ExtraBuys` unter anderem Wahloptionen wie `pawn` und `squire`.
- `printedBuyBonuses` enthält die verschiedenen ausdrücklich bezifferten Kaufboni im englischen Regeltext. `immediateBuys` enthält den beim Ausspielen beziehungsweise Auslösen sofort gewährten, ausdrücklich bezifferten Kaufbonus ohne zusätzliche Bedingung. Diese beiden Felder sind aus dem Text abgeleitet. `tactician` gibt den Kauf erst im nächsten Zug und hat daher den Wert 0 bei `immediateBuys`. `farrier` hat den Wert 1, obwohl das Spiel ihn nicht für `ExtraBuys` markiert. Indirekte Effekte und die Karten eines gemischten Stapels werden dabei nicht zusammengefasst.

| RVA in Dominion.exe | Prüfpunkt |
| --- | --- |
| `0x171770`, Sprungtabelle `0x1718B0` | Liefert Adresse und Zahl der Definitionen je interner Erweiterung. |
| `0x171A30`, `0x11B0B1` | Definitionen haben einen Abstand von `0x698` Bytes. Für das aktuelle Setup verwendet der Build Regelversion 23. |
| `0x18D7A0`, `0x190090`, `0x18FA71` | Aufbau der Zufallsauswahl und Trennung von Königreichstapeln und Landschaften. |
| `0x0DE443` bis `0x0DE4AD`, `0x18D980` | Konfigurationsnamen und `CardHasProp`. Die vier ausgewerteten Filter prüfen die Masken `0x4`, `0x10`, `0x20` und `0x2000` im Feld bei `+0x18` nach der Prüfung des Fähigkeitseintrags. |
| `0x176C00`, `0x3FCAF8` | Dekodierung der Kosten und Tabelle der Namen der Kartentypen. |

Die Kartendaten liegen teils initialisiert in der EXE und teils als Schreibzugriffe im Initialisierungscode vor. Die Extraktion liest beide Quellen. Für die aktuelle Definition muss Regelversion 23 im Intervall der Felder bei `+0x04` und `+0x08` liegen, mit exklusiver Obergrenze. Die ID-Zeichenkette steht bei `+0x58`, die numerische ID bei `+0x8C`, die Kosten bei `+0x90` und die Typbits bei `+0x98`. Die Textableitung nutzt `dom_card_power_*` aus `res/translation/en-us`. Für sofortige Käufe zählen der führende Block mit Statusboni und einzeln geprüfte Formulierungen wie die Sofortwirkung von `wharf` und `astrolabe`.

## Veröffentlichte Quellen

| ID | Quelle | Beleg |
| --- | --- | --- |
| W01 | [Temple Gates Games, Dominion-Factsheet](https://www.templegatesgames.com/press/sheet.php?p=Dominion) | **Vom Entwickler dokumentiert.** Eigene Kingdoms lassen sich zusammenstellen, speichern und teilen. |
| W02 | [Dominion auf Steam](https://store.steampowered.com/app/1131620/Dominion/) | **Vom Entwickler dokumentiert.** Produktzuordnung, unterstützte Spielmodi und Kingdom-Funktion. |
| W03 | [Community-FAQ zur Dominion-App](https://www.reddit.com/r/dominion/comments/1ahhoec/dominion_app_faq/) | **Von der Community berichtet.** Kingdom über das Burgsymbol exportieren und in das Kartensuchfeld einfügen. |
| W04 | [Community-Beispiele für Kampagneneffekte im TGG-Client](https://www.reddit.com/r/dominion/comments/1pnmsph/custom_campaign_features_in_your_kingdoms_tgg/) | **Von der Community berichtet.** Beispiele für `-x`-Parameter im normalen Kingdom-Setup. |
| W05 | [KingdomCreator, Quellstand d9a802d](https://github.com/blakevanlan/KingdomCreator/tree/d9a802db2e41d52786eaa02097279cbf1b64c778) | **Website und Quellcode geprüft.** Namensliste aus der Kopierfunktion, Web-URL-Format sowie Auswahl- und Setup-Regeln. [Befund](../Kingdom_Sets.md#web-randomizer-von-kingdomcreator) |

