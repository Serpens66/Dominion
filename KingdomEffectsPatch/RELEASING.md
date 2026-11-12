# Release v0.1.0

Der Release-Starter ist vom bisherigen Test-Starter getrennt. Er startet die originale Steam-Installation mit dem normalen Profil. Keine Spielkopie und keine Profilordner-Umschaltung. Der Test-Starter bleibt für Diagnosezwecke erhalten. Ein begonnenes altes Profil-Swapping muss zuerst über diesen Test-Starter abgeschlossen werden.

## Bauen und prüfen

`Build-Release.ps1` baut mit `build.cmd staging`, führt die Effektprüfungen aus und erzeugt unter `out/releases/<Zeitpunkt>` ein ZIP mit Prüfsumme. `-SkipBuild` verwendet den vorhandenen Staging-Build. Entwicklungsabhängigkeiten bleiben ausschließlich beim Ersteller: Visual Studio C++ x86, Python und Pillow sowie die lokal installierten Spielressourcen. `DOMINION_PYTHON` und `DOMINION_GAME_DIRECTORY` überschreiben die lokalen Build-Vorgaben.

Das Paket enthält eine explizite Dateiauswahl: Release-Skripte, Anleitung, Versionshinweise, fertigen Injector, Patch-DLL. Es enthält keine Grafikdateien oder eingebetteten Spielbilder. Der Build erzeugt nur Ressourcenpfade und Katalog-/Layoutmetadaten; der Patch liest Xpack, QOI-Texturen und Atlas-Ausschnitte zur Laufzeit. Keine Spiel-EXE, Xpack-Datei, Profile, Testprogramme, Logs oder Compiler-Zwischendateien. `package.json` verzeichnet Version, unterstützte EXE, Quellcommit, uncommittete Änderungen und SHA-256 der Nutzdateien. Beim Start werden die beiden Binärdateien geprüft. Einzelne fehlende Bilder dürfen auf Textdarstellung zurückfallen.

Die Prüfprogramme werden nicht mitgeliefert:

- `test_game_resources.py <Dominion.xpack>` mit `resources_test.exe`: native Bilddaten gegen Pillow vergleichen, alle 773 Vorderseiten aufbauen, Cache-Grenze und asynchrone Anfragen prüfen. Fehlerfälle umfassen beschädigten Index, abgeschnittenes QOI, übergroße Bilder und zyklische Atlasreferenzen. Testbilder werden ausschließlich unter dem ignorierten `out/staging/resource-check` erzeugt.

- `effects_test.exe`: Effektzusammenführung, Zielfelder, Kapazität und Eingabevalidierung.
- `launcher_test.exe`: erfolgreiche Initialisierung, Initialisierungsfehler und begrenztes Warten ohne Beenden des Remote-Threads.
- `clipboard_test.exe`: Unicode-Nutzdaten und HGLOBAL-Eigentumsübergabe mit echten Speicherfunktionen und isolierter Clipboard-API-Grenze; belegte Zwischenablage und fehlgeschlagene Veröffentlichung. Öffnet keine Nutzer-Zwischenablage. Der Dialogtest prüft zusätzlich Textausschnitt, ganze Zelle, Effektzeile und alle gefilterten Zeilen in einem eigenen Ausgabepuffer.
- `editor_release_test.exe`: echter F8-Dialog auf eigenem unsichtbaren Desktop; 89 Einträge, Prüfkennzeichnungen, Suche nach IDs und Zeitpunkt, leere Suche und Abbrechen. Mit DOMINION_RESOURCE_TEST_PACKAGE zusätzlich die tatsächliche asynchrone Kartenauswahl samt schnellem Wechsel, Kartentext und Schließen während des Ladens.
- `test_release_catalog.py`: Vollständigkeit und Zuordnung aller 89 Effekte und Prüfkennzeichnungen.
- `Test-Release.ps1 -ArchivePath <ZIP> -SourceGameDirectory <Spielordner>`: unter Windows PowerShell 5.1 außerhalb des Projektordners entpacken und alle Paketdateien prüfen; ausschließlich erzeugte Testprofile, Sicherungsrotation, abweichender Steam-Pfad, Leerzeichen/Umlaut, direkter Installationsmodus, falscher Build/Hash, fehlende Dateien und exklusive Sitzungssperre.
- `release_game_test.exe <Spielkopie/Dominion.exe> <entpacktes-Paket/bin> <Original/Dominion.exe> <0..3>`: tatsächlichen Paket-Injector und die DLL auf unsichtbarem Desktop prüfen. Der Test verwendet ein temporäres Offline-Profil und prüft bei zwei lokalen Spielern die erste Hand ohne bzw. mit mehreren Große-Ideen-Kopien. Er prüft den direkten Installationsmodus mit identischer Prozess- und Quelldatei, die Identitätsprüfung und den installierten Bann-Patch. Nur dieser Entwickler-Test nutzt eine vorhandene Testkopie und ein temporäres Profil; der Release-Starter tut das nicht. Kein Test verwendet das Nutzerprofil oder simuliert Eingaben am aktiven Desktop.

