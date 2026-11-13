# Sonderkarten im Kingdom-Code

## Ergebnis und Prüfstand

**Statisch geprüft am 13. November 2026:** Der Zwischenablagecode ist nicht auf zehn Hauptstapel beschränkt. Landschaften, Setup-Optionen und konkrete Zusatzkarten-Zuordnungen sind im normalen Kingdom-Text vorgesehen. Dafür ist kein nativer Patch erforderlich. Der Generator exportiert sie bisher absichtlich nicht.

Untersucht: installierte `Dominion.exe`, Spielbuild **3315**, Steam-Build **17683350**, SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Build und Hash wurden erneut gegen das [Quellenregister](Sources/Source_Register.md) geprüft. Methode: gezielte x86-Disassemblierung mit portablem Python, pefile und Capstone sowie Vergleich eingebauter Kampagnenvorlagen. Kein neuer Spiel- oder Suchfeldimporttest; Installation und Profile wurden nicht verändert.

## Schreibweisen

Alle Karten verwenden interne IDs. Die zehn Hauptstapel bleiben eine eigene Auswahl; Zusatzkarten werden passend zu ihrer Rolle ausgegeben.

| Auswahl | Darstellung | Beleg |
| --- | --- | --- |
| Ereignis, Landmarke, Projekt, Weg | ID nach den Hauptstapeln, beispielsweise `canal`, `basilica`, `way_of_the_owl` | Nativer Kartenlistenparser und zahlreiche Kampagnenvorlagen. |
| Platin und Kolonien einschalten | `colonies` im `-k`-Teil | Nativer Parser; zehn Wohlstand-Kampagnenvorlagen. Gemeinsame Setup-Option, nicht zwei Hauptstapel. |
| Platin und Kolonien ausschalten | `-m NoColonies` | Nativer Modifikatorparser schreibt den expliziten Aus-Zustand. |
| Unterschlüpfe einschalten | `shelters` im `-k`-Teil | Nativer Parser und Dark-Ages-Kampagnenvorlagen. |
| Unterschlüpfe ausschalten | `-m NoShelters` | Nativer Modifikatorparser schreibt den expliziten Aus-Zustand. |
| Bannstapel | `young_witch(village)` statt `young_witch` | Eingebaute Kampagnenvorlage. |
| Fährmann-Zusatzstapel | `ferryman(shop)` statt `ferryman` | Eingebaute Kampagnenvorlage. |
| Flussboot-Karte | `riverboat(imperial_envoy)` | Eingebaute Kampagnenvorlage. |
| Obelisk-Ziel | `obelisk(patrician)` | Eingebaute Kampagnenvorlage. |
| Merkmal-Ziel | beispielsweise `nearby(jewelled_egg)` | Eingebaute Kampagnenvorlage; mehrere Merkmale kommen gemeinsam vor. |
| Druiden-Gaben | `druid(the_moons_gift, the_suns_gift, the_mountains_gift)` | Eingebaute Kampagnenvorlage. Kommas innerhalb der Klammern gehören zur Zuordnung. |
| Verbündeter | ID direkt im `-k`-Teil, beispielsweise `order_of_masons` | Eingebaute Kampagnenvorlage mit Verbindungskarten. |
| Prophezeiung | ID direkt im `-k`-Teil, beispielsweise `enlightenment` | Eingebaute Kampagnenvorlage mit Omenkarten. |

`-m Colonies` und `-m Shelters` existieren ebenfalls als Einschalter. Für den Export bietet sich die bereits vom Spiel verwendete `-k`-Schreibweise an; die Aus-Zustände müssen ausdrücklich mitgegeben werden. Bloßes Weglassen setzt keinen nachgewiesenen Aus-Zustand und kann die native Zufallsentscheidung offenlassen.

### Landschaftszahl

`-m Landscapes(N,N)` kann die Gesamtquote auf N festlegen. Der Modifikatorparser liest ein einzelnes Argument als gleiche Unter- und Obergrenze und zwei Argumente als getrennte Grenzen. `Landscapes(0,N)`, das der Namenskonverter bisher ergänzt, bedeutet deshalb keine exakte Anzahl.

