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

Erst nach sichtbarer Abnahme veröffentlichen. Änderungen committen und nach GitHub übertragen. Das finale Paket aus diesem Quellstand bauen; `sourceDirty` muss `false` sein. Alle Prüfungen müssen sich auf die finalen Dateien beziehen. Reine Dokumentationsänderungen erfordern keinen neuen nativen Build, Änderungen an ausgelieferten Dateien dagegen einen neuen Paketbau und passende Prüfungen.

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
