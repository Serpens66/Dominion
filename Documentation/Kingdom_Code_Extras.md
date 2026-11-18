# Sonderkarten im Kingdom-Code

## Ergebnis und Prüfstand

**Statisch geprüft am 13. November 2026:** Der Zwischenablagecode ist nicht auf zehn Hauptstapel beschränkt. Landschaften, Setup-Optionen und konkrete Zusatzkarten-Zuordnungen sind im normalen Kingdom-Text vorgesehen. Dafür ist kein nativer Patch erforderlich. Der Generator exportiert sie bisher absichtlich nicht.

Untersucht: installierte `Dominion.exe`, Spielbuild **3315**, Steam-Build **17683350**, SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Build und Hash wurden erneut gegen das [Quellenregister](Sources/Source_Register.md) geprüft. Methode: gezielte x86-Disassemblierung mit portablem Python, pefile und Capstone sowie Vergleich eingebauter Kampagnenvorlagen. Kein neuer Spiel- oder Suchfeldimporttest; Installation und Profile wurden nicht verändert.

> **Nachtrag vom 18. November 2026 – statisch geprüfte Importgrenze:** Der normale Suchfeld- und Zwischenablageimport übernimmt Erweiterungsfilter sowie Platin/Kolonien- und Unterschlupfoptionen nicht aus der geparsten Eingabe. Dass die unten beschriebene Syntax im nativen Parser vorhanden ist, belegt daher keine Übernahme dieser Einstellungen durch die Oberfläche. Siehe [Import-Zusammenführung](#erweiterungen-und-aufbauoptionen-beim-einfügen).

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


## Erweiterungen und Aufbauoptionen beim Einfügen

**Statisch geprüft am 18. November 2026:** Spielbuild **3315**, Steam-Build **17683350**, EXE-SHA-256 erneut geprüft und unverändert `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Die Prüfung las die installierte Datei; Spiel, Profile und Zwischenablage wurden nicht verändert. Kein neuer Spieltest.

Der Optionsparser bei RVA `0x0DE780` erkennt tatsächlich `-e`. Sein Zweig bei `0x0DE8C8` ruft den Erweiterungsfilterparser `0x0DDD90` auf. Dieser schreibt Einträge mit Erweiterungs-ID, Edition und Von-bis-Quote ab Setup-Offset `+0x9C8`. Das ist vom Kartenzugriff beziehungsweise Besitz einer Erweiterung getrennt.

Der normale Einfügeweg übernimmt jedoch nicht die gesamte geparste Konfiguration:

| RVA in Dominion.exe | Statischer Befund |
| --- | --- |
| `0x1FDF40`, `0x1FDFF0` | UI-Ereignis 9 führt zum Zwischenablage-Helfer `0x22BEA0`. |
| `0x22BEDF`, `0x23C340` | Der Helfer liest die Windows-Zwischenablage; der Lesepfad verwendet unter anderem `CF_UNICODETEXT` (13). |
| `0x22BF93` | Der gelesene Text wird in eine temporäre Setup-Konfiguration geparst. |
| `0x1FE00B`, `0x1FE032` | Aktuelle Konfiguration über `0x0E49D0` holen und temporäre Auswahl über `0x1F6A90` einmischen. |
| `0x1F6DA0`, `0x1F6F17`, `0x1F6F2F` | Auch die Suchfeldverarbeitung parst in eine temporäre Auswahl und ruft dieselbe Zusammenführung auf. |
| `0x1F6A90` bis `0x1F6D6E` | Die Zusammenführung übernimmt Hauptstapel, Landschaften mit Zielzuordnungen, Zusatzkarten und Effekte. Die Erweiterungsfilter ab `+0x9C8` sowie die Setup-Optionen `+0xBFC` (Platin/Kolonien) und `+0xC00` (Unterschlüpfe) werden nicht aus der temporären Konfiguration kopiert. |
| `0x1FE042`, `0x1FE062` | Anschließend die bestehende Konfiguration ausgeben und die Ansicht aktualisieren; im geprüften Einfügepfad keine zusätzliche Erweiterungsaktivierung. |

Die Hauptstapel-Zusammenführung bei `0x1F6410` besitzt einen separaten Pfad für Erweiterungs-Zufallsplätze (Deskriptortyp 2). Dieser aktualisiert bei `0x1F6606` bis `0x1F667B` Quoten in der aktuellen Konfiguration. Er ist kein Ersatz für die Übernahme von `-e`: Ein solcher Zufallsplatz verändert die Hauptstapelauswahl. Bei zehn ausdrücklich vorgegebenen Hauptstapeln ist das kein belegter zusätzlich anhängbarer Aktivierungsbefehl. Der Pfad für konkrete Karten ab `0x1F6688` prüft die Kartenfreigabe, schreibt aber keine entsprechende Erweiterungsauswahl.

**Folgerung:** Ein zusätzliches `-e` ist für diese normalen Importwege keine belegte automatische Erweiterungsaktivierung. Ebenso sind `colonies`, `shelters`, `NoColonies` und `NoShelters` zwar vom Parser verstandene Angaben, ihre Übernahme durch diesen UI-Import ist damit aber gerade nicht abgesichert. Bis zu einer gesonderten Lösung Erweiterungsauswahl und Aufbauoptionen im Spiel einstellen beziehungsweise nach dem Einfügen kontrollieren. Die Parserfähigkeit und der tatsächlich gestartete Spielaufbau müssen getrennt getestet werden.

Gespeicherte Profilsets besitzen daneben ein eigenes Feld `expansionsBitset`; der Befund zum Suchfeld darf nicht ungeprüft auf das Laden solcher Sets übertragen werden. Ob ein entsprechend erzeugtes Set die gewünschte Menüauswahl vollständig setzt, bleibt ein eigener Spieltest. Ein Patch könnte die beim Einfügen ausgelassenen Felder gezielt übernehmen; das wäre eine separate Änderung mit Prüfung der bestehenden Kartenfreigabe.

Beim konkreten Nutzer-Code stammt Einladung aus der ID `summon`. Der lokale Katalog ordnet die ausgewählten Karten Basisspiel, Blütezeit (`prosperity`), Menagerie und Promos (`summon`) zu. `colonies` ist die gemeinsame Syntax für Platin und Kolonien und benötigt kein zusätzliches `platinum`-Token.


### Gemeinsamer Code mit optionaler Patch-Unterstützung

**Zusätzlich statisch geprüft am 18. November 2026, derselbe Build und EXE-Hash:** Ein korrekt formatierter `-e`-Abschnitt bricht den nativen Optionsparser nicht ab. Nach dem Aufruf des Erweiterungsfilterparsers bei RVA `0x0DE8D7` springt `0x0DE8DF` zur gemeinsamen Schleifenfortsetzung `0x0DEAFD`. Diese erhöht den Optionsindex und verarbeitet bei `0x0DEB0C` weitere Abschnitte. Nachfolgende `-k`-, `-m`- oder andere gültige Abschnitte werden daher nicht allein wegen `-e` abgeschnitten.

Damit ist für Build 3315 ein gemeinsamer Generatorcode grundsätzlich möglich: Vanilla versteht `-e` syntaktisch, übernimmt dessen Erweiterungsfilter beim normalen UI-Import aber nicht; ein entsprechend ergänzter Patch kann genau diese bereits geparsten Felder berücksichtigen. Eine eigene Kopierzeile nur für Patchnutzer ist hierfür nicht erforderlich. Vor der Freigabe des erweiterten Exports die konkrete Ausgabe mit und ohne Patch nativ parsen und über beide UI-Importwege bis zum lokalen Spielstart testen. Diese Aussage ist kein neuer Laufzeitnachweis und keine Zusage über unbekannte Parameter oder fehlerhafte Syntax.


### Nutzer-Spielbefund: Einladung fehlt wegen Kartenfreigabe

**18. November 2026, Build 3315:** Der Nutzer testete den Beispielcode mit zehn Hauptstapeln, `way_of_the_rat`, `way_of_the_worm`, `summon`, `delay`, `colonies`, `-e base, prosperity, menagerie, promos` und `-m Landscapes(4,4), NoShelters`. Laut Nutzer wurde vermutlich Vanilla gestartet; Platin und Kolonien waren vorhanden, aber nur drei Landschaften. Der Prozess war bei der anschließenden Prüfung bereits beendet, deshalb ist der ungepatchte Zustand nicht nachträglich über geladene Module bestätigt.

Die zugehörige lokale Partiedatei wurde ausschließlich gelesen. Ihr `setupData` enthält bereits nur die drei Landschaften `way_of_the_rat`, `way_of_the_worm` und `delay`; `summon` fehlt dort ebenso wie im aufgelösten `kingdomData`. `delay` hinter dem ursprünglichen `summon` bleibt erhalten. Die Hauptstapel aus Basisspiel, Blütezeit und Menagerie sind im aufgelösten Ergebnis vorhanden, während der gespeicherte Zufallspool nur `-e base` enthält. Das unterscheidet Zufallspool-Aktivierung von der Freigabe konkreter Karten.

Der Nutzer bestätigte anschließend, dass Einladung auch manuell im Spiel nicht auswählbar ist, und vermutet fehlenden Besitz. **Belegt ist die fehlende manuelle Auswählbarkeit, nicht ein unabhängig ausgelesener Kaufstatus.** Der Befund spricht damit für das Herausfiltern einer nicht freigegebenen Karte, nicht für eine Abbruchstelle des Codes bei `-e` oder hinter `summon`. Im Generator kann diese einzelne Promo-Karte ausgeschlossen werden, ohne alle Promos zu sperren.

Die vorhandenen Platin-/Kolonienstapel beweisen für sich keine Übernahme des `colonies`-Schalters: Eine bestehende Option oder reguläre Auswahl bei Blütezeit bleibt als Erklärung offen. Ein kontrollierter Vergleich mit vorher explizit ausgeschalteter Option steht aus. Rohprofile und Partiedateien wurden nicht ins Repository übernommen.


### Nutzer-Test: ein fester Keller und neun Zufallsplätze

**18. November 2026, Nutzerbericht und lokale Partiedateien:** Als gezielter Vergleich wurden `-k cellar` sowie `-k cellar -e menagerie(9,9)` bei zuvor nur aktiviertem Basisspiel und geleerter Kartenauswahl vorgeschlagen. Der Nutzer berichtet zum Code mit `-e`, dass keine Menagerie-Karten ins Spiel kommen.

Die anschließend ausschließlich gelesenen beiden neuesten lokalen Partiedateien enthalten jeweils `setupData = -e base -k cellar`. Ihre aufgelösten Königreiche bestehen aus Keller und neun weiteren Hauptstapeln, die der lokale Katalog sämtlich dem Basisspiel zuordnet. Der angeforderte Menagerie-Filter ist in diesen gespeicherten Konfigurationen nicht vorhanden. Welche dieser beiden Dateien die Kontrollrunde beziehungsweise den erweiterten Import darstellt, lässt sich aus den identischen Setup-Texten allein nicht unterscheiden. Die Stellung der Menagerie-Häkchen wurde nicht separat gemeldet; eine Prüfung der geladenen Module zum Nachweis eines ungepatchten Prozesses erfolgte nicht.

**Ergebnis:** Im gemeldeten Test wurde die Erweiterungsquote aus `-e menagerie(9,9)` nicht für die neun Zufallsplätze übernommen. Das passt zum statischen Befund der verworfenen Erweiterungsfilter bei der UI-Zusammenführung. Eine vollständig festgelegte Auswahl besessener Karten kann trotzdem auch bei abweichendem Zufallspool importiert werden, wie der vorige Nutzerfall zeigt. Soll der importierte Code zusätzlich den Zufallspool für offene Plätze festlegen, ist das eine eigenständige, funktionale Erweiterung des Importverhaltens; für bloß vollständig vorgegebene Kartenlisten ist diese Poolanpassung nicht erforderlich. Platin/Kolonien, Unterschlüpfe und Landschaftsquoten wurden durch diesen gezielten Test nicht separat abgenommen.


### Gemeldeter Unterschied bei Platin/Kolonien mit und ohne `-e`

**18. November 2026:** Der Nutzer präzisierte, dass für ihn die vollständige Übernahme eines konkret vom Generator vorgegebenen Königreichs zählt, nicht die Aktivierung von Erweiterungen für freie Zufallsplätze. Er meldete zum selben konkreten Königreich mit `colonies` und `-m Landscapes(4,4), NoShelters`: Mit zusätzlichem `-e base, prosperity, menagerie, promos` waren Platin/Kolonien vorhanden, ohne diesen Abschnitt nicht. Hauptstapel waren Amboss, Bibliothek, Gewölbe, Großer Markt, Herberge, Kavallerie, Keller, Laboratorium, Vorbotin und Heiligtum; Landschaften Weg der Ratte, Einladung und Verzögerung. Einladung ist laut vorigem Nutzerbefund nicht manuell auswählbar.

Die beiden zuletzt gelesenen Partiedateien enthalten denselben gespeicherten `setupData`-Text: nur `-e base`, diese zehn konkreten Hauptstapel sowie Weg der Ratte und Verzögerung. In keinem der beiden Texte steht ein explizites `colonies` oder ein `-m`-Abschnitt. Ein aufgelöstes `kingdomData` enthält `colonies`, das andere nicht. Da beide Königreiche Blütezeitkarten enthalten, ist dieser einzelne Ergebnisvergleich noch kein Nachweis dafür, dass der zusätzliche `-e`-Abschnitt den Kolonien-Schalter erzwingt; ein regulär variierender Spielaufbau bleibt eine mögliche Erklärung. Eine kontrollierte Prüfung mit zehn Basisspiel-Hauptstapeln und vorher nicht eingeschalteter Platin-/Kolonienoption würde diese Ursache ausschließen. Die bloße Existenz oder Nichtübernahme von Erweiterungs-Zufallsquoten beantwortet die Frage der vollständigen konkreten Set-Übernahme nicht.


### Kontrolltest mit ausschließlich Basisspiel-Hauptstapeln und Platin/Kolonien

**18. November 2026, Nutzerbericht:** Nach geleerter Kartenauswahl und Platin/Kolonien auf „Nach Spielregel“ wurde dieser gezielte Test vorgeschlagen:

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop, colonies -e base, prosperity -m Landscapes(0,0), NoShelters
```

Der Nutzer bestätigte anschließend, dass in diesem Fall kein Platin/Kolonien vorhanden ist, und beauftragte ausdrücklich die Fortsetzung der Importkorrektur im Patch-Chat. Die Hauptstapel enthalten keine Blütezeitkarte; damit erklärt ein regulärer Blütezeit-Zufallsentscheid kein vermeintlich positives Ergebnis. Das negative Ergebnis zeigt für diesen Nutzertest, dass die explizite Option auch mit `-e` nicht zuverlässig übernommen wird. Geladene Module wurden während dieser konkreten Partie nicht unabhängig erfasst.

Ziel ist die zuverlässige Übernahme des vollständig vorgegebenen Generator-Sets samt expliziten Aufbauoptionen. Die Erweiterungs-Zufallspoolwahl für offene Plätze ist davon getrennt und nachrangig. Ein gemeinsamer Export kann `-e` syntaktisch enthalten; es ersetzt jedoch weder `colonies`/`shelters` noch die expliziten Aus-Schalter oder Landschaftsquoten. Die automatische Wirkung dieser Angaben im UI-Import bedarf der Patchkorrektur beziehungsweise ihrer eigenen Abnahme. Der Generator gibt bislang noch kein `-e` automatisch aus.


### Generator-Ausgabe mit `-e` ab 18. November 2026

`buildKingdomCode()` ergänzt den gemeinsamen Kopiercode um einen einzelnen `-e`-Abschnitt zwischen `-k` und `-m`. Die alphabetisch sortierte, duplikatfreie Liste stammt ausschließlich aus den tatsächlich ausgegebenen Hauptstapeln, Landschaften, Verbündeten/Prophezeiungen und rekursiv codierten Zusatzkarten-Zuordnungen. Bei eingeschaltetem Platin/Kolonien kommt `prosperity` hinzu, bei Unterschlüpfen `dark_ages`. Die interne Gruppe `basic` wird nicht als Erweiterung ausgegeben. Nicht ausgeloste beziehungsweise nur zugelassene Erweiterungen, automatisch folgendes Material, ausgelassene Sonderzuordnungen und Planungsstapel jenseits der ersten zehn Hauptstapel erweitern die Liste nicht.

Die vorhandenen `colonies`-/`shelters`-Schalter, expliziten Aus-Schalter und Landschaftsquoten bleiben erhalten. Es wird keine vollständige Übernahme dieser Optionen in Vanilla versprochen: Der kurze Hinweis am Kopierfeld steht in Deutsch und Englisch. Die Namensumwandlung des separaten Randomizer-Eingabebereichs bleibt unverändert. Der Export schaltet keine Besitzdaten frei; die offizielle Freigabeprüfung bleibt Aufgabe des Spiels beziehungsweise des gesondert bearbeiteten Import-Patches.

**Technisch geprüft:** Der echte HTML-Exporter wird über `node SetGenerator/test_kingdom_export.js` ausgeführt. Abgedeckt sind Basisspiel und mehrere Erweiterungen, reine Platin-/Kolonien- beziehungsweise Unterschlupfoptionen, Landschaften einschließlich Promos, erste Editionen, gemischte Stapel, verschachtelte Zuordnungen, Verbündete/Prophezeiungen und ausgelassene Auswahlteile. Der Pages-Workflow führt diesen Test zusätzlich aus und veröffentlicht weiterhin die HTML-Datei unverändert. Die neue automatisch erzeugte Ausgabe bleibt bis zu einem eigenen Starttest mit dem neuen Patch ein technisch geprüfter Export, kein bestätigter neuer Spielaufbau.
