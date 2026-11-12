# Kampagnen-Features: Untersuchung der nativen Anzeige

Stand: 12. November 2026. Build 3315, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Statische Analyse und isolierter nativer Hintergrundtest; keine Maus- oder Tastaturübernahme, keine Änderung der Originalinstallation oder normalen Profile.

## Befund und enger Kandidat

`res/ui2/campaign_msg_box` hat für `txt_features` ein festes Textrechteck von 1132 × 713 Ressourceneinheiten. Alle Effekttexte werden gesammelt übergeben; der Hintergrund wächst nicht mit. Die native Kampagnen-Aktualisierung liegt bei RVA `0x2215C0`. Ihr Bindungsaufruf an RVA `0x2218A7` besitzt Originalbytes `E8 D4 4A 04 00` und ruft RVA `0x266380` auf. ABI: ECX = Root-Handle, EDX = Bindungsatom, zwei vom Aufrufer entfernte Stackargumente (DefString-Zeiger, Index).

Der vorhandene native Parameter `textAutoscale` hat ID `0xE7`; der Wert 2 steht im Ressourcen-Enum für Shrink. Der Eigenschaftsevaluator schreibt ihn in das nachgewiesene Laufzeitfeld UI2 + `0x1564`, das die Textlayout-Routine auswertet. Ein isolierter Kandidat ersetzt ausschließlich im geladenen `txt_features`-Datensatz das redundante `hidden=false` (ID `0x89`) durch diesen Parameter. Die geerbte TextBodyThin-Eigenschaft ist ebenfalls hidden=false. `textStyle=Shadow` bleibt erhalten: Der explizite Wert 0 ist gegenüber dem Stylewert Normal=1 gerade nicht redundant.

Der Kandidat prüft Assetidentität, Widgetnamen, Nodeindex 14 von 19, 14 Eigenschaften, vorhandenen Style, dessen 18 Eigenschaften und fehlende zusätzliche Vererbung. Der Speicher muss bereits schreibbar sein. Eigenschaften werden nativ indexweise ausgewertet; eine sortierte Propertyliste ist nicht erforderlich. Kein Ressourcenfile, globaler Renderer oder fremdes Widget wird verändert.

## Dynamischer Prüfstand

Auf einem unsichtbaren Windows-Desktop wurden eine getrennte Spielkopie und temporäre Profile verwendet. Bei elf Beschreibungen war die native Anzeige vorhanden, alle elf Beschreibungen standen im Rendertext, und der Renderer erzeugte 22 Zeichenkommandos (Texte und Symbole). Sowohl Definition als auch berechnetes Laufzeitfeld enthielten 2. Der native Schriftfaktor sank von der Ressourcenvorgabe 2 auf ungefähr 1,1983. Der Textcache führte weiterhin die vollständige Zielgröße 1132 × 713; gemessene Glyphengrenzen lagen vertikal ungefähr bei −29,95 bis 704,61. Das belegt die wirksame native Skalierung, ersetzt aber keine sichtbare Lesbarkeitsabnahme.

Der 32-Einträge-Fall wurde nicht belastbar abgeschlossen. Ein mit 32 Rohbefehlen vor Patchinitialisierung geladenes Testprofil verursachte eine native XString-Magic-Assertion. Der Aufrufstapel reicht durch die native Texteffekt-/Ressourcenverarbeitung; die Ursache und ein möglicher Fixtureeinfluss sind nicht abschließend zugeordnet. Dies ist daher kein nachgewiesener neuer Produktionsfehler. Beim späteren Erzeugen nach Patchinitialisierung überlagerte zunächst die alte kingdomData-Konfiguration die neue setupData-Konfiguration; der scheinbare Erfolg enthielt tatsächlich weiterhin nur elf Texte. Nach Leeren der Überlagerung fehlte die native Kampagnenanzeige. Dieser Lauf wird ausdrücklich nicht als erfolgreicher 32-Effekte-Test gewertet.

## Entscheidung

**Keine Produktionsfreigabe.** Der Überlauf bleibt als bekanntes Darstellungsproblem offen. Vor Integration müssen insbesondere ein tatsächlich mit 32 Beschreibungen erzeugter Dialog, dessen vollständiger Rendertext, Layoutgrenzen, Lesbarkeit und erreichbare Schaltflächen geprüft werden. Die Probe darf einen fehlenden Dialog oder eine kleinere tatsächliche Textzahl nicht als Erfolg werten.

Kandidat und eigene Probequellen liegen ausschließlich unter `KingdomEffectsPatch/out/experiments/campaign-dialog/`; sie sind weder Produktionsabhängigkeit noch Releaseinhalt. Beim separaten Kompilieren ist das Projektverzeichnis `KingdomEffectsPatch` als Includeverzeichnis erforderlich. Die separat erzeugten Testbinärdateien und Rohmessungen liegen lokal unter `KingdomEffectsPatch/out/campaign-probe/` und gehören nicht in ein Release.
