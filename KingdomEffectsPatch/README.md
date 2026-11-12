# Dominion Kingdom Effects Patch v0.1.0

Vorabversion für die Windows-Steam-Version von Dominion, **Spielbuild 3315** (Steam-Build 17683350). Andere EXE-Versionen werden abgelehnt. Eine eigene installierte Steam-Version ist erforderlich.

## Starten

1. Das Release-ZIP vollständig in einen eigenen Ordner entpacken.
2. **Dominion-mit-Mod-starten.cmd** doppelklicken. Dominion muss vorher geschlossen sein.
3. Der Starter findet Dominion in den Steam-Bibliotheken. Falls nötig, den Spielordner eingeben. Eine spätere Änderung ist über `Dominion-mit-Mod-starten.cmd -GameDirectory "D:\SteamLibrary\steamapps\common\Dominion"` möglich.
4. Der Starter startet Dominion direkt aus deiner Steam-Installation und aktiviert den Patch nur im Arbeitsspeicher. Es werden keine Spielkopie und kein Testprofil angelegt. Python und Visual Studio werden nicht benötigt.
5. Auf **Bereit** warten. In **Lokal spielen**, **Weitergeben** oder beim Erstellen eines **Online-Spiels / Privaten Spiels** die Königreich-Auswahl öffnen und **F8** drücken.

Steam und Internet können eingeschaltet bleiben. Kampagneneffekte werden in privaten Online-Partien nicht übernommen. Die erweiterten Online-Banns verwenden dagegen die normale Sammlung und Bannliste des Spiels.

Der Online-Editor trägt den Hinweis **Online – Wirkung serverabhängig**. Du kannst dort Effekte auswählen und für das betreffende Online-Königreich speichern. Lokal und online verwenden getrennte Spielkonfigurationen. Ob die Effekte in der Partie ankommen und wirken, entscheidet weiterhin der Server. Die automatische Verdopplung erst lokal gezogener Zufallsstapel ist online deaktiviert; explizite Verdopplungseffekte auf gewählte Stapel sind auswählbar. Deine lokale Automatik-Einstellung bleibt erhalten.

## Effekte bearbeiten

Die Tabelle zeigt Effektname und interne ID, Wirkung und Auslösezeitpunkt. Die Suche findet auch IDs wie `twist_mirror_loot`, `stamp_two_for_one` und `start_trash_curses`.

Das Kategorien-Dropdown enthält **Effektart** und **Wann/wie**. Du kannst beispielsweise nach **Zugbeginn**, **Spielaufbau**, **Dauerhaft** oder **Beim Spielen der gewählten Karte** filtern. Suche und Kategorie wirken gemeinsam. Effekte mit mehreren Auslösern erscheinen in mehreren passenden Kategorien. Alle Listeneinträge und ihre Zeilenhöhen werden sofort berechnet; der Scrollbereich wächst beim Herunterscrollen nicht nachträglich. Pfeiltasten, Bild auf/ab, Pos1 und Ende bedienen die Tabelle, Buchstaben springen zu passenden Effektnamen.

Text lässt sich direkt in einer Tabellenzelle mit der Maus markieren und mit **Strg+C** kopieren. **Esc** beendet die Textmarkierung. Das Rechtsklick-Menü bietet zusätzlich **Effektzeile kopieren** und **Alle angezeigten Effekte kopieren**. Für einen Testbericht nach **Ungetestet** suchen und die angezeigten Treffer gesammelt kopieren; der Text enthält Namen, IDs, Wirkung und Zeitpunkt. Die Tabelle selbst bleibt schreibgeschützt. Hat die Tabelle statt einer Textzelle den Fokus, markieren **Strg+A** alle angezeigten Zeilen und **Strg+C** kopiert sie.

**Ungetestet** bedeutet, dass die Wirkung noch nicht im Spiel bestätigt wurde. Diese Effekte können gewählt werden. **Teilweise geprüft** und **Mehrfach ungetestet** unterscheiden verbleibende Prüflücken; darunter steht der genauere Prüfstand für einmalige und wiederholte Anwendung. Bekannte Startabbrüche werden mit einer konkreten Meldung verhindert.

