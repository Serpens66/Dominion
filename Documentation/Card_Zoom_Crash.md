# Absturz beim Schließen einer vergrößerten Karte

## Belegstand

19. November 2026, lokaler Nutzerlauf um 21:36:28 Uhr (Europe/Berlin). Dominion 3315, Steam-Build 17683350, Original-EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Der Minidump bestätigt die normale Steam-Installation und die DLL des lokalen Testpakets `Test-v0.1.3-order-option`. Log und Minidump verbleiben im normalen lokalen Datenordner; keine Profile oder Speicherabbilder werden ins Repository aufgenommen.

Der Nutzer beschreibt Rechtsklick auf Laboratorium mit `stamp_discounted(laboratory)` und anschließend Linksklick neben die Großansicht. Der letzte Patchlog und die gespeicherte Partie bestätigen den Rabatt sowie Taifun mit Laboratorium als Ersatzstapel. Die Effekte wurden vollständig übernommen; der letzte Patchlog endet mit 27 installierten Effekten, nicht mit einem Übernahmefehler.

## Unmittelbare Ursache: nachgewiesen

Das native Log meldet `source->move.locCurrent.type == LOC_USER` in `DomZoomDismissAll`, DomClient.cpp:49745. Die Ausnahme ist ein absichtlich ausgelöster fataler Abbruch (`0xC0000025`) nach fehlgeschlagener Zustandsprüfung; kein belegter Nullzeigerzugriff.

Die EXE war im Dump bei VA `0x000E0000` geladen. Die folgenden Angaben sind **RVAs**, keine absoluten Dumpadressen:

- `0x214CA0` → `0x214A50` → `0x214800`: Schließen und Aufräumen der Karten-Großansicht.
- `0x214800`: Eintritt in `DomZoomDismissAll`.
- Im Zweig für Darstellungsobjektart 2 wird über `object+0x24C` das zugehörige Quellobjekt gesucht. Handle, Existenz und Löschzustand werden zuvor geprüft.
- `0x214962` vergleicht `source+0x370` mit 1 (`LOC_USER`). Der Vergleich scheitert, `0x214969` springt nach `0x214A15` zur fatalen Meldung.
- Der benachbarte Zweig für Objektart 1 prüft denselben Standort bei `0x214910`, überspringt bei abweichendem Standort aber lediglich die Rückbewegung. Er bricht nicht ab.

Der Erzeugungspfad grenzt Objektart 2 weiter ein: `0x1CFFD0` ruft `ZoomAssociatedIcons` (`0x1CFDF0`) auf. Bei `0x1D0511` entstehen Großansicht-Kopien zugehöriger Symbole; `0x1D051E` speichert deren ursprünglichen Quellhandle in `+0x24C`. Der Abbruch betrifft damit die Rückführung eines begleitenden Symbols, nicht unmittelbar das Hauptkartenbild. Welches Symbol betroffen war, ist im Dump nicht enthalten. Auch der Zeitpunkt während einer möglichen Animation bleibt offen.

Damit ist das vom Nutzer beschriebene Schließen der Großansicht als Absturzpfad bestätigt. Der native Code will eine Anzeige an ihre Quelle zurückbewegen, obwohl deren aktueller Standort nicht der erwarteten Benutzerposition entspricht.

## Erster Dump-Audit: Ursache des unerwarteten Standorts noch offen

Der kleine Minidump enthält das betroffene Heapobjekt und die benötigten globalen Objekttabellen nicht. Deshalb sind dessen tatsächlicher Standort, Karten-ID und Anzeigebeziehung nicht rekonstruierbar. Rabatt-/Stempelanzeige und ein durch Taifun veränderter Vorrat sind plausible beteiligte Umstände, aber durch diesen Dump nicht als Ursache bewiesen.

Zwei statische Prüfungen finden keinen direkten Eingriff der neuen Automatik in Laboratorium, Rabattberechnung oder diesen Schließpfad. Der Smartplay-Ersatz betrifft nur die Müllhaufen-Anzeigekarte `0x150E` und die private Einstellungs-ID. Normale Karten und normale Einstellungs-IDs gehen in die ursprünglichen nativen Pfade. Der Absturzstack enthält keine Patch-DLL-Frames. Das schließt eine indirekte Beteiligung nicht aus und beweist noch keinen unabhängig in Vanilla reproduzierten Fehler.

Beim ersten Dump-Audit wurde **keine** Zustandsprüfung blind entfernt und kein Produktionspatch installiert. Für einen belastbaren Fix waren eine isolierte Reproduktion mit echter Karten-/Stempel-Großansicht und Vergleiche ohne Rabatt, ohne Taifun sowie ohne neue Automatik erforderlich. Die anschließende Reproduktion und Korrektur sind nachfolgend dokumentiert. Die bestehenden Automatiktests prüften die Müllhaufen-Großansicht und deren Einstellungszeile; sie deckten das Schließen einer rabattierten Karte nicht ab.

## Reproduktion und gezielter Fix

Die Folgeprüfung vom 19. November stellt den Zustand mit Kopien der betroffenen lokalen Partie wieder her. Der native Dispatcher erhält Aktion 5 und die negative lokale Spiel-ID; er lädt XML und Replay-Binärdatei über `0xDA880` und `0x224070`. Der Laboratorium-Vorratsstapel wird über sein tatsächliches Darstellungsobjekt vergrößert (`0x1CFFD0`), über `0x214CA0` geschlossen. Keine globalen Eingaben und keine Änderung des normalen Profils.

