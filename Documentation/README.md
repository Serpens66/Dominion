# Dominion-Modding-Wissensbasis

Ziel ist die Steam-Version von Dominion von Temple Gates Games. Die erste Untersuchung am 9. November 2026 bezieht sich auf Spielbuild **3315**, Steam-Build **17683350**. Der [Projektleitfaden](../AGENTS.md) enthält die lokalen Pfade und den Arbeitsablauf.

| Aufgabe | Einstieg |
| --- | --- |
| Kingdom Effects Patch starten oder ein Release vorbereiten | [Release-Anleitung](../KingdomEffectsPatch/README.md) und [Release-Prüfung](../KingdomEffectsPatch/RELEASING.md) |
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
| Verlust beim lokalen Start und Erweiterungsbezug prüfen | [Korrigierter Startpfad und Erweiterungsfilter](Effect_Startup_Tests.md) |
| Privates Spiel bleibt nach „Leer“ privat | [Löschpfad, Fix und Vergleichstests](Private_Game_Clear.md) |
| F8-Bedienung, Sprache und tatsächliche Effektvoraussetzungen | [Editoraufbau und Prüfstand](F8_Editor_Usability.md) |
| Effektübernahme und Speicherung getrennt von Spielwirkung prüfen | [Isolierter Rundreise-Test aller 89 Effekte](Effect_Roundtrip_Tests.md) |
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

## Kartenbilder erstellen

Die Bilddateien liegen unter `SetGenerator/card-images`. Die HTML-Datei enthält ihre Zuordnung zu den Karten. Für neue Bilder installiere Python und Pillow und führe die Befehle im Projektordner aus. Passe den Steam-Pfad bei Bedarf an und wähle für das neue JSON-Paket einen noch unbenutzten Dateinamen.

```powershell
python -m pip install Pillow
python .\SetGenerator\extract_card_images.py "E:\ProgrammeE\Steam\steamapps\common\Dominion\Dominion.xpack" ".\SetGenerator\Dominion-Kartenbilder.json" --all-expansions
python .\SetGenerator\export_card_images.py .\SetGenerator\Dominion-Kartenbilder.json
```

Das Exportwerkzeug erzeugt die einzelnen PNG-/WebP-Dateien und aktualisiert die Bildpfade in der HTML-Datei. Geänderte Bilder erhalten neue Dateinamen für den Browsercache. Das JSON-Paket dient als lokale Zwischenstufe. Die Bilder werden erst beim Öffnen einer Kartenvorschau geladen. Lokal kannst du die HTML-Datei direkt öffnen, wenn der Ordner `card-images` danebenliegt.

## Generator pflegen

Die Quelldatei ist [Dominion-Generator.html](../SetGenerator/Dominion-Generator.html). Deutsche Texte stehen zwischen den HTML-Tags, englische Übersetzungen in `data-en`. Dynamische Texte stehen in den Sprachzuordnungen im JavaScript, etwa `tr("Deutsch", "English")`. Speichere die Datei und lade die lokale Seite neu.

Katalog und Zieh-Worker aus dem Projektordner aktualisieren und prüfen. Passe den Steam-Pfad bei Bedarf an.

```powershell
python .\SetGenerator\generator_catalog.py "E:\ProgrammeE\Steam\steamapps\common\Dominion\Dominion.xpack"
node .\SetGenerator\test_generator_solver.js
python .\SetGenerator\test_generator_catalog.py
```

Für die Katalogerzeugung wird zusätzlich `pefile` benötigt. Die passende `Dominion.exe` wird neben `Dominion.xpack` erwartet; bei abweichendem Pfad `--exe` verwenden. Der Build prüft beide Spieldateien gegen die registrierten Hashes und bettet die numerischen Profil-IDs ein. Die XML-Tests bearbeiten ausschließlich synthetische Profile.

Bei Änderungen am Generator auf `main` veröffentlicht GitHub Actions die HTML-Datei unverändert als `index.html` und legt `Dominion-Konverter.html` sowie den Ordner `card-images` daneben. Du kannst die Veröffentlichung auch über **Actions → Publish Dominion Generator → Run workflow** starten. [Workflow](https://github.com/Serpens66/Dominion/actions/workflows/generator-pages.yml)