Für **Doppelter Vorratsstapel** unter **Ziele wählen…** mehrere Stapel anhaken und mit **Fertig** übernehmen. **Weitere Kopie hinzufügen** erlaubt eine bewusste Wiederholung. Rechts stehen die aktiven Effekte in Ausführungsreihenfolge. Höchstens **32 Effekte** sind möglich. **Abbrechen** verwirft den Entwurf.

Die Kartenauswahl zeigt zunächst die bereits festgelegten Königreichstapel. **Alle Karten** öffnet den passenden Katalog. Eine offene Zufallsauswahl wird durch F8 nicht gezogen. Die Grafiken für Karten und Oberfläche werden bei Bedarf direkt aus `Dominion.xpack` deiner laufenden Spielinstallation gelesen. Im Download sind keine Spielgrafiken enthalten. Es gibt keine Bilderzeugung beim ersten Start und keinen Grafik-Cache auf der Festplatte. Der gemeinsame Speicher-Cache ist auf 96 MiB begrenzt. Bei fehlenden Ressourcen bleiben Text und Auswahl verfügbar.

## Profil und Sicherungen

Der Release-Starter verwendet dein **normales Spielprofil**. Änderungen an Sets, Bannlisten und Spielfortschritt bleiben erhalten. Das Profil wird beim Beenden nicht zurückgesetzt.

Vor jedem Spielstart sichert der Starter ein verändertes `profiles.xml` unter `%LOCALAPPDATA%\DominionKingdomEffectsPatch\profile-backups`. Es bleiben höchstens **drei Sicherungen**: `profiles-1.xml` ist die neueste, `profiles-3.xml` die älteste. Ein unverändertes Profil erzeugt keine neue Sicherung. Kann die Sicherung nicht erstellt werden, startet das Spiel nicht.

Zum Wiederherstellen Dominion und den Starter schließen, das aktuelle `%APPDATA%\TempleGates\Dominion\profiles.xml` separat aufbewahren und die gewünschte Sicherung als `profiles.xml` dorthin kopieren. Eine Sicherung erfasst den Profilstand vor einem Mod-Start, nicht jeden Spielzug.

Die Einstellungen und Profilsicherungen liegen unter `%LOCALAPPDATA%\DominionKingdomEffectsPatch`. Die Dateien der Steam-Installation werden nicht verändert; nach einem normalen Start über Steam ist der Patch nicht aktiv.

## Startprotokolle

Unter `%LOCALAPPDATA%\DominionKingdomEffectsPatch` liegt `letzter-start.log` mit den Startermeldungen, der gestarteten Prozess-ID und dem Ergebnis des Injectors. `vorheriger-start.log` enthält den vorherigen Lauf; es werden keine unbegrenzt vielen Startlogs angelegt. Nach Spielende wird ein vorhandenes DLL-Protokoll als `letzter-patch.log` dorthin kopiert. Die DLL schreibt ihr Prozessprotokoll zunächst unter `%TEMP%\DominionKingdomEffects-<PID>.log`. Fehlt es, kann die Aktivierung bereits vor dem Laden der DLL gescheitert sein. Der Starter gibt die Steam-App-ID ausschließlich in der Umgebung des gestarteten Spielprozesses mit.

## Grenzen und Entfernen

- Die Effektwirkung wird in **Lokal spielen** und **Weitergeben** geprüft; nicht alle Effekte und Kombinationen sind abschließend getestet. Der Online-Editor bereitet die Auswahl vor, die Wirkung hängt vom Server ab.
- Online-Banns können über das vorhandene Spielmenü über fünf Einträge hinaus bearbeitet werden. Die native gemeinsame Kapazität beträgt 700 Einträge. Größere Listen wurden in privaten Partien geprüft; Matchmaking und die volle Kapazität sind offen. Die deutschen und englischen Online-Bann-Hilfetexte nennen nun die gemeinsame Kapazität von 700 Karten und Querkarten.
- Bei einem Fehler der Patch-Initialisierung das Spiel schließen und den Starter erneut öffnen. Nicht mit einem fehlgeschlagenen Patch weiterspielen.
- Nach einem Dominion-Update kann der Patch bis zu einer angepassten Version nicht mehr starten.

Zum Entfernen Spiel und Starter schließen und den entpackten Modordner löschen. Optional den Mod-Datenordner unter `%LOCALAPPDATA%` entfernen, nachdem gewünschte Sicherungen aufbewahrt wurden. Die ursprüngliche Steam-Installation wird nicht verändert. Im normalen Profil gespeicherte Sets und Banns bleiben erhalten.