Vor nativen Tests Spielbuild, Steam-Build und EXE-Hash gegen das Quellenregister prüfen. Durch die Sandbox verweigerte Desktop-Erstellung oder Dateiumbenennung ist kein bestandener Test. Solche Prüfungen mit entsprechender Ausführungsfreigabe wiederholen.

## Sichtbare Schlussabnahme

Das zu veröffentlichende Paket über **Dominion-mit-Mod-starten.cmd** starten. Es darf keinen Compiler und keine Bilderzeugung aufrufen. Das Spiel muss direkt aus der gewählten Steam-Installation starten. Weder eine Spielkopie noch ein Testprofil darf angelegt werden.

Dieses Königreich in die Kartensuche kopieren:

```text
-k cellar, market, merchant, militia, mine, moat, remodel, smithy, village, workshop
```

In F8 nach `start_double_pile` suchen, Dorf und Markt gemeinsam übernehmen und das Effektset speichern. In **Lokal spielen** die Folge Effektset → normales Set → Effektset prüfen: **20/20 → 10/10 → 20/20**. Eine weitere bewusste Dorf-Kopie muss **40** Dorf ergeben. Eine Partie in **Weitergeben** starten, F8-Vorschau und Speicherung nach Neustart prüfen. Einen als **Ungetestet** markierten Effekt auswählen und den Entwurf abbrechen; keine Wirkung dieses Effekts muss dafür als getestet eingestuft werden.

Das normale Profil muss aktiv sein. Die Sicherungen unter `%LOCALAPPDATA%/DominionKingdomEffectsPatch/profile-backups` dürfen höchstens drei feste XML-Dateien umfassen. Es gibt keine Rücksetzung nach dem Spielende. Die sichtbare Abnahme bleibt bis zur Rückmeldung des Nutzers offen; insbesondere die tatsächliche Windows-Skalierung bei 125 % und 150 % nicht als geprüft ausgeben.

## GitHub-Veröffentlichung

Für die Online-Abnahme **Privates Spiel → Königreich-Auswahl → F8** öffnen. Der Titel muss **Online – Wirkung serverabhängig** enthalten und die lokale Automatik muss deaktiviert sein. Dorf und Markt als explizite Verdopplungsziele übernehmen, den Dialog erneut öffnen und nach einem Spielneustart dieselbe Online-Konfiguration prüfen. Anschließend zur lokalen Auswahl wechseln und deren bisherigen Effekte und Automatik prüfen. Dafür muss keine Online-Partie gestartet werden; die Übernahme der Effekte durch den Server ist kein Abnahmekriterium für diese Editor-Erweiterung. Der native Entwickler-Test unterstützt zusätzlich `--online-diagnostic` zur Prüfung des F8-Hooks im Patchmodus 4 ohne Erstellen einer Online-Partie.

