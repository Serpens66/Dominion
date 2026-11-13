# Zielkarten und offizielle Spielfreigabe

Statischer Audit vom 13. November 2026, Dominion-Spielbuild **3315**, Steam-Build **17683350**, EXE-SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Alle nachfolgenden Adressen sind **RVAs** in `Dominion.exe`, keine Dateioffsets oder absoluten virtuellen Adressen. Laufzeit- und Nutzerabnahmen sind getrennt vom statischen Befund zu dokumentieren.

## Anlass und Grenze des bisherigen Befunds

Der Nutzer meldet, dass Taifun im gepatchten lokalen Spiel Seaside-Ersatzstapel bereitstellt, obwohl die Erweiterung nicht besessen wird. Im Vanilla-Vergleich habe Taifun auch mit besessenen Karten nicht gewirkt; `start_add_card` habe dort dagegen den Erweiterungsbesitz berücksichtigt. Dies sind Nutzertests, kein Beleg dafür, dass Vanilla-Taifun selbst Zugriff erlauben würde. Die vorherige pauschale Aussage, der Patch ermögliche keinen Zugriff auf nicht freigeschaltete Zielkarten, ist damit nicht ausreichend abgesichert.

Die lokale Korrektur der Effektmetadatenprüfung bleibt erforderlich und von Zielkartenbesitz zu unterscheiden: Taifun soll weiterhin mit Basisspiel-Ersatzstapeln funktionieren. Die Erweiterungszuordnung eines Effekts ist keine allgemeine Kaufschranke. Alle 89 Effekte bleiben auswählbar; allein deren ausdrücklich angegebene Zielkarten müssen offiziell spielbar sein.

## Native Freigabequelle

| Stelle | Beleg und Bedeutung |
| --- | --- |
| `0x0DAF40` | Offizieller Freigabe-Getter. `__fastcall`: ECX = Ausgabeadresse für Grant-ID-Zeiger, EDX = Ausgabeadresse für Herkunftscode-Zeiger, Rückgabe EAX = Anzahl. Anfangsbytes: `55 8B EC 83 E4 F8 81 EC 34 02 00 00`. |
| `0x0DB7A0` | Offizieller Grant-/Kartenvergleich. `__fastcall`: ECX = Grant-ID, EDX = numerische Karten-ID, Ergebnis bool in AL. Anfangsbytes: `55 8B EC 83 E4 F8 51 53 56 33 F6 89`. |
| `0x0B1E5E` | Lokaler Spielstart ruft den Getter auf; `0x0B1E6B` reicht dessen Grant-Liste und Anzahl an die native Konfigurationsnormalisierung weiter. |
| `0x1FF9F4` | Online-Spielanlage ruft denselben Getter auf, bevor `0x1FFA03` normalisiert. |
| `0x1F66C1`, `0x1F66DF` | Ein nativer Auswahl-/UI-Pfad ruft Getter und Kartenvergleich gemeinsam auf. Derselbe Aufbau erscheint bei `0x218B08` und `0x218B28`. |
| `0x380878` | Native Grant-Tabelle mit 39 Einträgen, Schrittweite `0x10C`, ID bei +0 und Grant-Typ bei +8. Der erste Eintrag ist unbenutzt. |

Die Liste stammt aus dem aktiven Profil, nicht aus den für Zufallsauswahl angehakten Erweiterungen: Cache-Anzahl bei Profiloffset `+0x7450`, bis zu 128 Grant-IDs ab `+0x7454`, Herkunftscodes ab `+0x7654`. Der Getter berücksichtigt beim Aufbau Profilfreigaben und deren Ablaufzeit sowie native kostenlose und Steam-bezogene Freigaben. Herkunftscodes werden vom Patch nicht in eine eigene Kauf-/DLC-Logik umgedeutet. Ein Grant kann auch ein Bündel oder einzelne Karten freigeben; daher bleibt der native Vergleich maßgeblich.

Die Grant-Typen werden nativ ausgewertet: Typ 1 erlaubt allgemein; Typ 2 und 3 berücksichtigen Kartenfamilie und eine native Eigenschaft; Typ 4 enthält konkrete Karten-IDs; Typ 5 verweist rekursiv auf weitere Grants. Aus dieser Implementierung allein werden keine pauschalen Erweiterungs-/Nachtkartenregeln abgeleitet. Insbesondere ersetzt eine Division der Karten-ID durch 256 den nativen Vergleich nicht.

## Adapter und Cachegrenze

