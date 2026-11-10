# SetGenerator

Der HTML-Generator zieht zehn Königreichstapel für Dominion von Temple Gates Games. Der separate Konverter übernimmt Namenslisten eines bestehenden Königreichs. Speicherformat und Prüfnachweise stehen in [Documentation/Kingdom_Sets.md](../Documentation/Kingdom_Sets.md).

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

[Dominion-Generator.html](Dominion-Generator.html) lokal im Browser öffnen und auf **Königreich ziehen** klicken. Der Kartenkatalog aus Spielbuild 3315 umfasst 498 Königreichstapel. Anfangs zieht der Generator aus der zweiten Edition des Basisspiels. Unter **Erweiterungen und Editionen** lässt sich jede Herkunft mit einer Von-bis-Quote freigeben. 0 bis 0 sperrt sie, 0 bis 10 mischt sie in den Pool und 2 bis 2 verlangt genau zwei Stapel. Unter **Karten festhalten oder ausschließen** lassen sich die Gruppen einzeln öffnen oder nach deutschen und englischen Namen durchsuchen. Einen gezogenen Stapel kannst du direkt im Ergebnis für die nächste Ziehung festhalten.

Der Generator speichert Filter, Erweiterungsquoten, Editionen, Landschaften, festgehaltene und ausgeschlossene Karten, die Vorschau-Sprache und den Setnamen im Browser. Unter **Einstellungen sichern oder laden** kannst du eine JSON-Datei mit diesen Angaben und dem gewählten Design exportieren und wieder importieren. Die Datei funktioniert auch in einem anderen Browser oder wenn der automatische Speicher für die lokal geöffnete HTML-Datei gesperrt ist. Über **Einstellungen zurücksetzen** stellst du die Standardwerte wieder her.

Für +2 Aktionen, Kartenziehen, Extrakauf, Entsorgung und Angriff lassen sich **Von** und **Bis** getrennt einstellen. Die Grenzen beziehen sich auf die Anzahl der passenden Stapel. Mit `1 bis 3` enthält das Königreich ein bis drei davon. `0 bis 10` lässt eine Kategorie offen, `0 bis 0` schließt sie aus. +2 Aktionen, Ziehen mehrerer Karten und Entsorgung verwenden die Spielfilter aus dem Kartenkatalog. Der Extrakauf prüft den sofortigen Kaufbonus.

Der Bereich **Abwehrkarten bei Angriffen** greift, sobald mindestens ein Angriffsstapel im Königreich liegt. `1 bis 1` verlangt dann genau einen Abwehrstapel. Ohne Angriff bleibt die Auswahl der Abwehrkarten frei. Im Basisspiel zählt Burggraben. Im ausgelesenen Spielkatalog steht der Typ **Reaktion**. Der Generator kennzeichnet den Angriffsschutz anhand des geprüften Karteneffekts mit `defendsAgainstAttacks`.

**Angriffsreaktionen** und **Sonstige Reaktionskarten** haben jeweils einen eigenen Von-bis-Bereich. Die Bereiche gelten bei jeder Ziehung. Angriffsreaktionen werden durch das Ausspielen einer gegnerischen Angriffskarte ausgelöst und lassen den Angriff weiterlaufen. Sonstige Reaktionskarten haben den Spieltyp **Reaktion** und gehören zu keiner der beiden anderen Kategorien. Burggraben zählt nur als Abwehr. Mit den Erweiterungen können weitere passende Stapel in den Pool kommen.

Die Prüfung aller im Steam-Kartenkatalog erfassten Erweiterungen und der Vergleich mit KingdomCreator stehen in der [Übersicht zum Angriffsschutz](../Documentation/Abwehrkarten.md).

Ein Stapel kann für mehrere Kategorien zählen. Der Generator prüft beim Ändern der Eingaben, wie viele Königreiche unter allen Regeln, Ausschlüssen und festgehaltenen Karten möglich sind. Die Ziehprüfung läuft im Hintergrund und endet spätestens nach fünf Sekunden. Bei abgeschlossener exakter Zählung sind die Hauptstapelkombinationen gleichverteilt. Meldet der Generator nur eine gültige Ziehung, ist die Gleichverteilung offen. Neue Eingaben ersetzen die laufende Prüfung.