Nach sichtbarer Abnahme oder ausdrücklicher Veröffentlichungsfreigabe des Nutzers veröffentlichen. Die Freigabe für das erste öffentliche v0.1.0 erfolgte am 13. November 2026; verbleibende offene Tests sind in der Anleitung weiterhin als offen zu kennzeichnen. Änderungen committen und nach GitHub übertragen. Das finale Paket aus diesem Quellstand bauen; `sourceDirty` muss `false` sein. Alle Prüfungen müssen sich auf die finalen Dateien beziehen. Reine Dokumentationsänderungen erfordern keinen neuen nativen Build, Änderungen an ausgelieferten Dateien dagegen einen neuen Paketbau und passende Prüfungen.

Mit der vorhandenen GitHub-CLI-Anmeldung zunächst `gh api user --jq .login` prüfen. Das bestehende Release `converter-v1.0.0` und die Generator-Pages nicht verändern. Ein neues Release mit Tag **v0.1.0**, Titel **Dominion Kingdom Effects Patch v0.1.0**, `--prerelease` und `--latest=false` erstellen. ZIP und SHA256SUMS.txt hochladen, Versionshinweise über `--notes-file` aus RELEASE-NOTES.md übernehmen und das Ziel auf den geprüften Quellcommit setzen. Anschließend Download-Dateinamen, Prüfsumme und Vorabversionsstatus über GitHub prüfen. Veröffentlichung und Link in der Dokumentation nachtragen.

## Native Effekt-Rundreise

Nach `build.cmd staging` ausführen (Entwicklertest, nicht Teil des Downloads):

```powershell
$env:SteamAppId = '1131620'
$env:SteamGameId = '1131620'
.\out\staging\roundtrip_test.exe '<getrennte Testkopie>\Dominion.exe' '<Projekt>\KingdomEffectsPatch\out\staging\roundtrip_probe.dll'
```

Der Runner verweigert die Original-EXE als Testkopie und prüft deren Build-3315-Hash. Zwei isolierte Spielprozesse verwenden temporäre Profile und einen unsichtbaren Desktop. Der eigentliche Test läuft auf dem Spielthread, ohne Maus-/Tastatureingaben. `effect-roundtrip.tsv` entsteht neben der Test-DLL. Beide Testdateien und der Rohbericht bleiben aus dem Release-Paket ausgeschlossen. Der veröffentlichte Bericht enthält nur Effektbefehle und Prüfergebnisse.

## Zusätzlicher Startpfad-Test

`startup_effect_test.exe <Testkopie> <roundtrip_probe.dll> setupData 1` prüft die zwölf dokumentierten Startfälle einschließlich Achterfolge und nativem Schließen/Neustarten. `kingdomData` vergleicht den bisherigen Weg; `0` statt `1` protokolliert den ungepatchten Referenzlauf. Der Runner bleibt aus dem Release ausgeschlossen. Erweiterungsdaten stammen aus `Documentation/Effect_Expansion_References.json`; deren IDs und EXE-Hash werden beim Katalogbau geprüft.

## Bilinguale Oberfläche und Voraussetzungen

Vor Paketbau `test_release_catalog.py` ausführen. Die ID-Mengen beider Anleitungsfassungen, `Effect_Requirements.json` und der nativen Erweiterungsmetadaten müssen alle 89 Effekte enthalten. Die sechs Anleitungs-Spalten werden ebenfalls geprüft.

`editor_release_test.exe` läuft auf einem unsichtbaren Desktop, mit separater Test-INI statt Nutzerpräferenzen. Der Lauf deckt bleibende Textzellen, Umbrüche, Deutsch/English, Taifun-Kapazität und die gemeinsame Sortiersteuerung ab. `resources_test.exe` baut 773 Vorschauen in beiden Sprachen auf und prüft den gemeinsamen Cache sowie die Sprachzuordnung asynchroner Ergebnisse. Die Berichte und Screenshots in `out/staging` sind nicht Bestandteil des Pakets. [Aktueller Prüfstand](../Documentation/F8_Editor_Usability.md)

## Native Leer-Prüfung