`native_card_access.hpp` liest ausschließlich den bereits vom Spiel veröffentlichten offiziellen Cache und kopiert ihn in einen begrenzten Snapshot. Der native Getter wird vom Adapter **nicht aufgerufen**. Funktionsanfänge, aktives Profil, begrenzte Profil-/Cache-/Freigabezahlen und der Cache-Zähler vor/nach dem Kopieren werden geprüft. Jede verwendete Grant-ID muss in der nativen Tabelle vorhanden sein. Nicht bestimmbare Zustände und ein kalter Cache (Anzahl 0) ergeben **unbekannt**, keine vollständige Freigabe. Der Kartenvergleich wird nur mit numerischen IDs aus dem generierten Kartenkatalog verwendet.

Der Getter initialisiert einen kalten Cache nicht ohne weitere Nebenwirkungen: Bei `0x0DB1D2` ruft er `0x0E4D60` für die gefundenen Grants auf. Diese Routine markiert Freigaben als gesehen (Hashmap bei Profiloffset `+0x56DC`, Eintrag bei `0x0E4E22`), speichert das Profil (`0x0E4E2D`), liest die drei Königreich-Konfigurationen bei `+0x10`, `+0x11B8` und `+0x2360`, ergänzt grantbezogene Auswahlvorgaben und serialisiert sie zurück. Bei `0x0E502A`/`0x0E5034` wird der Setup-Text bei Konfigurationsoffset `+0x30` geschrieben; anschließend wird erneut gespeichert (`0x0E508D`). Ein Vergleich des gesamten Profils machte diesen Vorgang durch die Änderung bei Profiloffset `+0x40` sichtbar. Deshalb wird ein kalter Getter nicht vom Patch aufgerufen; diese Schreibzugriffe werden auch nicht in einem Lesetest als harmlose Cacheänderung ausgenommen.

Bei bereits gefülltem Cache liefert Dominions Getter die veröffentlichten Arrays unverändert zurück (`0x0DAF7E` bis `0x0DAFB9`), ohne Ablaufzeiten bei jedem Aufruf erneut zu prüfen. Der Patch liest diese Daten direkt und folgt der nativen Aktualisierung/Invalidierung. Ein erneuter Snapshot vor Übernahme beziehungsweise Spielstart berücksichtigt die dann bereitgestellte Liste; eine vom Spiel noch nicht invalidierte Ablaufzeit wird nicht als unabhängig neu geprüft ausgegeben. Bei kaltem Cache bleibt die Freigabe unbekannt; die native Sammlung beziehungsweise Königreich-Auswahl einmal öffnen und erneut versuchen. Normale UI-Auswahlpfade rufen den Getter selbst auf, unter anderem der Erweiterungspfad bei `0x1F4F65` und die Kartenprüfung bei `0x1F66C1`.

Der Adapter wird auf dem Spiel-UI-Thread verwendet. Ungültige Speicherzugriffe an den schmalen nativen Lese-/Aufrufgrenzen führen zu **unbekannt**. Die bestehende EXE-Hashprüfung bleibt Voraussetzung; die zusätzlichen Byteprüfungen ersetzen sie nicht. Die Standalone-Kompilierung des Headers mit MSVC x86 `/W4 /EHsc` besteht. Sie ist kein Laufzeitnachweis für die offizielle Freigabe.

## Umsetzung und Laufzeitprüfung am 13. November

Der generierte Freigabekatalog enthält dieselben Argumentvorgaben wie der F8-Katalog für alle 89 Effekte sowie 853 native Karten-/Landschaftsidentitäten und die bestehenden Alias-Zuordnungen. Die gemeinsame Validierung prüft jedes Ziel, bei geteilten Stapeln beide Felder und bei Taifun bis zu 20 Ziele. Die native Herkunfts-/Erweiterungsmetadatenprüfung eines Effekts wird dabei nicht zur Effektsperre. Der bestehende lokale Effekt-Erhaltungsfix bleibt erhalten.

F8 blendet nicht erlaubte und nicht zuverlässig bestimmbare Ziele in Katalog, Suche und Königreich-Zielen aus; leere Erweiterungsgruppen entfallen. Die aktive Effektliste und der Entwurf werden nicht gekürzt. Vor Übernahme wird ein neuer Snapshot geprüft, auch beim unveränderten Entwurf. Import und Spielstart verwenden dieselbe Zielprüfung. Der Start prüft die tatsächlich ausgewählte Konfiguration über eine Kopie der Resolverfelder; eine Ablehnung löst keine nachträgliche Reparatur und keine Kürzung gültiger Folgeeffekte aus.

