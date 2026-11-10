# Eigene Kingdoms speichern und laden

## Lokale Speicherung

**Lokal beobachtet.** Zwei gespeicherte Kingdoms lagen in `%APPDATA%\TempleGates\Dominion\profiles.xml`. Der XML-Pfad lautet `/ProfileSettings_Save/profiles/saveSets/sets`. Pro Set gibt es einen wiederholten `sets`-Knoten mit `name`, `strKingdom` und `expansionsBitset`. Der Name im Beispiel wurde ersetzt. [Quellen L01 und L02](Sources/Source_Register.md)

```xml
<saveSets>
  <sets>
    <name>Beispiel</name>
    <strKingdom>pawn, market, ironworks, expand, workers_village, throne_room, lurker, steward, bridge, nobles, colonies, shelters</strKingdom>
    <expansionsBitset>38</expansionsBitset>
  </sets>
</saveSets>
```

`strKingdom` enthält kommagetrennte IDs. Im zweiten lokalen Set standen außerdem die Vorsilbe `-k` und der Zusatz `-m Landscapes(0,4)`. Das sind beobachtete Formen. Die vollständige Grammatik und die Bedeutung aller Modifikatoren sind offen. `gamelocal_*.xml` mit Wurzel `GameSetup_Save` und die zugehörigen `.bin`-Dateien gehören zu einzelnen lokalen Partien. Deren XML kann ein Feld `kingdomData` enthalten. [Quellen L02 und L03](Sources/Source_Register.md)