`clear_kingdom_test.exe <Testkopie> <roundtrip_probe.dll>` prüft Vanilla, normale Nutzung, Online-Diagnose und Messmodus mit temporären Einzelspieler-/Weitergeben-Profilen auf einem unsichtbaren Desktop. Es sendet keine Netzwerknachrichten. Der Bericht `clear-kingdom.tsv` liegt neben der Test-DLL; je Lauf entstehen 27 Fälle, über beide Profile und vier Varianten insgesamt 216. Der Test startet zusätzliche Prozesse zur Kontrolle der gespeicherten privaten Konfiguration. [Befund und Aussagegrenzen](../Documentation/Private_Game_Clear.md)

`clear_kingdom_test.exe <Testkopie> <roundtrip_probe.dll> --editor` prüft zusätzlich 468 direkte Aufrufe des produktiven F8-Übernahmepfads in beiden Profilarten, normalen/Diagnose-Patchmodi und drei Konfigurationen. 32/33 Effekte, Wiederholungen, Reihenfolge, Taifun, Entfernen, unveränderte Entwürfe, ungültige Befehle/Ziele und bekannte Startsperren werden abgedeckt. Der Bericht `editor-apply.tsv` bleibt aus dem Release ausgeschlossen. Sechs Neustarts kontrollieren gespeicherte Karten, Effekte und Privatstatus.

## Gezielt korrigierte native Effekte und Bannhilfe

`native_effect_regression_test.exe <Testkopie> <roundtrip_probe.dll> --production` installiert den vollständigen normalen Patch und prüft neun gültige Engine-Fälle: Triumph ohne sowie mit 1/2/3 Schuldenstempeln, Festzug ohne Freigabe mit 0/1 und mit passender Freigabe mit 0/1/2 Startsteinen. Die nativen deutschen/englischen Anzeigenamen werden abgefragt; bei Schuldenstempeln wird außerdem die tatsächlich durchlaufene Textkorrektur gezählt. `--exploratory` ist ein ausdrücklicher Entwicklermodus für ungefilterte Referenzfälle einschließlich möglicher nativer Abbrüche und gehört nicht zur Release-Abnahme.

`clear_kingdom_test.exe <Testkopie> <roundtrip_probe.dll> --ban-hints` kontrolliert native Texte, Referenzverwaltung, Metadaten, Sprachwechsel und wiederholte Anwendung. Zusätzlich muss der normale Produktionslauf `native ban help: status=0 updated=4` protokollieren. [Befunde und RVAs](../Documentation/Native_Effect_Fixes.md)

## Such-/Filtervergleich des F8-Dialogs

Nach `build.cmd staging` den isolierten `editor_search_test.exe <Bericht.tsv>` ausführen. Für einen Vergleich vor und nach einer Änderung denselben Rechner, Buildmodus und dieselbe Query-/Kategorienfolge verwenden. Der Test verwendet einen unsichtbaren Desktop und keine physischen Eingabegeräte; `UpdateWindow` zählt zur gemessenen Aktualisierung. Alle 89 Zeilen werden vorab besucht, Runde 0 dient dem Aufwärmen. Pro Sprache werden anschließend 54 Aktionen aus zwei Runden mit je 27 Aktionen gemessen. Testinterne Zähler müssen bei warmer Suche unveränderte Anzahlen von Zell-Erzeugung, Zell-Löschung und Höhenmessung bestätigen.

Median, 95. Perzentil und Maximum für Deutsch und Englisch getrennt dokumentieren. Rohberichte bleiben außerhalb des Release-Pakets. Die erste Vergleichsmessung steht im [F8-Prüfstand](../Documentation/F8_Editor_Usability.md#suche-und-filter-wiederverwendung-und-vergleichsmessung). Der finale Fontcache-Build ist dort separat gemessen; nach weiteren Layout-/Cacheänderungen erneut prüfen. Die sichtbare Ingame-Prüfung bleibt eine eigene Nutzerabnahme. Ein erfolgreicher Benchmark veröffentlicht keinen GitHub-Entwurf.