Münzkosten von 1 bis 8 haben eigene Von-bis-Filter. **Kosten: Genau 5 Münzen** zählt Stapel mit reinen Münzkosten von 5. Tränke und Schulden haben eigene Filter. Unter **Ressourcen und Marker** kannst du Von-bis-Grenzen für Stapel setzen, die Taler, Dorfbewohner, Gefallen, Sonnenmarker, Punktemarker, Pferde oder Kostbarkeiten gewähren. Die Filter zählen auch Stapel, deren Bonus erst beim Nehmen oder Entsorgen ausgelöst wird. **Landschaften** begrenzt Gesamtzahl, Typ und Herkunft separat. **Spielaufbau** steht über den Ziehregeln und enthält die Optionen für Platin und Kolonien, Unterschlüpfe und Kostbarkeiten. **Kostbarkeiten im Spiel verlangen** setzt mindestens einen gezogenen Hauptstapel oder eine Landschaft voraus, die Kostbarkeiten ins Spiel bringen. Beim ersten Start und nach einem Reset gelten die [Standardquoten für Kartenziehen, Angriffe, Kosten und Ereignisse](../Documentation/KingdomCreator.md#abnahmekriterien-für-die-spätere-umsetzung). Gespeicherte Werte haben beim Laden Vorrang.

Unter **Spielaufbau** erscheinen die Landschaften, benötigte Sonderkarten und Zuordnungen. Ist der Steam-Import eines Teils offen, kennzeichnet der Generator den Kopiertext als Hauptstapel-Teilcode. Die offenen Fälle stehen in der [Testliste](../Documentation/Generator_Erweiterungen_Tests.md).

Der Generator erzeugt den `-k`-Code und separat zehn `start_double_pile`-Effekte für dieselbe Ziehung. Der XML-Eintrag für die Effekte funktioniert wie beim Konverter, einschließlich optionaler Schreibfreigabe und `expansionsBitset = 0`. Der Spieltest mit dem neuen Generator steht noch aus. [Ablauf in Dominion](#html-konverter-mit-doppelten-stapeln)

### Kartenvorschau und optionales Bildpaket

Fahre mit der Maus über einen Kartennamen im Kartenpool, in den Ziehregeln oder im Ergebnis. Du kannst ihn auch per Tab fokussieren. Per Klick oder Tippen lässt sich die Vorschau offen halten und schließen. **Kartenvorschau** wechselt zwischen deutschen und englischen Titeln und Regeltexten. Der Filter zum Ziehen mehrerer Karten zählt Burggraben, Hexe, Laboratorium, Ratsversammlung, Schmiede und Bibliothek.

Die kleinen Münz-, Punkte- und Filtersymbole aus Dominion sind in der HTML-Datei eingebettet und stehen offline zur Verfügung. Feste Boni erscheinen in der Spielschreibweise, etwa „+2 Karten, +1 Aktion“ beim Laboratorium. Das optionale Bildpaket enthält vollständige Kartenvorderseiten des Basisspiels auf Deutsch und Englisch. Mit der Zusatzoption für Erweiterungen kommen auflösbare Illustrationen hinzu. Bei fehlenden Bildern erscheint die lokalisierte Textvorschau. Der Generator zeigt auf Wunsch unter **Effekt als Text** auch die lesbare Kurzfassung mit den Vorschauen für erwähnte Karten.

Für die Kartenbilder das [Extraktionswerkzeug](extract_card_images.py) herunterladen und mit Python 3.9 oder neuer und Pillow ab Version 10.4 ausführen. Gib den Pfad zu deiner Steam-Datei `Dominion.xpack` und den gewünschten Pfad zum neuen Bildpaket an.

```powershell
python -m pip install Pillow
python .\extract_card_images.py "E:\ProgrammeE\Steam\steamapps\common\Dominion\Dominion.xpack" "Dominion-Kartenbilder.json"
```

Ergänze für die verfügbaren Illustrationen der Erweiterungen den Parameter **--all-expansions**. Das Werkzeug meldet fehlende oder nicht lesbare Ressourcen je Karte.

Wähle das erzeugte JSON im Generator unter **Kartenbilder laden** aus. Das Werkzeug setzt die Karten lokal aus Spielillustration, Rahmenteilen, Münze, Titel, Typ und lokalisiertem Spieltext zusammen. Ältere Pakete zeigen weiterhin die Illustration mit der Textvorschau. Der Exporter wurde mit der Steam-Installation von Spielbuild 3315 geprüft. [Ressourcenbefund](../Documentation/Sources/Source_Register.md#illustrationen-aus-l05)

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