**Ergebnis:** In der kopierten Partie bricht derselbe Ablauf auch ohne Installation irgendeiner Produktions-Patchstelle an derselben Zustandsprüfung ab. Die Diagnose-DLL ruft dabei ausschließlich die ursprünglichen nativen Funktionen auf. Der Abbruch ist somit unabhängig vom Produktionspatch reproduziert. Auch der bisherige vollständige Patch ohne Zoomkorrektur zeigte denselben Abbruch.

Die zusätzliche direkte Beobachtung klärt den im kleinen Dump fehlenden Zustand: Die Großansicht erzeugt eine zugehörige Symbolkopie (Art 2), deren gültiges Quellobjekt Art 6 hat und an `LOC_NONE` (0) steht. Die normale Laboratorium-Großansicht mit Rabatt in einem frischen Königreich hat dagegen ein solches Quellobjekt an `LOC_USER` (1) und lässt sich mit und ohne Patch schließen. Der Fehler ist damit für den gespeicherten Aufbau aus dem Nutzerset bestätigt; der Rabatt allein reproduziert ihn nicht. Die konkrete Entstehungsstelle des unsichtbaren Quellsymbols während der vorherigen Vorratsänderungen ist damit noch nicht isoliert.

Der Fix ändert ausschließlich die vorhandene bedingte Verzweigung bei **RVA `0x214969`**:

- Original: `0F 85 A6 00 00 00`, Ziel `0x214A15`, fataler Abbruch.
- Ersatz: `0F 85 4C 00 00 00`, Ziel `0x2149BB`, vorhandene native Bereinigung für fehlende/ungültige Symbolquellen.

Bei einem anderen Standort als `LOC_USER` wird nur die temporäre Symbolkopie durch `0x1CB5A0` entfernt. Originalsymbol, Karte, Rabatt und Spielzustand werden nicht umgeschrieben. Gültige Quellen behalten die ursprüngliche Rückanimation. Ein bloßes Überspringen der Prüfung wäre unzureichend, weil die temporäre Kopie dann ohne Rückbewegung oder Löschung bestehen bleiben könnte. Der bestehende Aufräumpfad setzt danach alle Großansicht-Globals zurück. Native Nutzer des Quell-Rückverweises prüfen Index und Generation, bevor sie eine Symbolkopie verwenden.

EXE-Hash und alle sechs Originalbytes werden vor Installation geprüft; die bestehende transaktionale Installation/Rücknahme gilt unverändert. Die Korrektur ist in normaler Nutzung und Online-Diagnose aktiv; der Messmodus bleibt unverändert. Eine unabhängige statische Prüfung bestätigt Sprungziel, Registerzustand und Begrenzung auf das temporäre Anzeigeobjekt.

## Automatisierte Abnahme

`startup_effect_test --zoom-crash` besteht sechs Vergleichsfälle auf einem unsichtbaren Desktop: frischer Laboratorium-Aufbau ohne/mit Rabatt jeweils ohne/mit Produktionspatch, danach dieselbe kopierte Nutzerpartie ohne/mit Fix. Die ungepatchte gespeicherte Partie muss ausdrücklich die ursprüngliche `DomZoomDismissAll`-Meldung erzeugen; dies ist eine erwartete Fehlerkontrolle, kein erfolgreicher Schließvorgang.

Mit Korrektur wurden drei aufeinanderfolgende Vergrößern-/Schließen-Zyklen durchgeführt: zwei ohne absichtliche Wartezeit zwischen den Diagnoseaufrufen, einer mit 750 ms Wartezeit. Auch die schnellen Zyklen bestehen aus getrennten UI-Timer-Aufrufen und sind keine exakte Eingabezeitmessung. Nach jedem Zyklus wurde weitere 1000 ms auf native Animation/Bereinigung gewartet. Die temporären Symbolhandles sind dann ungültig, die Originalsymbolhandles und ursprünglichen Standorte unverändert, Großansichtsmodus 0; erneutes Öffnen bleibt möglich. Kein neuer `DomZoomDismissAll`-Abbruch.

Die Kontroll-Fixtures ohne Stempel protokollieren außerdem eine bereits ungepatcht auftretende `id != DATAID_NULL`-Meldung außerhalb dieses Schließfehlers; der Zoomvergleich und seine Bereinigung werden dadurch nicht ersetzt. Der Test behauptet keine generell fehlerfreien nativen Logs. Spielerische Ausführung aller Effekte und sämtliche anderen Symbolarten sind mit dieser gezielten Prüfung nicht abgedeckt. Die sichtbare Abnahme des neuen Fixes durch den Nutzer bleibt offen.

## Sichtbare Nutzerabnahme

Am 19. November 2026 bestätigt der Nutzer den Fix im normalen Spiel und beauftragt anschließend die Veröffentlichung des aktuellen Codes als v0.1.3. Die zuvor offene sichtbare Abnahme dieses konkreten Absturzes ist damit abgeschlossen; dies erweitert nicht den Prüfstand anderer Karten, Effekte oder Symbolarten.
