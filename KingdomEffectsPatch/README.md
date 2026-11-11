# Dominion Kingdom Effects Patch v0.1.0

Vorabversion für die Windows-Steam-Version von Dominion, **Spielbuild 3315** (Steam-Build 17683350). Andere EXE-Versionen werden abgelehnt. Eine eigene installierte Steam-Version ist erforderlich.

## Starten

1. Das Release-ZIP vollständig in einen eigenen Ordner entpacken.
2. **Dominion-mit-Mod-starten.cmd** doppelklicken. Dominion muss vorher geschlossen sein.
3. Der Starter findet Dominion in den Steam-Bibliotheken. Falls nötig, den Spielordner eingeben. Eine spätere Änderung ist über `Dominion-mit-Mod-starten.cmd -GameDirectory "D:\SteamLibrary\steamapps\common\Dominion"` möglich.
4. Beim ersten Start wird eine getrennte Spielkopie erstellt. Das kann dauern und benötigt zusätzlichen Speicherplatz. Weitere Starts verwenden dieselbe Kopie; es findet kein Build statt. Python und Visual Studio werden nicht benötigt.
5. Auf **Bereit** warten. In **Lokal spielen** oder **Weitergeben** die Königreich-Auswahl öffnen und **F8** drücken.

Steam und Internet können eingeschaltet bleiben. Kampagneneffekte werden in privaten Online-Partien nicht übernommen. Die erweiterten Online-Banns verwenden dagegen die normale Sammlung und Bannliste des Spiels.

## Effekte bearbeiten

Die Tabelle zeigt Effektname und interne ID, Wirkung und Auslösezeitpunkt. Die Suche findet auch IDs wie `twist_mirror_loot`, `stamp_two_for_one` und `start_trash_curses`.

**Ungetestet** bedeutet, dass die Wirkung noch nicht im Spiel bestätigt wurde. Diese Effekte können gewählt werden. **Teilweise geprüft** und **Mehrfach ungetestet** unterscheiden verbleibende Prüflücken; darunter steht der genauere Prüfstand für einmalige und wiederholte Anwendung. Bekannte Startabbrüche werden mit einer konkreten Meldung verhindert.

Für **Doppelter Vorratsstapel** unter **Ziele wählen…** mehrere Stapel anhaken und mit **Fertig** übernehmen. **Weitere Kopie hinzufügen** erlaubt eine bewusste Wiederholung. Rechts stehen die aktiven Effekte in Ausführungsreihenfolge. Höchstens **32 Effekte** sind möglich. **Abbrechen** verwirft den Entwurf.

Die Kartenauswahl zeigt zunächst die bereits festgelegten Königreichstapel. **Alle Karten** öffnet den passenden Katalog. Eine offene Zufallsauswahl wird durch F8 nicht gezogen. Kartenbilder fehlen vereinzelt; Text und Auswahl bleiben verfügbar.

## Profil und Sicherungen

Der Release-Starter verwendet dein **normales Spielprofil**. Änderungen an Sets, Bannlisten und Spielfortschritt bleiben erhalten. Das Profil wird beim Beenden nicht zurückgesetzt.

Vor jedem Spielstart sichert der Starter ein verändertes `profiles.xml` unter `%LOCALAPPDATA%\DominionKingdomEffectsPatch\profile-backups`. Es bleiben höchstens **drei Sicherungen**: `profiles-1.xml` ist die neueste, `profiles-3.xml` die älteste. Ein unverändertes Profil erzeugt keine neue Sicherung. Kann die Sicherung nicht erstellt werden, startet das Spiel nicht.

Zum Wiederherstellen Dominion und den Starter schließen, das aktuelle `%APPDATA%\TempleGates\Dominion\profiles.xml` separat aufbewahren und die gewünschte Sicherung als `profiles.xml` dorthin kopieren. Eine Sicherung erfasst den Profilstand vor einem Mod-Start, nicht jeden Spielzug.

Die Einstellungen und Spielkopie liegen unter `%LOCALAPPDATA%\DominionKingdomEffectsPatch`. Bei einer unterbrochenen ersten Kopie den dort genannten `game.preparing`-Ordner umbenennen und erneut starten. Bei einer unvollständigen vorhandenen Kopie entsprechend den `3315\game`-Ordner umbenennen. Der Starter löscht solche Ordner nicht automatisch.

## Grenzen und Entfernen

- Für **Lokal spielen** und **Weitergeben**; nicht alle Effekte und Kombinationen sind abschließend geprüft.
- Online-Banns können über das vorhandene Spielmenü über fünf Einträge hinaus bearbeitet werden. Die native gemeinsame Kapazität beträgt 700 Einträge. Größere Listen wurden in privaten Partien geprüft; Matchmaking und die volle Kapazität sind offen. Der ursprüngliche Hilfetext nennt weiterhin fünf Banns.
- Bei einem Fehler der Patch-Initialisierung das Spiel schließen und den Starter erneut öffnen. Nicht mit einem fehlgeschlagenen Patch weiterspielen.
- Nach einem Dominion-Update kann der Patch bis zu einer angepassten Version nicht mehr starten.

Zum Entfernen Spiel und Starter schließen und den entpackten Modordner löschen. Optional den Mod-Datenordner unter `%LOCALAPPDATA%` entfernen, nachdem gewünschte Sicherungen aufbewahrt wurden. Die ursprüngliche Steam-Installation wird nicht verändert. Im normalen Profil gespeicherte Sets und Banns bleiben erhalten.

Quellcode, Anleitung und Befunde: [Serpens66/Dominion](https://github.com/Serpens66/Dominion). Dies ist ein inoffizielles Projekt. Die verwendeten Spielgrafiken stammen aus Dominion und gehören ihren jeweiligen Rechteinhabern.
