# Native Korrekturen: Effekttexte, Projektsteine und Bannhilfe

Stand: **12. November 2026**, Dominion **3315**, Steam-Build **17683350**, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Alle genannten Adressen sind **RVAs**, keine virtuellen Adressen oder PE-Dateioffsets. Die Tests verwenden getrennte Prozesse, temporäre Profile und einen unsichtbaren Desktop; keine physische Maus-/Tastatureingabe und kein Online-Spielversand.

## Schuldenkosten-Stempel: Textfehler statt fehlerhafter Kostenrechnung

Der originale Befehl `stamp_cost_2D_less` enthält ein großes **D**. Die native Effektanzeige übergab diesen Namen an `LookupTranslationSafe`, das den Schlüssel kanonisiert und dabei am unveränderten Großbuchstaben scheiterte. Dadurch brach die Anzeige mit `ToKey(key, NULL) == key` ab, obwohl der eigentliche Effekt registriert ist. Die vorherige pauschale Startsperre war eine Absicherung dieses Symptoms, kein Beweis einer defekten Schuldenberechnung.

Zwei tatsächliche Aufrufer werden gezielt umgeleitet:

| Stelle | Originalbytes | Zweck |
| --- | --- | --- |
| `0x19B3D2` | `E8 59 13 0F 00` | Effekttext-Lookup |
| `0x19B68C` | `E8 9F 10 0F 00` | Zweiter Effekttext-Lookup |

Der Wrapper ersetzt ausschließlich den Textschlüssel `stamp_cost_2D_less` durch den bereits vorhandenen lokalisierten Schlüssel `stamp_minus_two_debt`. Andere Schlüssel werden unverändert weitergereicht. Native Effekt-ID, Kingdom-Befehl, Ziel und Kostenlogik bleiben unverändert. Die originale Schreibweise wird weiter gespeichert und exportiert; die kleingeschriebene Variante ist kein Ersatzbefehl.

Die globale Startsperre für diesen Effekt fällt nur weg, wenn die benötigte Textkorrektur im aktiven Patchmodus installiert ist. Die Prüfung der ursprünglichen Bytes und die transaktionale Installation bleiben erhalten. Bei fehlendem Fix bleibt der Startschutz wirksam.

### Verifizierte Spielwerte

Der isolierte Vorversuch und anschließend die **tatsächlich installierte vollständige Produktions-Patchversion im Modus 2** erreichten den ersten Zug. Im Produktionslauf wurden auch die deutschen und englischen nativen Texte kontrolliert; die Zähler der umgeleiteten Caption-Aufrufe waren ungleich null (unter anderem 5 beziehungsweise 12 Aufrufe).

| Kopien von `stamp_cost_2D_less(triumph)` | Schuldenkosten von Triumph |
| ---: | ---: |
| 0 | 5 |
| 1 | 3 |
| 2 | 1 |
| 3 | 0 |

Das belegt die einfache und mehrfache Kostenwirkung für Triumph mit Untergrenze 0. Weitere Ereignisse und Kombinationen werden dadurch nicht pauschal als einzeln im Spiel geprüft ausgegeben.

## Projektsteine: gültige Doppelbelegung freigeben, Kapazität absichern

Die native Projektsteinverwaltung bietet insgesamt höchstens **zwei Startsteine pro Spieler**. Zwei `start_with_cube`-Einträge auf demselben Projekt sind nur mit passendem `start_allow_second_cube` für genau diese Projekt-ID zulässig. Die Freigabe erzeugt selbst keinen Startstein. Zusätzliche Freigabe-Einträge erhöhen die Gesamtgrenze nicht. Drei Startsteine führten im nativen Vergleich tatsächlich zum Startabbruch.

Der Patch ersetzt deshalb die bisher zu pauschale Sperre aller doppelten Projektziele durch zwei konkrete Regeln:

1. Höchstens zwei `start_with_cube`-Einträge in der gesamten Effektliste.
2. Bei gleichem Projektziel muss ein `start_allow_second_cube`-Eintrag mit derselben Ziel-ID vorhanden sein; dessen Reihenfolge ist unerheblich.

Die Regeln gelten sowohl für Textimporte als auch für die native Effektliste, F8-Übernahme und lokalen Start. Bei Ablehnung bleibt die vorherige Konfiguration erhalten. Ein dritter Startstein und eine Doppelbelegung ohne passende Freigabe werden mit einer konkreten Meldung abgefangen.

Der Produktionslauf umfasste neben den vier Schuldenfällen fünf zulässige Projektsteinfälle: ohne Freigabe 0/1 Startstein auf Festzug, mit `start_allow_second_cube(pageant)` 0/1/2 Startsteine. Alle **neun Produktionsfälle** bestanden. Die Messung kontrolliert die tatsächlich zugeordneten Marker und den verbleibenden Vorrat; sie ist kein vollständiger Auszahlungstest jedes Projekts.