Der Kartenlistenparser besitzt vier Plätze für als Landschaft erkannte Einträge und zehn Hauptstapelplätze. Die Generatoroberfläche beziehungsweise Exportvalidierung muss diese native Kapazität berücksichtigen; zusätzliche Verbündete, Prophezeiungen und Merkmale dürfen nicht unbesehen als unbegrenzt verfügbare weitere Plätze behandelt werden. Die genaue gemeinsame Belegung und das Verhalten des Suchfelds bei Grenzfällen benötigen einen Importtest.

### Kopierbares Beispiel

Zehn Hauptstapel, das Projekt Kanal, Platin/Kolonien und ausdrücklich keine Unterschlüpfe:

```text
-k cellar, chapel, moat, village, workshop, smithy, remodel, market, mine, militia, canal, colonies -m Landscapes(1,1), NoShelters
```

Dies ist eine aus der statisch geprüften Grammatik abgeleitete Testeingabe, kein bereits im Spiel abgenommener Export.

## Grenzen und weitere Sonderkarten

- Für Weg der Maus und Nahende Armee ist `way_of_the_mouse(ziel)` beziehungsweise `approaching_army(ziel)` der anhand des allgemeinen Zuordnungspfads naheliegende Export. Die konkrete Zielübernahme muss noch separat geprüft werden; in den ausgewerteten Kampagnenstrings fand sich dafür kein direktes Beispiel.
- Der Parser besitzt einen besonderen Zusatzkartenpfad mit einem 60-Einträge-Puffer und einer Sonderbehandlung für Karten-ID `0xD00`. Ein vollständiges Schwarzmarktdeck sollte erst nach eigener Zuordnungs- und Rundreiseprüfung exportiert werden. Aus dem allgemeinen Klammerformat allein folgt noch keine abgesicherte Schwarzmarktunterstützung.
- Pferde, Beute, Ruinen, Erbstücke, Reisende, Mitglieder gemischter Stapel und vergleichbares Material nicht pauschal anhängen. Solches Material folgt den ausgewählten Karten; die Generatorliste `components` mischt Material und ausdrücklich gewählte Zusatzkarten. Sie ist keine fertige Exportliste.
- Die Syntax umgeht keine Erweiterungsfreigaben. Eine akzeptierte ID, ihre Übernahme im Suchfeld und ein korrekt aufgebautes Spiel sind getrennte Nachweise.

## Ursache im Generator und mögliche Umsetzung

`showDraw()` in [Dominion-Generator.html](../SetGenerator/Dominion-Generator.html) setzt die Ausgabe derzeit ausschließlich aus den ersten zehn Hauptstapel-IDs zusammen. `assignSetup()` in HTML und [generator_solver.js](../SetGenerator/generator_solver.js) liefert bereits `main`, `landscapes`, `assignments`, `withColonies` und `withShelters`. Die beiden Booleans enthalten das konkrete Ziehungsergebnis und sollten beim Import nicht neu ausgewürfelt werden.

Ein vollständiger Export kann daraus aufgebaut werden:

1. Hauptstapel-IDs ausgeben und bei vorhandenen Zuordnungen durch `quelle(ziel, …)` ersetzen.
2. Gewählte Landschaften mit ihren eventuellen Zielzuordnungen ergänzen; Verbündeten und Prophezeiung aus den zugehörigen Assignments übernehmen und Duplikate vermeiden.
3. `colonies` und `shelters` für eingeschaltete Optionen ergänzen, andernfalls `NoColonies` und `NoShelters` im gemeinsamen `-m`-Abschnitt setzen.
4. Landschaftsquote passend zum bestätigten nativen Zählverhalten setzen. Kapazitätsüberschreitungen und noch ungeprüfte Sonderfälle sichtbar melden.
5. Den Export nativ parsen, serialisieren und erneut parsen; anschließend das normale Suchfeld und einen lokalen Spielaufbau prüfen. Erst dann die betreffende Ausgabe als vollständigen Code bezeichnen.

