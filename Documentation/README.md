# Dominion-Modding-Wissensbasis

Ziel ist die Steam-Version von Dominion von Temple Gates Games. Die erste Untersuchung am 9. November 2026 bezieht sich auf Spielbuild **3315**, Steam-Build **17683350**. Der [Projektleitfaden](../AGENTS.md) enthält die lokalen Pfade und den Arbeitsablauf.

| Aufgabe | Einstieg |
| --- | --- |
| Eigene Kingdoms speichern, laden oder teilen | [Speicherformat, Zwischenablage und Offline-Test](Kingdom_Sets.md) |
| Regelbasierten Kingdom-Generator entwerfen | [SetGenerator](../SetGenerator/README.md) |
| Erweiterungsquoten und Steam-Import abnehmen | [Testliste zum Generator](Generator_Erweiterungen_Tests.md) |
| Offene Wünsche an den Web-Randomizer mit unserem Generator abgleichen | [KingdomCreator als Referenz](KingdomCreator.md) |
| Deutsche Kartentitel als Kingdom-IDs nachschlagen | [Kartentitel und IDs (JSON)](Card_Titles_de.json) |
| Königreichstapel, Kartentypen, Kosten und Filter nachschlagen | [Karteneigenschaften (JSON)](Card_Properties.json) |
| Kopierte Kartennamen von dominionrandomizer.com umwandeln | [Web-Namenslisten im Konverter](Kingdom_Sets.md#web-namenslisten-im-konverter) |
| Ohne Mods spielen und eigene Varianten ausprobieren | [Steam-Spielanleitung](Steam_Spielanleitung.md) |
| Kingdom-Syntax, Kampagneneffekte und Modding-Befunde nachschlagen | [Technische Referenz](Modding_Overview.md#kingdom-text-und-kampagneneffekte) |
| Native Mod für doppelte Königreichstapel untersuchen | [Aufbau, lokale Abgrenzung und Offline-Testplan](Doppelte_Koenigreichstapel_Mod.md) |
| Kampagneneffekte mit Freunden spielen | [Befund für Privates Spiel und offener Test mit Weitergeben](Modding_Overview.md#kampagneneffekte-im-privaten-online-spiel) |
| Build, Prüfmethode oder Quelle nachschlagen | [Quellenregister](Sources/Source_Register.md) |

## Belegstatus

- **Lokal beobachtet** kennzeichnet Daten aus Dateien der installierten Version.
- **Statisch geprüft** kennzeichnet einen nachvollzogenen Befund aus der EXE. Die Aussage gilt für den untersuchten Build und den beschriebenen Codepfad.
- **Vom Entwickler dokumentiert** kennzeichnet eine veröffentlichte Aussage von Temple Gates Games.
- **Von der Community berichtet** kennzeichnet ein Beispiel oder eine Anleitung aus der Spielerschaft. Die Buildzuordnung kann offen sein.
- **Im Spiel getestet** setzt einen protokollierten Lauf mit Build, Modus und beobachtetem Ergebnis voraus.
- **Offen** kennzeichnet eine Hypothese oder eine ausstehende Prüfung.

Die Profilstruktur ist lokal belegt. Der erste Spieltest soll zeigen, ob ein extern hinzugefügter Eintrag in der Set-Auswahl erscheint, sich spielen lässt und einen Neustart übersteht. Ablauf und Abnahmekriterien stehen im [Kingdom-Befund](Kingdom_Sets.md).