Quellcode, Anleitung und Befunde: [Serpens66/Dominion](https://github.com/Serpens66/Dominion). Dies ist ein inoffizielles Projekt. Die verwendeten Spielgrafiken stammen aus Dominion und gehören ihren jeweiligen Rechteinhabern.

## Kartenvorschau und Effektdiagnose

Die Kartenauswahl zeigt Namen, Eigenschaften und scrollbaren Regeltext immer oberhalb der Karte. Der Bildbereich bleibt beim Laden leer und erhält anschließend die fertige Karte. Text und Bild behalten ihre Positionen. Wiederholtes Überfahren derselben Karte startet keinen neuen Ladeauftrag; vorhandene Kartenbilder werden aus dem gemeinsamen Speicher-Cache wiederverwendet.

F8 zeigt die Effekte der geöffneten Königreich-Konfiguration, nicht eine Prüfung ihrer tatsächlichen Spielwirkung. Verwirft oder ersetzt das Spiel diese Konfiguration, kann ein Effekt anschließend auch in F8 fehlen. Der isolierte Prüfbericht im [Repository](https://github.com/Serpens66/Dominion/blob/main/Documentation/Effect_Roundtrip_Tests.md) unterscheidet deshalb Übernahme und Speicherung von bestätigter Spielwirkung.

Das Patch-Protokoll enthält nun die angeforderten Effektbefehle, die eingelesenen Werte, die erneute Auflösung vor dem Speichern sowie die vorbereiteten und im lokalen Spiel angekommenen Effekte. Vollständige Profile werden nicht protokolliert. Bei einer unvollständigen F8-Übernahme bleibt die bisherige Auswahl erhalten; die Meldung nennt den betroffenen Effekt und Verarbeitungsschritt.

Zur Untersuchung eines Verlusts nach einer Partie: Effekte in F8 festlegen, mit **Fertig** übernehmen, die Partie starten und anschließend auf dem üblichen Weg ins Königreich-Menü zurückkehren. F8 dort erneut öffnen und mit **Abbrechen** schließen, damit dieser Kontrollschritt keine Änderungen speichert. Danach Dominion schließen und den Starter fertig laufen lassen. `letzter-patch.log` enthält dann den Vergleich vor und nach der Partie. Alle 89 Effekte müssen dafür nicht gleichzeitig aktiviert werden.

## Erweiterungsbezug und korrigierter lokaler Start

F8 zeigt in der Wirkungsspalte **Basisspiel ausreichend**, **Benötigt: …** oder bei unklarer Beweislage **Voraussetzung ungeklärt**. Der Detailtext erklärt Mechaniken und Selbstbereitstellung, etwa von Talern. Voraussetzungen ersetzen nicht den separat ausgewiesenen Funktionsnachweis. Der **Erweiterungsbezug im Spielcode** bleibt getrennt im Detailtext und ist weiterhin suchbar; er ist eine native Filtervorgabe, keine zwingende Erweiterungspflicht. Die Voraussetzungen sperren keine Effekte und fügen keine Karten hinzu.

Der Patch verhindert beim lokalen Start, dass dieser Erweiterungsfilter einen Effekt und alle nachfolgenden Einträge entfernt. Er schaltet keine Erweiterungskarten frei und verändert den Online-Startpfad nicht. Die größere Kartenauswahl zeigt beim Öffnen alle Erweiterungen geschlossen. Hover über einen Kartennamen, Anklicken oder Tastaturauswahl lädt eine Kartenvorschau. Über Erweiterungsnamen erscheinen stattdessen deren Symbol, Originalbeschreibung und scrollbare Regeln; Leerflächen ändern die Vorschau nicht. Die Karte nutzt bei ausreichender Bildschirmhöhe die rechte Spaltenbreite, der Text bleibt darüber stehen.

Nach einem erfolgreichen Start wurden auch natives Schließen/Neustarten und erneutes Auflösen der F8-Konfiguration geprüft. Die abschließende sichtbare Kontrolle erfolgt mit deinem normalen Starter: Achterfolge setzen, starten, zurück ins Königreich-Menü und F8 erneut öffnen. Für die drei bislang übersprungenen Effekte sind Ankunft und Spielwirkung getrennte Befunde.

## Neue Auswahlbedienung und Sprache

Für **Taifun** die gewünschte Karte anklicken oder mit den Pfeiltasten wählen und **Stapel hinzufügen** drücken; Doppelklick und Leertaste funktionieren ebenfalls. Hover allein wählt kein Ziel. Die nummerierte Liste enthält höchstens 20 verschiedene Stapel. **Entfernen** oder **Entf** bei Fokus in dieser Liste entfernt den markierten Stapel.

Taifun-Stapel und aktive Effekte per Drag-and-drop sortieren. Die Einfügemarkierung zeigt die neue Position; am Rand scrollt die Liste. **Alt+Pfeil hoch/runter** ist die Tastaturalternative. **Esc** oder Ablegen außerhalb bricht ab. Gleichnamige aktive Effekte bleiben getrennte Einträge. Die Taifun-Zielfolge bleibt erhalten; ihre spätere tatsächliche Verwendung durch das Spiel ist noch nicht vollständig geprüft.

Helle Checkboxen kennzeichnen die gewöhnliche Mehrfachauswahl. Tabellenzellen bleiben von der ersten Anzeige an markierbar, ohne beim Anklicken Schrift oder Umbrüche zu ändern. Kopierte Texte behalten vorhandene Zeilenumbrüche; nur unsichtbare Hilfszeichen in IDs werden entfernt.

Unten in F8 schaltet **Sprache: Deutsch / English** das gesamte Menü um, einschließlich Effekttexten, Teststand, Voraussetzungen, Kartenbildern, Erweiterungsregeln und Fehlermeldungen. Entwurf, Ziele, Reihenfolge, Auswahl und Filter bleiben erhalten. Deutsche und englische Namen bleiben unabhängig von der Anzeigesprache suchbar. Die Wahl wird separat in `%LOCALAPPDATA%\DominionKingdomEffectsPatch\editor.ini` gespeichert, auch nach **Abbrechen**. Die Spielsprache und dein Profil werden nicht verändert. Kartenbilder werden nach ID und Sprache innerhalb des gemeinsamen 96-MiB-Limits zwischengespeichert.

**Rotationskoller** braucht passende rotierbare Stapel aus **Verbündete**: Die oberste Gruppe gleicher Karten wird gemeinsam unter den Stapel gelegt. `A A B B C C` wird zu `B B C C A A`. Ein gewöhnlicher Dorf-Stapel verändert sich sichtbar nicht.

## „Leer“ in der Königreich-Auswahl

Der Patch korrigiert den nativen Fehler, durch den **Leer** ein privates Spiel in ein öffentliches Online-Spiel umwandeln konnte. Die Privat-Kennzeichnung bleibt sowohl im gemeinsamen Löschpfad als auch beim Übernehmen geänderter F8-Effekte erhalten. Karten, Querkarten, Effekte und zugehörige Auswahlvorgaben werden weiterhin wie im Original geleert; ein geladenes Set wird gelöst, aber nicht aus den gespeicherten Sets gelöscht. Der normale Profil-/Speicherablauf bleibt erhalten. Die sichtbare Abnahme des Einladens nach **Leer** steht noch aus.

Der tatsächliche F8-Speicherpfad wurde zusätzlich mit 468 nativen Fällen einschließlich ungültiger Entwürfe und Neustarts geprüft. Validierungsfehler zeigen eine konkrete Meldung und lassen den bisherigen Zustand vollständig erhalten. Die sichtbare Abnahme erfolgt über **Privates Spiel → F8 ändern → Fertig → Leer → Einladen**.

Der Schuldenkosten-Stempel ist nach Korrektur seiner nativen Beschriftung nutzbar; mit Triumph ergeben 0/1/2/3 Kopien 5/3/1/0 Schuldenkosten. Höchstens zwei Start-Projektsteine sind insgesamt zulässig; zwei auf demselben Projekt benötigen eine passende Freigabe für den zweiten Stein. Drei Startsteine bleiben wegen eines nachgewiesenen nativen Abbruchs gesperrt. Die native Online-Bannhilfe ist in Deutsch und Englisch korrigiert. Der bei vielen Effekten überlaufende Kampagnenhinweis wird noch untersucht; er lässt sich schließen oder über **Nicht wieder anzeigen** ausblenden. [Vollständiger Befundstatus](https://github.com/Serpens66/Dominion/blob/main/Documentation/Known_Issues.md)
