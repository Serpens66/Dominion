# SetGenerator

Der geplante Generator erstellt aus einer konfigurierbaren Kartenauswahl zehn verschiedene zulässige Königreichkarten und macht sie in Dominion von Temple Gates Games spielbar. Die regelbasierte Ziehung ist bisher ein Entwurf. Speicherformat, Prüfnachweise und der nächste Spieltest stehen in [Documentation/Kingdom_Sets.md](../Documentation/Kingdom_Sets.md).

## Web-Namenslisten umwandeln

Auf dominionrandomizer.com die gewünschte Auswahl mit dem Copy-Button kopieren. Anschließend im Projektordner in PowerShell ausführen.

```powershell
& 'D:\CDesktopLink\Portable\Python\WinPy64\python\python.exe' .\SetGenerator\convert_kingdomcreator.py (Get-Clipboard -Raw)
```

Die ausgegebene `-k`-Zeile ins Kartensuchfeld der Kingdom-Auswahl einfügen. Der Konverter erkennt die sieben Website-Sprachen automatisch. Mit `--lang de` oder `--lang en` lässt sich die Sprache festlegen. Karten der ersten Edition werden separat gemeldet. `--colonies` und `--shelters` ergänzen die entsprechenden Setup-Optionen, wenn sie auf der Website ausgewählt waren.

Der Konverter verarbeitet zehn Hauptstapel und bis zu vier gewöhnliche Landschaften ohne zusätzliche Zuordnung. Mehrdeutige Titel, Duplikate und Zusatzsetup wie der Bane-Stapel führen zu einer Fehlermeldung. Der Namensabgleich und die Ausgabe wurden lokal geprüft. Die Landschaftsausgabe verwendet die im Profil beobachtete Syntax und steht als Spieltest noch aus. [Kartennamen und Herkunft](../Documentation/Kingdom_Sets.md#web-namenslisten-im-konverter)

## HTML-Konverter mit doppelten Stapeln

[Dominion-Konverter.html](Dominion-Konverter.html) lokal im Browser öffnen und die kopierte Namensliste einfügen. Die Seite gibt den normalen Kingdom-Code fürs Kartensuchfeld und separat eine reine `-x`-Zeile aus. Je einer der zehn Hauptstapel erhält einen `start_double_pile`-Effekt. Landschaften und zusätzliche Setup-Optionen gehören nur in den Kingdom-Code.

Nach Eingabe eines Namens lässt sich der fertige `<sets>`-Eintrag kopieren. In einem Browser mit lokalem Dateizugriff kann der Konverter ihn auch direkt in `%APPDATA%\TempleGates\Dominion\profiles.xml` einfügen. Dafür Dominion vollständig schließen, die Datei sichern, auswählen und die Schreibfreigabe erteilen. Der direkte Export unterstützt Dateien mit genau einem Profil. Bei einem gleichnamigen Set oder 64 vorhandenen Sets bricht er ab.

Das normale Königreich zuerst über das Kartensuchfeld in Dominion importieren und als Set speichern. Danach das Verdopplungsset laden und „Spielen“ wählen. Die Partie aufgeben, das Set in der Kingdom-Auswahl entladen und die Zusatzregeln stehen lassen. Dann das gespeicherte Königreich laden und die Partie starten. Die Übernahme in ein weiteres gespeichertes Set wurde in Build 3315 bestätigt. Für den HTML-Export steht der Spieltest noch aus. [Spielbefund](../Documentation/Modding_Overview.md#effektsets-und-zufallsauswahl), [Speicherformat](../Documentation/Kingdom_Sets.md#nativer-profilpfad)

## Entwurf

- **Eingabe.** Ein Kartenpool mit Dominion-IDs, die verfügbaren Erweiterungen und die vom Nutzer gewählten Regeln. Kartenkatalog und Spielsprache im Zielbuild prüfen. Welche Eigenschaften und Kombinationen die Regeln berücksichtigen, wird beim gemeinsamen Entwurf festgelegt.
- **Auswahl.** Die Ziehung liefert genau zehn unterschiedliche erlaubte Königreichkarten, die alle gewählten Regeln erfüllen. Sind die Regeln mit dem gewählten Pool nicht erfüllbar, meldet der Generator das ohne einen Export.
- **Erste Ausgabe.** Die Auswahl in den getesteten Kingdom-Text umsetzen und für das Kartensuchfeld in die Zwischenablage kopieren. Die Auswahl funktioniert unabhängig vom jeweiligen Ausgabeweg.
- **Optionale Ausgabe des normalen Königreichs in das Profil.** Für diesen späteren Export die Erweiterungsmaske aus der konkreten Auswahl ableiten und die relevanten Setup-Optionen im Spiel prüfen. Der aktuelle HTML-Entwurf kann den separaten Verdopplungseintrag speichern.

## Karten nach Eigenschaften filtern

[Card_Properties.json](../Documentation/Card_Properties.json) enthält unter `cards` die Eigenschaften je interner ID. Für einen sofortigen Extrakauf den erlaubten Kartenpool auf Einträge mit `kingdomPile == true` und `immediateBuys > 0` einschränken. In der zweiten Ausgabe des Basisspiels bleiben `council_room`, `festival` und `market`. Für die Auslegung des Spiels stattdessen nach `ExtraBuys` in `nativeFilters` filtern. Das kann auch Käufe erfassen, die eine Wahl oder Bedingung voraussetzen. [Details zur Herkunft und Abdeckung](../Documentation/Sources/Source_Register.md#karteneigenschaften-aus-l01-und-l05)

Weitere Quoten, Kartensperren und Setup-Abhängigkeiten sind im [Befund zum Web-Randomizer](../Documentation/Kingdom_Sets.md#optionale-regeln-für-die-ziehung) dokumentiert.

## Erste Abnahme

Der Spieltest prüft, ob die zehn ausgegebenen Karten vollständig und ohne Abweichungen übernommen werden und sich eine lokale Partie damit starten lässt. Für den optionalen Profilexport zusätzlich nach einem Neustart prüfen, ob der generierte Eintrag und die vorherigen Sets auswählbar bleiben. Landschaften und weitere Setup-Optionen werden bei Bedarf anhand eigener Importbeispiele ergänzt.
