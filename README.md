# Dominion

**[Dominion-Generator direkt im Browser öffnen](https://serpens66.github.io/Dominion/)**

Dieses Projekt entwickelt unabhängige Werkzeuge für das digitale Kartenspiel Dominion von Temple Gates Games. Der Schwerpunkt liegt auf dem Erstellen, Importieren und Teilen eigener Königreiche. Die Dokumentation sammelt außerdem Erkenntnisse zur Steam-Version und zu ihren Karten- und Setup-Daten. [Offizielle Dominion-Seite](https://www.templegatesgames.com/press/sheet.php?p=Dominion)

## Dominion-Konverter

1. Stelle auf dem Dominion Randomizer ein Königreich zusammen und kopiere die Kartennamen mit dem Copy-Button. [Dominion Randomizer](https://dominionrandomizer.com/)
2. Lade `Dominion-Konverter.html` aus dem Release herunter, öffne die Datei im Browser und füge die Namensliste ein. Der Konverter läuft lokal und erkennt alle sieben Sprachen des Randomizers. [Aktueller Release](https://github.com/Serpens66/Dominion/releases/latest)
3. Kopiere die fertige `-k`-Zeile und füge sie in der Königreich-Auswahl von Dominion in das Kartensuchfeld ein.

Der Konverter verwendet den Kartenkatalog aus Dominion-Spielbuild 3315. Die Anleitung beschreibt auch die Python-Variante und die Grenzen bei besonderem Karten-Setup. [SetGenerator](SetGenerator/README.md)

Der Dominion-Generator zieht Königreiche aus Basisspiel und Erweiterungen. Er unterstützt feste Karten, Ausschlüsse, Ziehregeln und den Export für doppelte Stapel. [Anleitung](SetGenerator/README.md#html-generator-für-basisspiel-und-erweiterungen)

## Dokumentation

- [Einstieg und Quellen](Documentation/README.md)
- [Spielanleitung für die Steam-Version](Documentation/Steam_Spielanleitung.md)
- [Kartenkatalog mit Erweiterungen, Editionen und Filtern](Documentation/Card_Properties.json)
- [Kartentitel und IDs des Web-Randomizers](Documentation/KingdomCreator_Card_Names.json)

Die lokalisierten Namen des Web-Randomizers stammen aus KingdomCreator. [Quellstand](https://github.com/blakevanlan/KingdomCreator/tree/d9a802db2e41d52786eaa02097279cbf1b64c778), [Lizenzhinweis](THIRD_PARTY_NOTICES.md)