## Native Online-Bannhilfe in Deutsch und Englisch

Die native Galerie hatte in drei der vier deutschen/englischen Hilfetexte weiterhin die alte Fünferbegrenzung genannt; der deutsche `online_body` enthielt schon zuvor keine falsche Zahl. Der Patch aktualisiert beide Varianten (`dom_taptip_gallery_ban_mp_body` und `dom_taptip_gallery_ban_online_body`) konsistent in beiden Sprachen.

Es gilt eine **eigene gemeinsame Online-Liste mit bis zu 700 Karten und Querkarten**. Dies ist weder eine unbegrenzte Liste noch eine gemeinsame 700er-Grenze für Online- und lokale Banns; diese liegen in getrennten nativen Listen. Die neue Hilfe beschreibt die Online-Liste entsprechend.

### Geprüfte native Schnittstellen

| RVA | Bedeutung |
| --- | --- |
| `0x28C730` | `LookupTranslationSafe`, `__fastcall`, Schlüssel in ECX, Sprachindex in EDX; 1 = Englisch, 2 = Deutsch |
| `0x23D720` | Native XString-Konstruktion aus UTF-8 |
| `0x23D850` | Native Referenzzuweisung mit korrekter Freigabe/Erhöhung der Referenzzähler |
| `0x2378E0` | Textcache-Leerung, wie beim nativen Sprachwechsel |
| `0x7E3598` | Objekt des aufbereiteten Textcaches |
| `0x107ABFC` | Globaler Zeiger auf die Übersetzungen; aktuelle Sprache bei Objekt-Offset `+0x1F8` |

Die vier Einträge haben drei native XString-Felder; nur das erste enthält den sichtbaren Text. Die beiden Hilfsstrings sind leer. Die geprüften Metadaten sind `1,1,0` an `+0x0C/+0x10/+0x14` sowie drei Nullbytes an `+0x18…+0x1A`. Das Byte `+0x1B` ist Padding und wird weder geprüft noch beschrieben. Der Stilpräfix `{taptip_normal}` bleibt erhalten.

`ban_hint_fix.hpp` prüft UI-Thread und Prozesszugehörigkeit, Originalprefixe der benötigten APIs, vorhandene Maps und exakt passende Texte/Metadaten. Erst nach vollständiger Vorprüfung werden alle vier neuen nativen XStrings aufgebaut. Anschließend wird nur das erste Feld zugewiesen und zuletzt der aufbereitete Textcache geleert. Temporäre Referenzen werden über dieselbe native Referenzverwaltung freigegeben. Die Ausführung erfolgt nach der Patchinstallation durch einen einmaligen UI-Timer, weder unter ThreadPause noch unter dem DLL-Loader-Lock. Es wird keine neue native Sprungstelle benötigt.

Die nativen Prüfungen bestanden, unter anderem für korrekte Texte, erhaltene Metadaten und Stilpräfixe, fremde unveränderte Texte, Referenz-Ownership, wiederholten Aufruf ohne erneute Änderung sowie Sprachwechsel zwischen Deutsch und Englisch ohne Speichern einer Spracheinstellung. Sechs separate Produktionslogs bestätigen `native ban help: status=0 updated=4`, also die tatsächliche Ausführung des Timers und die Änderung aller vier Einträge. Original-Xpack, Spielsprache und normales Profil werden nicht verändert.

## Weitere Prüfungen und Aussagegrenzen

Der korrigierte F8-Speicherpfad besteht mit **468 nativen Übernahmefällen und sechs Neustartprüfungen**. Die **216 Leer-Vergleiche**, **356 Parser-/Speicherprüfungen**, native Editor-Dialogprüfung und echten Injector-/DLL-Läufe im normalen Modus sowie in der Online-Diagnose bestehen. Der normale Engine-Regressionslauf erreicht mit Große Ideen 3× weiterhin acht Handkarten im ersten Zug. [Privatstatus und Auswahllöschung](Private_Game_Clear.md)

Die Online-Effektverarbeitung des Servers wird durch diese Änderungen nicht ergänzt. Die sichtbare Ingame-Kontrolle von Tooltip, privaten Überschriften und Einladen bleibt Nutzerabnahme. Der native überlaufende Kampagnendialog wird [gesondert untersucht](Campaign_Dialog_Investigation.md) und ist zum hier dokumentierten Stand noch nicht als korrigiert nachgewiesen. [Verbleibende bekannte Punkte](Known_Issues.md)