Für Online-Spielanlage wird der vorhandene Hook bei RVA `0x1FFA03` nun auch im normalen Patchmodus aktiviert. Der unveränderte Erfolgszweig ruft weiterhin die native Normalisierung auf. Bei Ablehnung werden Rücksprungadresse und die zwei bei `0x1FF9FF`/`0x1FFA00` übergebenen Wörter entfernt (12 Byte), danach folgt der geprüfte Fehler-Epilog bei `0x1FF7D4`. Der native SEH-Zustand ist bereits bei `0x1FF94E` auf -1 gesetzt; es wird kein noch lebender temporärer String übersprungen. Profiländerungen, die das Spiel vor dieser Stelle selbst vornimmt, gehören nicht zur Freigabeprüfung. Netzwerkformat und Serververhalten werden nicht geändert.

Bestandene Prüfungen auf unsichtbaren Desktops mit getrennten Profilen, ohne physische Eingaben:

- Alle **853** Identitäten stimmen mit dem unabhängigen offiziellen Getter/Kartenmatcher überein: im Testfixture 346 erlaubt und 507 abgelehnt. Diese Zahlen beschreiben das Fixture, keine allgemeine Kauf-/Erweiterungsliste.
- Kalter Cache ergibt unbekannt und lässt das gesamte Profil unverändert. Nach expliziter Vorbereitung durch den nativen Getter ausschließlich im Testfixture bleibt das gesamte Profil während wiederholter Adapterabfragen unverändert, ohne Ausnahmen für einzelne Felder.
- **42** produktive Übernahme-Ablehnungen (sieben Zielkonstellationen × drei Konfigurationen × Deutsch/English), zusätzlich unveränderte ungültige Entwürfe, unbekannte Freigaben und weiterhin erlaubte ziellose Effekte. Konfiguration und interne Effektliste bleiben bei Ablehnung vollständig erhalten. Taifun mit Dorf und Markt wird unter echten Freigaben erfolgreich übernommen.
- Vier Import-/Start-Vergleiche prüfen `start_add_card` und einen nicht freigegebenen Taifun-Stapel an erster/mittlerer/letzter Position. Es erscheint kein Testdialog, weder Konfiguration noch Profil werden verändert.
- Der tatsächliche Online-Hook besteht synthetische Tests für erlaubt, verweigert und unbekannt: Stack, Register und Argumente stimmen, Ablehnung ruft den nativen Filter nicht auf. Kein Netzwerkversand findet statt.
- Vier F8-Dialoge (lokal/online × Deutsch/English) mit acht echten Zielauswahlen behalten alle 89 Effekte einschließlich Taifun, vorhandene ungültige Ziele und den vollständigen Entwurf. Nur offiziell erlaubte Ziele erscheinen; Suche, Erweiterungsgruppen und Königreich-Ziele stimmen überein.
- Die zwölf bisherigen Startfälle bestehen sowohl über `setupData` als auch `kingdomData`, einschließlich der Achterfolge, Neustart und F8-Konfigurationsauflösung. Die Prüfhilfe bereitet vor diesen Starts den offiziellen Cache so vor wie die normale Sammlung/Königreich-Auswahl. Diese Vorbereitung ist nicht Teil des ausgelieferten Adapters.
- Bestehende 356 Parserfälle, 216 Leer-Vergleiche, 468 F8-Speicherprüfungen samt Privatstatus sowie 18 gerenderte Checkboxfälle bestehen erneut. Die breit angelegten Speicherprüfungen verwenden ausdrücklich erlaubte Ziel-Fixtures; sie ersetzen nicht die oben getrennt geprüfte echte Kartenfreigabe.

Die UI-Prüfung nutzt erlaubte/abgelehnte/unbekannte Testanbieter, um den Filter reproduzierbar zu prüfen. Die nativen Vergleiche verwenden die tatsächlichen Freigaben des isolierten Clients. Eine aktuell aktive kostenlose Rotation wurde nicht eigens durch einen Kauf oder eine Kontoveränderung hergestellt; deren Behandlung folgt dem nachgewiesenen nativen Cache-/Grant-Pfad. Rohberichte und temporäre Profile bleiben außerhalb des Releases. Sichtbare Nutzerabnahme des zusätzlichen Freigabefixes und Veröffentlichung von v0.1.1 bleiben offen.

## Nutzerabnahme und Veröffentlichung

Am 13. November 2026 bestätigt der Nutzer die Funktion der Zielkartenfreigabe und beauftragt die Veröffentlichung von v0.1.1. Die Checkboxdarstellung war bereits bestätigt. Die frühere Veröffentlichungssperre ist damit aufgehoben; das alte Patch-Release v0.1.0 und veraltete lokale Patch-Pakete werden auf ausdrücklichen Wunsch entfernt. Historische Messungen und Testgrenzen bleiben dokumentiert.
