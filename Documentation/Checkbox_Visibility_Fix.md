# Checkboxdarstellung im Kartendialog: v0.1.1-Testkorrektur

Stand: 13. November 2026. Dominion 3315 / Steam-Build 17683350, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Getrennte Test-EXE, temporäres Profil und unsichtbarer Windows-Desktop. Originalinstallation, normales Profil, Maus und Tastatur des Nutzers bleiben unberührt.

## Befund und Korrektur

Die in v0.1.0 gemeldeten schwarzen Kästchen waren trotz funktionierender Auswahl nicht als Checkboxen erkennbar. Die frühere Behauptung heller Checkboxen war nicht durch eine Prüfung ihrer tatsächlichen Bildpunkte im Spielprozess abgesichert. Die exakte Ursache des alten Darstellungsfehlers ist nicht abschließend nachgewiesen.

Der Kartendialog verwendet jetzt ausschließlich eigene Statusbilder ohne `TVS_CHECKBOXES`. Zustand 0 zeigt kein Bild, Zustand 1 ein leeres helles Kästchen, Zustand 2 einen dunklen Haken. Die bestehenden Maus-/Leertastenaktionen verwalten die Auswahl. Erweiterungen, Einzelwahl und Taifun bekommen keine Checkboxen. Auch beim Entfernen eines Taifun-Ziels werden keine unnötigen Statusbits mehr gesetzt.

Die Bilder entstehen als geräteunabhängige 32-Bit-BGRA-DIB mit cremeweißem Inneren, brauner Kontur und dunklem Haken. Ein erster Alpha-Versuch zeigte im Spielprozess noch schwarze Außenränder: Beim leeren 38×38-Kästchen wurden 382 nahezu schwarze Randpixel gemessen. Daher sind die tatsächlich verwendeten Bilder einschließlich ihres papierfarbenen Außenrands vollständig deckend; nur der unbenutzte Index 0 bleibt transparent. Die korrigierte Darstellung hängt weder von einer Farbmaske noch von einer Theme-Checkbox oder der Unterstützung teiltransparenter Ränder ab.

Die Bildliste gehört eindeutig dem Picker. Sie wird beim Schließen vom Baum gelöst und genau einmal freigegeben; der Destruktor deckt vorzeitige Abbrüche ab. Erzeugung oder Anbindung fehlgeschlagen: vorhandener Dialogfehler statt unbrauchbarer Auswahl. Es werden keine Spielgrafiken hinzugefügt und keine neuen nativen Spieladressen verwendet.

## Tatsächliche Prüfung im Spielprozess

`checkbox_game_test.exe <getrennte Dominion.exe> <checkbox_game_probe.dll>` öffnet den echten Kartendialog auf dem UI-Thread des isolierten Spiels. Die Probe braucht keine laufende Partie und übernimmt keine Effekte. 18 Dialogfälle bestehen: drei Schriftgrößen (9/11/14 Punkt), Ressourcen vorhanden/fehlend und Einzel-/Mehrfach-/Taifun-Auswahl.

Die Schriftgrößen sind eine Typografie-Näherung für 100/125/150 Prozent. Die Windows-Anzeigeskalierung wird nicht verändert; die sichtbare Abnahme auf dem Bildschirm des Nutzers bleibt gesondert offen.

Bei sechs Mehrfachauswahlfällen werden leeres und angehaktes Kästchen aus einem tatsächlich gerenderten TreeView gemessen. Das gesamte Bildrechteck einschließlich Außenrand muss ohne nahezu schwarze Pixel (alle RGB-Kanäle höchstens 8) bleiben. Das helle Innere und die zusätzlichen dunklen Hakenpixel werden separat geprüft:

| Schrift | Ressourcen | Größe | Helle Innenpixel leer | Zusätzliche dunkle Hakenpixel | Nahezu schwarze Pixel im gesamten Bild, leer/angehakt |
| --- | --- | --- | --- | --- | --- |
| 9 Punkt | vorhanden und fehlend | 38×38 | 576/576 | 149 | 0/0 |
| 11 Punkt | vorhanden und fehlend | 45×45 | 729/729 | 206 | 0/0 |
| 14 Punkt | vorhanden und fehlend | 58×58 | 1296/1296 | 358 | 0/0 |

Die eigene Bildlistenidentität wird vor der ersten Anzeige und nach Zeichnen, Suchen und Aufklappen verglichen. Mausnachrichten, die vollständige Leertastenfolge, beibehaltene Auswahl über mehrere Suchbegriffe und fehlende Statusbilder in Einzelwahl/Taifun bestehen. Nach jedem Dialog sind die Besitzfelder geleert. Screenshots von leerer und angehakten Darstellung wurden zusätzlich betrachtet.

Die bestehenden nativen F8-Dialogprüfungen einschließlich zusätzlicher Pixelkontrollen bestehen ebenfalls: lokale und Online-Kontexte, Sprachen, Suche, Zielauswahl, Abbrechen, 32-Effekte-Grenze, große Schrift und fehlende Ressourcen. Das bestätigt keine neue Online-Spielwirkung.

## Warntöne der Tests

Nach dem Nutzerhinweis auf Windows-Sounds wurde die nach `WM_KEYDOWN` eintreffende Leertasten-Zeichennachricht als möglicher Auslöser eingegrenzt. Der eigene Auswahlhandler fängt jetzt auch `WM_CHAR` für die Leertaste ab, statt diese zusätzlich an die native Namenssuche weiterzureichen.

Die Prüfhilfe ersetzt ausschließlich im jeweiligen Testprozess den importierten `MessageBeep`-Aufruf des tatsächlichen TreeView-Moduls durch eine zählende, lautlose Funktion. Vor Tastaturprüfungen muss dieser Schutz erfolgreich installiert sein. Die vollständigen Leertastenprüfungen bestehen ohne zusätzlichen Warnton-Aufruf; andernfalls würde der Test lautlos fehlschlagen. Beim Ende wird die Änderung zurückgenommen. Der Schutz gehört nur zu den Entwicklerprüfungen, nicht zur ausgelieferten DLL. Keine Windows-Soundeinstellung, Lautstärke oder fremde Anwendung wird verändert.

## Bereitstellung und sichtbare Abnahme

Nur v0.1.1 als separates Testpaket und GitHub-Entwurf bereitstellen. Das veröffentlichte v0.1.0 bleibt unverändert. Rohmessungen und Aufnahmen unter `out/staging/checkbox-game-ui` beziehungsweise `ui-check` bleiben aus dem ZIP ausgeschlossen.

Der Nutzer schließt Dominion, startet den neuen v0.1.1-Doppelklick-Starter und öffnet **F8 → Doppelter Vorratsstapel → Ziele wählen**. Leere helle Kästchen und dunkle Häkchen prüfen, Dorf anklicken, mit Leertaste umschalten und die Auswahl über eine Suche nach Markt/Dorf hinweg kontrollieren. Erst nach dieser Rückmeldung v0.1.1 veröffentlichen.
