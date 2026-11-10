# SetGenerator

Der HTML-Generator zieht standardmäßig zehn Königreichstapel für Dominion von Temple Gates Games. Der separate Konverter übernimmt Namenslisten eines bestehenden Königreichs. Speicherformat und Prüfnachweise stehen in [Documentation/Kingdom_Sets.md](../Documentation/Kingdom_Sets.md).

## Web-Namenslisten umwandeln

Auf dominionrandomizer.com die gewünschte Auswahl mit dem Copy-Button kopieren. Anschließend im Projektordner in PowerShell ausführen.

```powershell
& 'D:\CDesktopLink\Portable\Python\WinPy64\python\python.exe' .\SetGenerator\convert_kingdomcreator.py (Get-Clipboard -Raw)
```

Die ausgegebene `-k`-Zeile ins Kartensuchfeld der Kingdom-Auswahl einfügen. Der Konverter erkennt die sieben Website-Sprachen automatisch. Mit `--lang de` oder `--lang en` lässt sich die Sprache festlegen. Karten der ersten Edition werden separat gemeldet. `--colonies` und `--shelters` ergänzen die entsprechenden Setup-Optionen, wenn sie auf der Website ausgewählt waren.

Der Konverter verarbeitet zehn Hauptstapel und bis zu vier gewöhnliche Landschaften ohne zusätzliche Zuordnung. Mehrdeutige Titel, Duplikate und Zusatzsetup wie der Bane-Stapel führen zu einer Fehlermeldung. Der Namensabgleich und die Ausgabe wurden lokal geprüft. Die Landschaftsausgabe verwendet die im Profil beobachtete Syntax und steht als Spieltest noch aus. [Kartennamen und Herkunft](../Documentation/Kingdom_Sets.md#web-namenslisten-im-konverter)

## HTML-Konverter mit doppelten Stapeln

[Dominion-Konverter.html](Dominion-Konverter.html) lokal im Browser öffnen. Einen bis zehn Königreichstapel durch Kommas getrennt eingeben oder die kopierte Namensliste einfügen. Landschaften stehen am Ende der Liste. Nach Klick auf **Kartenliste bestätigen** gibt die Seite den normalen Kingdom-Code fürs Kartensuchfeld und separat eine reine `-x`-Zeile aus. Dominion füllt freie Königreichplätze zufällig auf. Der Verdopplungscode enthält je einen `start_double_pile`-Effekt für die festgelegten Stapel.

Nach Bestätigung der Kartenliste erscheint der fertige `<sets>`-Eintrag. Der Name „Doppelte Stapel“ ist vorausgefüllt und lässt sich ändern. In einem Browser mit lokalem Dateizugriff kann der Konverter den Eintrag auch direkt in `%APPDATA%\TempleGates\Dominion\profiles.xml` einfügen. Dafür Dominion vollständig schließen, die Datei sichern, auswählen und die Schreibfreigabe erteilen. Der direkte Export unterstützt Dateien mit genau einem Profil. Bei einem gleichnamigen Set oder 64 vorhandenen Sets bricht er ab.

Das normale Königreich zuerst über das Kartensuchfeld in Dominion importieren und als Set speichern. Danach das Verdopplungsset laden und „Spielen“ wählen. Die Partie aufgeben, das Set in der Kingdom-Auswahl entladen und die Zusatzregeln stehen lassen. Dann das gespeicherte Königreich laden und die Partie starten. Die Übernahme in ein weiteres gespeichertes Set wurde in Build 3315 bestätigt. Für den HTML-Export steht der Spieltest noch aus. [Spielbefund](../Documentation/Modding_Overview.md#effektsets-und-zufallsauswahl), [Speicherformat](../Documentation/Kingdom_Sets.md#nativer-profilpfad)

## HTML-Generator für Basisspiel und Erweiterungen

[Dominion-Generator.html](Dominion-Generator.html) lokal im Browser öffnen und auf **Königreich ziehen** klicken. Der Kartenkatalog aus Spielbuild 3315 umfasst 498 Königreichstapel. Anfangs zieht der Generator aus der zweiten Edition des Basisspiels. Die Zeile **Erweiterungen und Editionen** über den Ziehregeln zeigt die aktiven Erweiterungen. Aufgeklappt lässt sich jede Herkunft mit einer Von-bis-Quote freigeben. 0 bis 0 sperrt sie und 2 bis 2 verlangt genau zwei Stapel. Die gewählte Stapelzahl als Bis-Wert lässt die Obergrenze offen. **1. Ed.** nimmt ältere Karten in denselben Pool auf. Beide Editionen teilen sich die Quote. Der Erweiterungsname öffnet die Originalbeschreibung und die verfügbaren Regelabschnitte aus der Steam-Version. Unter **Karten festhalten oder ausschließen** lassen sich die Gruppen einzeln öffnen oder nach Namen und Eigenschaften auf Deutsch und Englisch durchsuchen. Über **Eigenschaften auswählen** direkt neben der Kartensuche lassen sich Kriterien per Checkbox kombinieren. Karten der ersten Edition tragen die Markierung **1. Ed.**. Die Spielsymbole sind direkt in der HTML-Datei eingebettet. Einen gezogenen Stapel kannst du direkt im Ergebnis für die nächste Ziehung festhalten. Änderungen am Kartenpool und an den Quoten lassen die letzte Ziehung samt Export stehen.

Mit **Immer**, **Zufall** und **Aus** legst du fest, aus welchen Sets der Generator ziehen darf. Stelle zum Beispiel Basisspiel, Intrige und Blütezeit auf **Immer**, wähle unter **Zusätzlich aus dem Lostopf** die Zahl **2** und klicke **Übrige für Zufall freigeben**. Das Ergebnis nennt die beiden ausgelosten Sets. Auch ein Set im Kartenpool kann bei einer Mindestquote von null ohne Stapel bleiben. Festgehaltene Karten und positive Mindestquoten aus Zufallssets belegen jeweils einen Platz für ihr Set. Landschaften mit **Auto** übernehmen den ausgelosten Pool. **Erweiterungen im Ergebnis (ohne Basisspiel)** zählt dagegen nur Sets mit mindestens einem tatsächlich gezogenen Hauptstapel. Beim Standardwert von 0 bis zur gewählten Stapelzahl bleibt diese Anzahl offen.

Der Generator speichert Stapelzahl, Filter, Erweiterungsauswahl, Erweiterungsquoten, Editionen, Landschaften, festgehaltene und ausgeschlossene Karten, die Vorschau-Sprache und den Setnamen im Browser. Unter **Einstellungen sichern oder laden** kannst du eine JSON-Datei mit diesen Angaben und dem gewählten Design exportieren und wieder importieren. Die Datei funktioniert auch in einem anderen Browser oder wenn der automatische Speicher für die lokal geöffnete HTML-Datei gesperrt ist. Über **Einstellungen zurücksetzen** stellst du die Standardwerte wieder her. Die Buttons in den einzelnen Bereichen setzen gezielt Spielaufbau, Erweiterungen, Ziehfilter, Landschaftsquoten, ausgeschlossene oder festgehaltene Karten, die Kartenvorschau oder den Setnamen zurück. Über **Suche zurücksetzen** leerst du Suchtext und ausgewählte Sucheigenschaften.

Für +2 Aktionen, Kartenziehen, Extrakauf, Entsorgung und Angriff lassen sich **Von** und **Bis** getrennt einstellen. Die Grenzen beziehen sich auf die Anzahl der passenden Stapel. Mit `1 bis 3` enthält das Königreich ein bis drei davon. `0 bis 0` schließt passende Stapel aus. Die gewählte Stapelzahl als Bis-Wert lässt die Obergrenze offen. +2 Aktionen, Ziehen mehrerer Karten und Entsorgung verwenden die Spielfilter aus dem Kartenkatalog. Der Extrakauf prüft den sofortigen Kaufbonus.

Der Bereich **Abwehrkarten bei Angriffen** greift, sobald mindestens ein Angriffsstapel im Königreich liegt. `1 bis 1` verlangt dann genau einen Abwehrstapel. Ohne Angriff bleibt die Auswahl der Abwehrkarten frei. Im Basisspiel zählt Burggraben. Im ausgelesenen Spielkatalog steht der Typ **Reaktion**. Der Generator kennzeichnet den Angriffsschutz anhand des geprüften Karteneffekts mit `defendsAgainstAttacks`.

**Angriffsreaktionen** und **Sonstige Reaktionskarten** haben jeweils einen eigenen Von-bis-Bereich. Die Quote für Angriffsreaktionen gilt nur, wenn das gezogene Königreich mindestens eine Angriffskarte enthält. Die Quote für sonstige Reaktionskarten gilt bei jeder Ziehung. Angriffsreaktionen werden durch das Ausspielen einer gegnerischen Angriffskarte ausgelöst und lassen den Angriff weiterlaufen. Sonstige Reaktionskarten haben den Spieltyp **Reaktion** und gehören zu keiner der beiden anderen Kategorien. Burggraben zählt nur als Abwehr. Mit den Erweiterungen können weitere passende Stapel in den Pool kommen.

Die Prüfung aller im Steam-Kartenkatalog erfassten Erweiterungen und der Vergleich mit KingdomCreator stehen in der [Übersicht zum Angriffsschutz](../Documentation/Abwehrkarten.md).

Ein Stapel kann für mehrere Kategorien zählen. Der Generator prüft beim Ändern der Eingaben, wie viele Königreiche unter allen Regeln, Ausschlüssen und festgehaltenen Karten möglich sind. Die Ziehprüfung läuft im Hintergrund und endet spätestens nach fünf Sekunden. Bei abgeschlossener exakter Zählung sind die Hauptstapelkombinationen gleichverteilt. Meldet der Generator nur eine gültige Ziehung, ist die Gleichverteilung offen. Neue Eingaben ersetzen die laufende Prüfung. Bei zusätzlichen Zufallssets prüft der Generator, ob ein ausgeloster Pool eine gültige Ziehung zulässt. Die Bereitschaftsmeldung bezieht sich auf diese Prüfung.

Münzkosten von 1 bis 8 haben eigene Von-bis-Filter. **Genau 5 Münzen** zählt Stapel mit reinen Münzkosten von 5. Tränke und Schulden haben eigene Filter. Unter **Ressourcen und Marker** kannst du Von-bis-Grenzen für Stapel setzen, die Taler, Dorfbewohner, Gefallen, Sonnenmarker, Punktemarker, Pferde oder Kostbarkeiten gewähren. Die Filter zählen auch Stapel, deren Bonus erst beim Nehmen oder Entsorgen ausgelöst wird. **Landschaften** begrenzt Gesamtzahl, Typ und Herkunft separat. Zwei leere Herkunftsfelder zeigen **Auto** und übernehmen die Freigabe aus dem Hauptstapel-Pool. Ein eingetragenes 0 bis 0 sperrt diese Herkunft separat. **Spielaufbau** steht über den Ziehregeln und enthält die Optionen für Platin und Kolonien, Unterschlüpfe und Kostbarkeiten. **Kostbarkeiten im Spiel verlangen** setzt mindestens einen gezogenen Hauptstapel oder eine Landschaft voraus, die Kostbarkeiten ins Spiel bringen. Beim ersten Start und nach einem Reset verlangen +2 Aktionen, mehrere Karten ziehen und sofort +1 Kauf je mindestens einen Stapel. Entsorgung hat kein Minimum. Für genau sieben und genau acht Münzen gilt je höchstens ein Stapel. Für Abwehrkarten, Angriffsreaktionen und sonstige Reaktionskarten gilt je höchstens zwei. Für Kartentypen, Trank- und Schuldenkosten sowie Ressourcen und Marker gilt je 0 bis 3. Gespeicherte Werte haben beim Laden Vorrang.

Unter **Spielaufbau** erscheinen die Landschaften, benötigte Sonderkarten und Zuordnungen. Ist der Steam-Import eines Teils offen, kennzeichnet der Generator den Kopiertext als Hauptstapel-Teilcode. Die offenen Fälle stehen in der [Testliste](../Documentation/Generator_Erweiterungen_Tests.md).

**Anzahl der Königreichstapel** steht unter Spielaufbau. 11 bis 13 Stapel dienen der experimentellen Planung. Der [Import zusätzlicher frei gewählter Stapel in Steam](../Documentation/Steam_Spielanleitung.md#zwölf-oder-mehr-königreichstapel) ist offen. Der Steam-Teilcode und die Verdopplung enthalten dieselben ersten zehn Stapel. Die übrigen gezogenen Stapel stehen separat im Ergebnis. Landschaften und regelbedingte Zusatzstapel zählen separat. Der XML-Eintrag für die Effekte funktioniert wie beim Konverter, einschließlich optionaler Schreibfreigabe und `expansionsBitset = 0`. Der Spieltest mit dem neuen Generator steht noch aus. [Ablauf in Dominion](#html-konverter-mit-doppelten-stapeln)

### Kartenvorschau und optionales Bildpaket

Fahre mit der Maus über einen Kartennamen im Kartenpool, in den Ziehregeln oder im Ergebnis. Du kannst ihn auch per Tab fokussieren. Per Klick oder Tippen lässt sich die Vorschau offen halten und schließen. Über **Language** neben **Design** kannst du die Oberfläche zwischen Deutsch und Englisch umschalten. Der Wechsel stellt auch die Kartenvorschau auf die gewählte Sprache. Mit **Kartenvorschau** lässt sich die Kartensprache danach separat ändern. Der Filter zum Ziehen mehrerer Karten zählt Burggraben, Hexe, Laboratorium, Ratsversammlung, Schmiede und Bibliothek.

Die kleinen Münz-, Punkte- und Filtersymbole aus Dominion sind in der HTML-Datei eingebettet und stehen offline zur Verfügung. Feste Boni erscheinen in der Spielschreibweise, etwa „+2 Karten, +1 Aktion“ beim Laboratorium. Das optionale Bildpaket enthält fertig gerenderte Vorderseiten des Basisspiels auf Deutsch und Englisch. Für die übrigen Karten setzt der Generator die Vorderseite aus dem Spielrahmen, den Kosten, der Illustration und dem lokalisierten Regeltext zusammen. Karten ohne Illustration verwenden denselben Aufbau. Der Generator zeigt auf Wunsch unter **Effekt als Text** auch die lesbare Kurzfassung mit den Vorschauen für erwähnte Karten.

Für die Kartenbilder das [Extraktionswerkzeug](extract_card_images.py) herunterladen und mit Python 3.9 oder neuer und Pillow ab Version 10.4 ausführen. Gib den Pfad zu deiner Steam-Datei `Dominion.xpack` und den gewünschten Pfad zum neuen Bildpaket an.

```powershell
python -m pip install Pillow
python .\extract_card_images.py "E:\ProgrammeE\Steam\steamapps\common\Dominion\Dominion.xpack" "Dominion-Kartenbilder.json"
```

Ergänze für die verfügbaren Illustrationen der Erweiterungen den Parameter **--all-expansions**. Das Werkzeug meldet fehlende oder nicht lesbare Ressourcen je Karte.

Wähle das erzeugte JSON im Generator unter **Kartenbilder laden** aus. Ältere Bildpakete liefern ihre vorhandenen Illustrationen an dieselbe Kartenvorschau. Der Generator lädt die Bilder beim Öffnen der jeweiligen Vorschau. Bei einem nicht lesbaren Bild bleibt der Kartentext sichtbar. Das Werkzeug meldet Karten, für die sich keine Illustration auflösen lässt. Der Exporter wurde mit der Steam-Installation von Spielbuild 3315 geprüft. [Ressourcenbefund](../Documentation/Sources/Source_Register.md#illustrationen-aus-l05)

## Karten nach Eigenschaften filtern

[Card_Properties.json](../Documentation/Card_Properties.json) enthält unter `cards` die Eigenschaften je interner ID. Für einen sofortigen Extrakauf den erlaubten Kartenpool auf Einträge mit `kingdomPile == true` und `immediateBuys > 0` einschränken. In der zweiten Ausgabe des Basisspiels bleiben `council_room`, `festival` und `market`. Für die Auslegung des Spiels stattdessen nach `ExtraBuys` in `nativeFilters` filtern. Das kann auch Käufe erfassen, die eine Wahl oder Bedingung voraussetzen. [Details zur Herkunft und Abdeckung](../Documentation/Sources/Source_Register.md#karteneigenschaften-aus-l01-und-l05)

Weitere Quoten, Kartensperren und Setup-Abhängigkeiten sind im [Befund zum Web-Randomizer](../Documentation/Kingdom_Sets.md#optionale-regeln-für-die-ziehung) dokumentiert. Der [KingdomCreator-Issue-Abgleich](../Documentation/KingdomCreator.md) zeigt, welche Wünsche der eigene Generator bereits erfüllt und welche Erweiterungen noch sinnvoll sind.

## Erste Abnahme

Der Spieltest prüft, ob die zehn ausgegebenen Karten vollständig und ohne Abweichungen übernommen werden und sich eine lokale Partie damit starten lässt. Für den optionalen Profilexport zusätzlich nach einem Neustart prüfen, ob der generierte Eintrag und die vorherigen Sets auswählbar bleiben. Landschaften und weitere Setup-Optionen werden bei Bedarf anhand eigener Importbeispiele ergänzt.

## Eingebetteten Steam-Katalog aktualisieren

Aus dem Projektordner den Generator-Build mit der geprüften Dominion.xpack ausführen. Er bettet den Kartenkatalog mit Lokalisierung und den Quelltext des Zieh-Workers in die eigenständige HTML-Datei ein.

```powershell
& 'D:\CDesktopLink\Portable\Python\WinPy64\python\python.exe' .\SetGenerator\generator_catalog.py 'E:\ProgrammeE\Steam\steamapps\common\Dominion\Dominion.xpack'
```


## Eigenständige HTML-Datei mit eingebetteten Bildern erzeugen

Ein mit `extract_card_images.py` erzeugtes Bildpaket verwenden. Mit `--all-expansions` enthält es die verfügbaren Erweiterungsbilder. Als Ausgabe eine eigene Datei wählen. Der Build verwendet den eingebetteten Katalog der lokalen HTML-Datei und den aktuellen Zieh-Worker.

```powershell
python .\SetGenerator\generator_catalog.py --image-pack .\Dominion-Kartenbilder.json --output .\Dominion-Generator-mit-Bildern.html
node .\SetGenerator\test_generator_solver.js .\Dominion-Generator-mit-Bildern.html
```

Der Test vergleicht den Zieh-Worker mit seinem Quelltext und beide HTML-Dateien miteinander. Die Varianten unterscheiden sich nur durch das Bildpaket und gegebenenfalls den Pfad zum Extraktionswerkzeug.

## Generator über GitHub Pages anbieten

GitHub Pages kann den Generator als statische Seite unter `https://serpens66.github.io/Dominion/` bereitstellen. Besucher öffnen die Seite direkt im Browser. Ziehung und Dateiverarbeitung bleiben im Browser. Auf der Website und beim lokalen Öffnen gelten getrennte Browserspeicher. Vorhandene Einstellungen können per JSON übertragen werden. [GitHub Pages im Überblick](https://docs.github.com/en/pages/getting-started-with-github-pages/about-github-pages)

1. Eine öffentlich verwendbare Ausgabe erstellen. Auch die Version ohne eingebettete Kartenbilder enthält Symbole, Texturen und Originaltexte aus dem Steam-Spiel. Vor einer Veröffentlichung die Rechte für diese Inhalte klären oder sie durch freigegebene Alternativen ersetzen.
2. Die freigegebene Ausgabe als `index.html` in einem eigenen Branch `gh-pages` ablegen. Nur Dateien ergänzen, auf die diese Ausgabe verlinkt und die öffentlich bereitgestellt werden dürfen. Für die statische Ausgabe eine leere `.nojekyll` ergänzen.
3. Im GitHub-Repository unter **Settings → Pages → Source** die Option **Deploy from a branch** wählen. Als Quelle **gh-pages** und **/(root)** auswählen und speichern. Anschließend die von GitHub angezeigte Adresse und HTTPS prüfen. [Veröffentlichungsquelle konfigurieren](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site)
4. Die veröffentlichte Seite in Firefox und einem Chromium-Browser öffnen. Ziehung, Kopieren sowie Import und Export von Einstellungen prüfen. Für den Profilexport steht je nach Browser der XML-Kopierweg zur Verfügung. Für direkte Schreibtests ein separates Testprofil verwenden.

GitHub Free unterstützt Pages für öffentliche Repositorys. Pages kann auch bei einem privaten Repository öffentlich erreichbar sein. [Voraussetzungen für GitHub Pages](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site#who-can-use-this-feature)