Die zehn Hauptstapel und zusätzliche Vorratsstapel durch Kampagneneffekte sind getrennte Teile des Setups. Im lokalen Test blieb ein gespeichertes Set mit `twist_room_for_more(chapel)` bei zehn Stapeln. Die achte Füllhorn-Mission enthält eine Vorlage für zwölf Vorratsstapel. [Spielanleitung](Steam_Spielanleitung.md#zwölf-oder-mehr-königreichstapel), [Technischer Befund zur Kapazität](Doppelte_Koenigreichstapel_Mod.md#befund)

## Nativer Profilpfad

**Statisch geprüft für Spielbuild 3315.** Die Profilinitialisierung verwendet `profiles.xml`. Die XML-Serialisierungsdeskriptoren der EXE verbinden das Profilfeld `saveSets` mit den Set-Daten. Der über den Diagnosestring `DomSaveSetGet` identifizierte Pfad greift auf die Set-Liste des aktiven Profils zu. Beim Anlegen eines Sets vergleicht der Code die Anzahl mit `0x40` und nimmt den Eintrag nur bei weniger als 64 vorhandenen Sets an. Vor dem Schreiben des Profils berechnet der Code `expansionsBitset` aus der geparsten Kingdom-Auswahl. Der Dezimalwert `38` oben gehört zum konkreten Beispiel. [Quelle L01](Sources/Source_Register.md)

**Reines Verdopplungsset, statisch geprüft für Spielbuild 3315.** Für einen `strKingdom` mit ausschließlich `-x start_double_pile(…)`-Einträgen ergibt sich `expansionsBitset = 0`. Der Parser speichert das Ziel im Effekteintrag (RVA `0x0E6148`). Die Maskenroutine (RVA `0x0E4560`) liest Hauptstapel, Landschaften, Zusatzkarten und gegebenenfalls Erweiterungsfilter aus der Kingdom-Auswahl. Diese Felder bleiben bei dieser Eingabe leer. Die Maske des später geladenen Königreichs wird aus dessen eigener Auswahl berechnet. [Quelle L01](Sources/Source_Register.md)

**Prüfstand.** Manuell ergänzte Verdopplungssets wurden lokal im Spiel verwendet. Der Export aus dem HTML-Konverter wurde im Browser mit Testdaten geprüft. Die Übernahme eines vom Konverter ergänzten Eintrags in Dominion und die Online-Synchronisierung stehen noch aus. Für einen allgemeinen Profilexport müssen die Maskenberechnung und die relevanten Setup-Optionen für die vom Generator unterstützten Karten bestätigt werden.

## Zwischenablage

**Vom Entwickler dokumentiert.** Die App unterstützt eigene, speicherbare und teilbare Kingdoms. **Von der Community berichtet.** Ein Rechtsklick auf das Burgsymbol in einer Partie bietet den Export in die Zwischenablage. Der Text lässt sich anschließend in das Kartensuchfeld der Kingdom-Konfiguration einfügen. In der EXE sind außerdem `exportToClipboard` und Windows-Zwischenablageaufrufe vorhanden. Für den vorbereiteten `Generator-Test.txt` hat der Nutzer bestätigt, dass das Einfügen funktioniert. Weitere Eingabeformen und der Profilexport sind getrennte Tests. [Quellen L01, W01 und W03](Sources/Source_Register.md)

## Web-Randomizer von KingdomCreator

**Geprüft am 9. November 2026.** Die verlinkte Website meldet Version 7.0.7. Ihr Copy-Button kopiert eine kommagetrennte Liste lokalisierter Kartentitel, im Beispiel beginnend mit `Cellar, Chancellor, Feast`. Der Quellcode hängt gegebenenfalls die Titel der Landschaften, Gaben, des Verbündeten und der Prophezeiung an. Die Kopierfunktion erzeugt keinen eigenen Export im `-k`-Format und lässt Zuordnungen wie die Bane-Karte oder das Ziel eines Traits weg. Ob der Steam-Client die unformatierte Namensliste dennoch übernimmt, wurde nicht im Spiel geprüft. [Beispiel-Link](https://dominionrandomizer.com/?lang=en&supply=cellar,chancellor,feast,festival,gardens,market,moat,remodel,thief,woodcutter), [Zusammenstellung der Namensliste](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/components/randomize/Randomizer.vue#L84-L97), [Copy-Button](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/components/CopyButton.vue#L39-L46)

Der teilbare Web-Link speichert dagegen Kurz-IDs in `supply` und Extras in eigenen Parametern, etwa `bane`, `riverboat`, `ways`, `colonies` und `shelters`. Für einen späteren Import die URL dekodieren, Kurz-IDs explizit auf TGG-IDs abbilden und die Auswahl gegen [Card_Properties.json](Card_Properties.json) prüfen. Beispielsweise entspricht `youngwitch` dem lokalen `young_witch`. Unbekannte, doppelte oder für den gewählten Pool gesperrte Karten sowie noch nicht unterstützte Zusatzparameter melden, damit sich das exportierte Setup nicht stillschweigend ändert. [Web-Serializer und Beispiel](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/randomizer/serializer.ts#L15-L150)

Alle zehn IDs aus dem konkreten Beispiel sind im lokalen Katalog für Build 3315 vorhanden. Daraus ergibt sich diese Zeile im bereits mit einem anderen Kingdom getesteten Format. `chancellor`, `feast`, `thief` und `woodcutter` gehören zur ersten Ausgabe des Basisspiels. Die Übernahme dieses Beispielsets mit diesen vier Karten steht noch aus.

```text
-k cellar, chancellor, feast, festival, gardens, market, moat, remodel, thief, woodcutter
```

### Web-Namenslisten im Konverter

[KingdomCreator_Card_Names.json](KingdomCreator_Card_Names.json) enthält 1.133 Website-Einträge aus dem bereitgestellten Kartenkatalog und die lokalisierten Titel der sieben Website-Sprachen. Unter `cards` steht je Website-ID die TGG-ID, die Rolle und die Namensliste. Fehlt eine Übersetzung, nutzt der Konverter wie die Website den englischen Namen. Von den 1.133 Einträgen können 885 in der kopierten Liste vorkommen. Sie decken 703 verschiedene TGG-IDs ab, darunter alle 498 Königreichstapel aus dem lokalen Katalog einschließlich erster Editionen. Die übrigen Einträge stammen aus dem Boxinhalt, etwa Marker und Spieltableaus. Ohne Entsprechung im lokalen Katalog steht ihre TGG-ID auf `null`.

Der Abgleich nutzt den englischen Namen, die Kartenrolle und bei sechs geteilten Stapeln eine feste Zuordnung zum TGG-Stapel. Zum Beispiel wird `Sauna / Avanto` als `sauna` ausgegeben. Doppelte Website-Einträge aus verschiedenen Editionen dürfen auf dieselbe TGG-ID zeigen. Zeigt derselbe kopierte Name auf mehrere TGG-IDs, bricht die Umwandlung ab. Ein Beispiel ist das französische `Courtier`, das auf der Website sowohl für `courtier` als auch für `broker` vorkommt. [Ausgelieferter Kartenkatalog](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/docs/dominion-content.js), [Sprachdateien](https://github.com/blakevanlan/KingdomCreator/tree/d9a802db2e41d52786eaa02097279cbf1b64c778/docs/locales), [Quellprojekt und Lizenz](https://github.com/blakevanlan/KingdomCreator/tree/d9a802db2e41d52786eaa02097279cbf1b64c778)

Der [Konverter](../SetGenerator/README.md#web-namenslisten-umwandeln) übernimmt die Reihenfolge der zehn Hauptstapel und erkennt danach Ereignisse, Landmarken, Projekte und Wege ohne weitere Zuordnung. Verbündete, Prophezeiungen, Gaben, Traits und Karten mit besonderem Setup meldet er als derzeit nicht exportierbar. Kolonien und Unterschlüpfe stehen nicht in der kopierten Namensliste und können beim Aufruf ausdrücklich ergänzt werden. Die Zuordnung und das Format wurden mit Beispielen aus allen sieben Sprachen geprüft. Das Einfügen der neu erzeugten Ausgaben in Build 3315 steht noch aus.

### Optionale Regeln für die Ziehung

| Regel | Umsetzung mit unseren Daten |
| --- | --- |
| Mindestens ein Aktionsgeber, Mehrfachzieher, Kaufgeber oder Entsorger | Je Rolle eine Mindestanzahl auf den zehn Hauptstapeln prüfen. KingdomCreator verwendet dafür eigene Tags und trennt einfachen Kartenzug von Mehrfachzug. Unsere `nativeFilters` bieten ähnliche Kategorien. Für präzise Regeln wie mindestens zwei Aktionen oder Karten ohne Bedingung braucht es getrennt geprüfte Effektdaten wie beim bestehenden `immediateBuys`. |
| Angriffe erlauben, begrenzen oder eine Abwehr verlangen | Angriffe über `types` zählen. KingdomCreator fordert optional bei Angriffen eine beliebige Reaktionskarte. Der Generator nutzt für Angriffsschutz das geprüfte Merkmal `defendsAgainstAttacks`. [Prüfung der Kartentexte](Abwehrkarten.md) |
| Kosten verteilen | Optional beispielsweise drei bis fünf teure Stapel fordern. KingdomCreator setzt die Grenze bei einem Kostenwert von 5 und gewichtet Münzen, Schulden und Tränke mit 1, 1 und 2. Für unseren Generator die Kostenmetrik wählbar halten, da die drei Währungen im Spiel verschiedene Bedeutungen haben. |
| Erweiterungen priorisieren und Alchemie bündeln | Eine Mindest- oder Höchstzahl je Erweiterung vorgeben. KingdomCreator reserviert bei Priorisierung fünf Plätze. Für Alchemie bietet er beim Mischen drei bis fünf Stapel oder keine an. |
| Karten fixieren oder ausschließen | Fixierte Stapel bei der Neuziehung behalten und auf Quoten anrechnen. Ausschlüsse vor der Ziehung anwenden. Wenn feste Karten oder mehrere Quoten einander widersprechen, den Konflikt melden. |

Die Regeln der Website sind Auswahloptionen. Als Vorgaben für unseren Generator lassen sie sich getrennt aktivieren und gemeinsam prüfen. [Ziehregeln im Quellcode](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/randomizer/randomizer.ts#L201-L293), [Kartentags](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/dominion/card-type.ts#L1-L24), [Kostenmetrik](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/randomizer/cost-supply-divider.ts), [Dark-Ages-Regelheft](https://www.riograndegames.com/wp-content/uploads/2022/03/Dominion-Rules-DarkAges.pdf)

### Setup-Abhängigkeiten für konkrete Karten

Die zehn Hauptstapel bleiben eine eigene Liste. Zusätzliche Vorratsstapel, Karten neben dem Vorrat, beiseitegelegte Einzelkarten und Landschaften haben getrennte Rollen. Bei exakten Münzkosten für Zusatzkarten auch `potions == 0` und `debt == 0` prüfen. Die Regeln unten beschreiben das Setup. Die Eingabesyntax für diese Zuordnungen im TGG-Client muss separat geprüft werden. [Zusatzkarten im Web-Quellcode](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/randomizer/special-need-cards.ts), [Kostenregeln in Rising Sun](https://www.riograndegames.com/wp-content/uploads/2024/04/DomRisingSunRules.pdf#page=3)

| Auslöser | Zusätzliche Auswahl |
| --- | --- |
| `young_witch` | Ungenutzter Königreichstapel für 2 oder 3 Münzen als Bane-Stapel im Vorrat. |
| `ferryman` | Ungenutzter Königreichstapel für 3 oder 4 Münzen neben dem Vorrat. |
| `riverboat` | Ungenutzte Aktionskarte ohne Dauer-Typ für genau 5 Münzen beiseitelegen. |
| `way_of_the_mouse` | Ungenutzte Aktionskarte ohne Dauer-Typ für 2 oder 3 Münzen beiseitelegen. |
| `obelisk` oder ein Trait | Obelisk wählt einen vorhandenen Aktionsstapel. Ein Trait wählt einen vorhandenen Aktions- oder Geldstapel. Pro Stapel höchstens ein Trait. |
| Mindestens eine Liaison oder ein Omen | Bei Liaison einen Verbündeten wählen. Bei Omen eine Prophezeiung wählen. `approaching_army` braucht zusätzlich einen ungenutzten Angriffsstapel im Vorrat, auch wenn einer der zehn Hauptstapel bereits ein Angriff ist. |
| `druid` | Drei verschiedene Gaben beiseitelegen. |

Platin und Kolonien gehören zusammen. Sie und die Unterschlüpfe sind Setup-Optionen und werden getrennt von den Königreichstapeln gespeichert. Die Regelhefte empfehlen für gewöhnliche Landschaften wie Ereignisse und Traits insgesamt höchstens zwei. Verbündete und Prophezeiungen folgen ihren eigenen Auslösern. [Cornucopia & Guilds](https://www.riograndegames.com/wp-content/uploads/2024/02/CornucopiaGuilds.pdf#page=2), [Rising Sun](https://www.riograndegames.com/wp-content/uploads/2024/04/DomRisingSunRules.pdf#page=2), [Menagerie](https://www.riograndegames.com/wp-content/uploads/2020/01/DominionMenagerie.pdf), [Empires](https://www.riograndegames.com/wp-content/uploads/2022/03/Dominion-Rules-Empires.pdf), [Plunder](https://www.riograndegames.com/wp-content/uploads/2022/08/DomPlunder.pdf#page=2), [Nocturne](https://www.riograndegames.com/wp-content/uploads/2022/03/Dominion-Rules-Nocturne.pdf), [Verbündete und Prophezeiungen im Web-Quellcode](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/randomizer/randomizer.ts#L407-L426)

## Geplanter Offline-Test

1. Dominion beenden und `profiles.xml` bytegetreu sichern. Im aktiven Profil den vorhandenen Set-Eintrag mit `pawn` als `Generator-Test` duplizieren. Nur in der Kopie `pawn` durch `courtyard` ersetzen. Beide Karten stammen aus Intrige. Die übrigen Karten, Einstellungen und die beobachtete Maske beibehalten. Die bearbeitete Datei als XML prüfen.
2. Dominion offline starten, `Generator-Test` in der Liste der gespeicherten Sets auswählen und eine lokale Partie beginnen. Prüfen, ob `courtyard` anstelle von `pawn` im Vorrat liegt und das Originalset weiterhin auswählbar ist.
3. Den Kingdom-Text aus der Testpartie exportieren und in einer frischen Spielkonfiguration in das Kartensuchfeld einfügen. Die angezeigte Kartenauswahl mit der Testpartie vergleichen. Nach einem Neustart offline erneut prüfen, ob `Generator-Test` auswählbar ist.
4. Dominion beenden und die Originaldatei aus der Sicherung wiederherstellen. Build, sichtbare Sets, Karten im Vorrat, Zwischenablageergebnis und Verhalten nach Neustart protokollieren.
