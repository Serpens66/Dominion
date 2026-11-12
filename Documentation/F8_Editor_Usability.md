# F8: Voraussetzungen, Sprachwahl und Auswahlbedienung

Stand: 12. November 2026, Dominion 3315, Steam-Build 17683350, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`.

## Daten und Beleggrenzen

[Effect_Requirements.json](Effect_Requirements.json) führt für alle 89 IDs eine eigene Voraussetzungseinstufung, deutsche und englische Kurz-/Langtexte sowie Belege einschließlich der jeweils ausgewerteten dokumentierten Regel. Die Einstufung beruht auf den vorhandenen nativen Spieltexten, Zielverträgen und dokumentierten Testbefunden. Sie ist keine neue Einzelprüfung der gesamten Effektimplementierung oder Bestätigung sämtlicher Spielwirkungen. Ungeprüfte Wirkungen bleiben im vorhandenen Teststand offen. Eine ungeklärte Voraussetzung kann mit `kind: unclear` ausdrücklich dargestellt werden.

Die 45 nativen Erweiterungsfilter aus [Effect_Expansion_References.json](Effect_Expansion_References.json) bleiben separat erhalten. Sie werden nicht in eine automatische Erweiterungspflicht umgedeutet. Die Wirkungsspalte zeigt die Mechanik-/Zielvoraussetzung, der Detailtext zusätzlich die native Metadatenvorgabe. Kein Effekt wird aufgrund dieser redaktionellen Einstufung gesperrt und es werden keine Karten automatisch ergänzt.

Selbst bereitgestellte Taler, Dorfbewohner, Sonderkarten und Zusatztypen brauchen nicht automatisch Karten ihrer Ursprungserweiterung. Bei Gefallen und Tableaus wird der weitere Nutzen gesondert erklärt. Großeigentümer benötigt Siegpunktmarker aus einer beliebigen passenden Quelle. Bechergläser benötigt Trank-Guthaben, Verzögertes Glück Gaben und Rotationskoller geeignete rotierbare Stapel. Die letzte Regel ist zusätzlich anhand der [offiziellen Verbündete-Regeln](https://www.riograndegames.com/wp-content/uploads/2021/09/Dominion-Allies-Rules.pdf) erklärt.

## Aufbau

`generate_effect_catalog.py` prüft die identischen ID-Mengen der deutschen Anleitung, englischen Anleitung, Voraussetzungstabelle und nativen Erweiterungsmetadaten. Kataloge enthalten nur Texte, Ressourcennamen und Zuordnungen, keine Spielbilder. Beide Sprachen verwenden dieselben stabilen Effekt-/Karten-IDs. Native Patchstellen, Effektkorrekturen, Kartenziehung und Online-Filter bleiben unverändert.

Die Sprachpräferenz liegt ausschließlich in `%LOCALAPPDATA%\DominionKingdomEffectsPatch\editor.ini` (`[editor]`, `language=de` oder `en`). Standard ist Deutsch. Speichern erfolgt sofort bei der Umschaltung; **Abbrechen** verwirft nur den Effektentwurf. `settings.json`, Spielsprache und Profil werden nicht dafür verändert. Kartenbilder werden mit ID und Sprache als Schlüssel im gemeinsamen 96-MiB-Cache gehalten. Ein asynchrones Ergebnis wird nur bei passender Karten-ID und Sprache veröffentlicht.

Die Tabelle verwendet durchgehend dieselben schreibgeschützten EDIT-Zellen. Schrift, Formatierungsrechteck und Zeilenmessung stimmen überein. Alle Zeilenhöhen werden vor Anzeige gemessen; bei Scrollbewegungen erhalten nur sichtbare Zellen Layout-Aktualisierungen. Ein Klick öffnet keine zweite Textdarstellung. Clipboard-Export entfernt nur U+200B-Hilfszeichen aus IDs und erhält explizite Zeilenumbrüche.

`ListReorder` bedient native Listen über Einfügepositionen statt über Beschriftungen. Identische aktive Effekte bleiben dadurch getrennt. Drag-Schwelle, Einfügemarkierung, Randscrollen, Escape/Außenablage, Alt+Pfeil hoch/runter und fokussiertes Entf sind gemeinsam für aktive Effekte und Taifun verfügbar. Taifun übernimmt ausschließlich die tatsächliche Baum-Auswahl; eine Hover-Vorschau ersetzt diese nicht. Es gelten weiterhin 20 eindeutige Ziele und 32 Effektplätze.

Erweiterungsbeschreibungen und Regeln stammen aus `SetGenerator.generator_catalog.load_catalog` und dessen originalen deutschen/englischen Spielressourcen. Der Dialog zeigt diese beim Überfahren des Erweiterungstexts oder bei Tastaturauswahl. Hover klappt keine Gruppen auf. Kartentexte stellen beim Wechsel zur Karte das unveränderte Text-/Bildlayout wieder her.

## Prüfstand

Die Prüfungen erfolgen ohne Vordergrundwechsel und ohne physische Maus-/Tastatureingaben. Native Steuerelementtests verwenden einen separaten unsichtbaren Desktop. Größere Typografie und begrenzte Dialoghöhe sind automatisierbar; eine echte Windows-Skalierung mit sichtbarer Ingame-Darstellung bei 125/150 Prozent bleibt Nutzerabnahme. Der finale Prüflauf besteht:

| Prüfung | Ergebnis |
| --- | --- |
| 89 deutsche/englische Effekte, Voraussetzungen, Belege und sechs konsistente Anleitungs-Spalten | Bestanden |
| Native lokale/Online-Dialoge, unveränderte Automatik, Suche/Filter, 32-Effekte-Grenze, Abbrechen | Bestanden |
| Gleiche EDIT-Zelle, Geometrie und Zeilenanzahl vor/nach Anklicken; Unicode-/Teiltextkopie mit erhaltenen Umbrüchen | Bestanden |
| Deutsch/English, erhaltener Entwurf und Auswahl, zweisprachige Namenssuche, getrennte INI-Speicherung und erneutes Laden | Bestanden |
| Taifun 0/1/20/21 Ziele, Duplikate, Button/Leertaste, gewählte Karte statt Hover, nummerierte Liste | Bestanden |
| Drag-and-drop, identische aktive Effekte, Alt+Pfeiltasten, Entf mit/ohne Listenfokus, Escape/Außenablage und Randscrollen | Bestanden |
| Geschlossene Erweiterungen, Text-Hover/Leerflächen, Erweiterungsregeln und Tastaturzugang | Bestanden |
| Große Typografie, verkleinerter Tabellenbereich, begrenzte Dialoghöhe, fehlende Bilder | Bestanden; echte Windows-DPI-Abnahme bleibt offen |
| 773 Karten × zwei Sprachen, Cache-Grenze, unterschiedliche Sprachschlüssel und veraltete Ladeergebnisse | 1546 Vorschauen ohne fehlenden Aufbau; Cache unter 96 MiB |
| Bestehende native Parser-/Speicherprüfungen | 356/356 bestanden |
| Bestehender lokaler Spielstart einschließlich Achterfolge und Schließen/Neustarten/F8-Auflösung | 12/12 bestanden |
| Finale DLL im isolierten Spielprozess, Große Ideen 3× | Installiert; 8 Handkarten im ersten Zug |

Die Tests beweisen nicht die Spielwirkung sämtlicher 89 Effekte. Der bisherige Wirkungs-/Mehrfachprüfstand bleibt unverändert. Die sichtbare Abnahme im echten Spiel erfolgt durch den Nutzer.