### Umsetzung im Generator am 13. November 2026

`buildKingdomCode()` erzeugt jetzt die erweiterte Ausgabe aus dem Ziehungsergebnis. Unterstützt sind die oben belegten Zuordnungen, Verbündete, Prophezeiungen und die konkreten Setup-Schalter. Weg der Maus und Nahende Armee nutzen das allgemeine Klammerformat; der Hinweis auf ausstehende Steam-Importtests bleibt bestehen. Verschachtelte Zusatzkarten-Zuordnungen werden rekursiv ausgegeben.

Die Ausgabe setzt `Landscapes(N,N)` für die exportierten gewöhnlichen Landschaften einschließlich Merkmale; Verbündete und Prophezeiungen folgen ihren eigenen Setup-Regeln. Insgesamt werden höchstens vier Landschaftseinträge ausgegeben. Überzählige Einträge, Schwarzmarktdeck-Auswahlen und Ziele, die nur im Planungsbereich jenseits der ersten zehn Hauptstapel liegen, werden unter dem ausdrücklich bezeichneten Steam-Teilcode genannt. Die Verdopplungsausgabe bleibt auf die zehn Hauptstapel bezogen.

**Automatisiert geprüft:** `node SetGenerator/test_kingdom_export.js`, `node SetGenerator/test_generator_solver.js`, `node SetGenerator/test_name_converter.js` und `python SetGenerator/test_generator_catalog.py`. Die Exporttests führen die echten HTML-Funktionen aus und prüfen Zusatzoptionen, Landschaften, verschachtelte Zuordnungen, Verbündete/Prophezeiungen, Kapazitätsgrenzen, Planungsziele und die sichtbaren Ausgabehinweise in beiden Oberflächensprachen. Ein neuer Import- und Spielaufbautest in Dominion steht weiterhin aus.

## Native Prüfstellen

Alle Angaben sind **RVAs im Modul Dominion.exe**, keine virtuellen Adressen oder Dateioffsets; ImageBase `0x00400000`, Hash wie oben.

| RVA | Beobachtung |
| --- | --- |
| `0x0DDBB0` | Setup-Parser; Eingaben mit führendem Minus gehen in den Optionsparser. |
| `0x0DE780` | Optionsparser; `-k` ruft den Kartenlistenpfad, `-m` den Modifikatorpfad. |
| `0x0DD810` | Kartenlistenparser; erkennt `colonies` und `shelters` gesondert. Kartentypen entscheiden zwischen Hauptstapel und Landschaft. |
| `0x0DD943`, `0x0DDB56` | Kapazitätsvergleiche gegen vier Landschaften beziehungsweise zehn Hauptstapel. |
| `0x0DD660`, `0x0DD440` | Verarbeitung von Klammerargumenten und Zusatzkarten. |
| `0x0DE120` | Modifikatorparser; `Colonies`, `NoColonies`, `Shelters`, `NoShelters`, Landschafts- und Typquoten. |
| `0x0DE040` | Zahlenpaar: ein Argument wird zu beiden Grenzen, zwei zu Unter- und Obergrenze. |
| Setup `+0xBFC`, `+0xC00` | Kolonien- beziehungsweise Unterschlupfoption: Einschalter schreibt 1, Ausschalter 2. Das sind Struktur-Offsets. |
| `0x0DF350`, `0x0DF150` | Ausgabe einer Karten-ID mit ihren Klammerargumenten. |
| `0x0DFB20` | Ausgabe der Setup-Optionen und Modifikatoren. |
| `0x40E0C6`, `0x40F2BD` | Eingebaute `-k`-Beispiele mit `colonies` beziehungsweise `shelters`. |
| `0x410E4D`, `0x412009`, `0x41260B`, `0x412D9D` | Vorlagen mit Druiden-Gaben, Verbündetem, Merkmalen beziehungsweise Prophezeiung. |
