# Geld- und Punktekarten als Effektziele

Stand 19. November 2026, Spielbuild **3315**, Steam-Build **17683350**, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`.

## Auswahl und native Unterstützung

`start_add_card` verwendet die Zielart Karte. Gold, Platin, Anwesen, Herzogtum, Provinz und Kolonie sind im generierten F8-Katalog bereits Karten- und Stapelziele. Sie sind keine Königreichstapel; Effekte mit ausschließlich dieser Zielart behalten ihre Einschränkung. In der Verdopplungsauswahl öffnet **Alle Karten** den vollständigen freigegebenen Katalog. Die anfängliche Ansicht **Königreich** umfasst die festgelegten Hauptstapel.

Eine zusätzliche Startkarte wird für jeden Spieler im Startdeck angelegt. Dies garantiert keine bestimmte erste Hand. Beispielbefehle:

```text
-x start_add_card(gold)
-x start_add_card(platinum)
-x start_double_pile(gold), start_double_pile(estate)
```

Die offizielle Kartenfreigabe bleibt erforderlich. Ein Effektziel erzeugt keinen ansonsten fehlenden Vorratsstapel; für Platin-/Koloniestapel muss das Spiel diese tatsächlich aufbauen.

**Statisch geprüft:** Der native Verdopplungswrapper bei RVA `0x110280` ruft die gemeinsame Routine bei RVA `0x10F980` mit Faktor 2 auf. Diese sucht physische Stapel ab Slot 7 bis ausschließlich `0x48`, prüft primäre und sekundäre Typ-ID und kopiert die vorhandenen Karten. Die Suche ist nicht auf die zehn Königreichstapel beschränkt. Der Provinz-Wrapper hat einen besonderen Zusatzaufruf: Ziel `0x103` (Provinz) verdoppelt außerdem einen vorhandenen Koloniestapel `0x601`. Eine Automatik muss diese Kopplung berücksichtigen, damit Kolonie nicht unbeabsichtigt mehrfach verdoppelt wird.

## Automatisierte Prüfung

Der vorhandene stille Runner `startup_effect_test.exe` erhält den Modus `--basic-targets`. Er startet über den produktiven Patch und den nativen lokalen Aufbau mit temporären Weitergeben-Profilen. 30 Fälle vergleichen 0/1/2 Anwendungen: zusätzliche Gold-/Platinkarten für beide Spieler, Verdopplung von Gold/Platin/Anwesen/Herzogtum/Provinz/Kolonie und Verdreifachung von Gold/Provinz. Gemessen werden tatsächliche Karteninstanzen beziehungsweise die native Stapelliste, nicht nur Parserwerte. Die Prüfung verwendet die vorhandene offizielle Freigabevorbereitung des isolierten Testprofils; sie ändert keine Freigaben des normalen Profils.

```text
startup_effect_test.exe <Testkopie/Dominion.exe> <roundtrip_probe.dll> --basic-targets
```

Die UI-Prüfung kontrolliert zusätzlich die getrennte Zielart Karte/Stapel gegenüber Königreichstapel sowie Zustandserhaltung der beiden Automatikoptionen beim Abbrechen und beim Online-Kontextwechsel. **Im Spiel getestet am 19. November 2026:** Alle 30 Aufbauprüfungen bestanden. Gold und Platin gelangen bei 1/2 Anwendungen in die Startdecks beider lokalen Spieler. Die geprüften Geld-/Punktestapel verdoppeln beziehungsweise verdreifachen sich wie erwartet. Der Rohbericht liegt lokal unter `KingdomEffectsPatch/out/staging/basic-targets-v0.1.3.log` und wird nicht ausgeliefert.

## Zwei getrennte Automatikoptionen

Die F8-Fußzeile enthält nebeneinander die Automatik für die zehn Königreichstapel und für tatsächlich vorhandene zusätzliche Geld-/Punktegrundstapel. Beide sind unabhängig und nur lokal aktiv. Die zweite erfasst auch Trank, sofern dessen Stapel vorhanden und freigegeben ist, jedoch weder Flüche noch Geld-/Punktekarten aus der Königreichsauswahl. Bereits ausdrücklich doppelte/dreifache physische Stapel werden übersprungen. Einstellungen gelten pro Prozess und werden erst bei Fertig übernommen; Abbrechen lässt sie unverändert.

Die zweite Automatik ergänzt normale `start_double_pile`-Einträge am bereits vorhandenen Generator-Hook RVA `0x1E9F57`, bevor Dominion die aufgelöste Königreich-Konfiguration serialisiert. Dadurch werden sie wie andere Effekte in gespeicherten Partien reproduzierbar. Eine direkte zusätzliche Kartenmutation nach Phase 4 wurde während der Umsetzung verworfen, weil die Spielstanddatei Königreich und Aktionsfolge speichert, keinen vollständigen unabhängigen Kartenbestand.

Provinz und Kolonie werden als natives Paar behandelt: Ein ausdrücklicher Doppel-/Dreifacheffekt auf eines der beiden Ziele lässt dieses Paar von der Automatik unverändert; andernfalls genügt ein Provinz-Eintrag für beide vorhandenen Stapel. Weitere freigegebene Grundziele werden einzeln ergänzt. Ein Ziel ohne tatsächlichen Vorratsstapel bleibt wirkungslos und schaltet keinen Vorrat frei. Ergänzungen erfolgen gemeinsam in einer Kopie der Effektliste. Reichen die 32 Positionen nicht aus oder ist die Freigabe unbekannt, bleibt die bisherige Liste unverändert. Es werden keine neuen nativen Adressen für diese Automatik gepatcht.

**Im Spiel geprüft:** Der Schlusslauf `--basic-automatic` bestand alle sieben Vergleichsfälle (14 temporäre Partien). Verglichen wurden echte Vorratskarten gegen die entsprechenden ausdrücklichen nativen Effektfolgen: ohne/mit Platin/Kolonien, ausdrückliche Goldverdopplung/-verdreifachung, einzelne Kolonie-/Provinzeffekte und beide Automatiken gemeinsam. Die erzeugten `gamelocal_*.xml` enthalten die normalen Verdopplungsbefehle in `kingdomData`, auch beim gemeinsamen 17-Effekte-Fall. Im Testprofil war Trank nicht freigegeben und wurde korrekt nicht automatisch ergänzt.

Benannte Sets speichern normale Effektbefehle, nicht die beiden prozessweiten Checkbox-Einstellungen. Deshalb die automatisch erzeugten Regeln nach dem Spielstart als Set speichern. Für ein bereits vorbereitetes Set lassen sich Grundkarten alternativ sofort als ausdrückliche F8-Ziele wählen und speichern.
