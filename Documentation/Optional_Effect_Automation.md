# Optionale Effekte: lokale Automatik

## Geprüfter nativer Pfad

**Statisch geprüft am 19. November 2026:** Dominion 3315, Steam-Build 17683350, SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Alle Adressen in diesem Dokument sind RVAs dieser EXE.

`twist_garbage_heap` hat die native Effekt-ID `0x17DA`, aber die native **Anzeigekarten-ID `0x150E`**. Die gemeinsame Bezeichnung wird bei `0x259F0`, die Anzeigekarten-ID bei `0x25A52` registriert. Der Aufbau registriert den Zugbeginn-Callback `0x112BB0`. Dieser ruft `0x16C680` mit Auswahlart 1, dem Rückruf `0x112BF0`, Flag `0x10000` und abschließendem Wert 1 auf. Der Rückruf öffnet über `0x162690` die Handkartenauswahl zum Entsorgen. Die angezeigte Fähigkeits-ID `0x13` ist keine hinreichende Identifikation des Effekts.

Anfragen liegen in `G+0x3B568`, jeweils `0xA8` Byte; ihre Anzahl steht bei `G+0x19B8`. Der native Leser `0x176940` verwendet `handle >> 18` als Sequenznummer und sucht diese bei Anfrageoffset `0x2C`. Ein isolierter Aufbau mit Müllhaufen zeigte im Aktionskontext (`G+0x20`) den Typ 3 bei `+0x54` und den Anfragehandle bei `+0x58`. Die zugehörige Anfrage enthielt genau den Rückruf `Modulbasis+0x112BF0` bei `+0x54` sowie die Kennwerte `+0x4C=2`, `+0x50=1`, `+0x60=0x10000`, `+0x84=1`.

Die native Automatik `0x222940` wird von den Auswahlbauern bei `0x223499`, `0x223836`, `0x223A8F` und `0x223CFA` aufgerufen. Vor `0x2229B1` prüft sie unter anderem zuständige Spieler, Spielertyp und verfügbaren Aktionsstand. Der Zusatz wird erst danach ausgeführt. Bei einer eindeutig zugeordneten Müllhaufen-Anfrage setzt er die Antwortanzahl auf null und verwendet den vorhandenen leeren Antwortpfad `0x2232DE` → `0x2228C0`. Eine unbekannte Quelle bleibt unbeantwortet und wird regulär angezeigt.

## Anzeige und Speicherung

Die feste native Smartplay-Tabelle (`0x3E7D30`, 28 Einträge à `0xEC` Byte) enthält Müllhaufen nicht. Sie wird nicht erweitert. Die Mod liefert nur für die Anzeigekarte `0x150E` eine eigene Definition an `GetSmartplayDefs` (`0x21CE00`). Der native Renderer `0x1E43D0` kann solche Definitionszeiger direkt verwenden. Das tatsächliche Effektbanner ist ein natives Stapelobjekt (Objektart 3, Karten-ID bei `+0x30`); `DomZoom` bei `0x1CFFD0` öffnet dafür die vorhandene Großansicht.

Die private Einstellungs-ID `0x4B455001` wird ausschließlich in der Mod verwaltet. Die nativen Einstiegsstellen für Wertabfrage (`0x21CEE0`), Umschalten (`0x1E3EC0`) und Übernahme als Profilstandard (`0x1E3E50`) behandeln diese ID separat; alle übrigen IDs gehen unverändert in ihren ursprünglichen Pfad. Die Einstellung wird weder in Profile noch in Netzwerknachrichten aufgenommen. Mehrere Müllhaufen-Einträge teilen sich die Einstellung desselben lokalen Teilnehmers. Die vorhandenen Spielübersetzungen bezeichnen die Auswahl als **Fragen / Off** beziehungsweise **Ask / Off**; Off bedeutet hier, das freiwillige Entsorgen niemals anzubieten. Es wird keine Handkarte automatisch gewählt.

Die gemeinsame Ladefunktion `0x224070` setzt den Zustand zurück und führt danach ihren ursprünglichen Prolog und den unveränderten Code aus. Ihre sieben direkten Aufrufer (`0x0B0DF3`, `0x0B1753`, `0x0B1816`, `0x0B1D16`, `0x0B1EC6`, `0x0B2739`, `0x0B289E`) liegen ausschließlich in Spiel-Lade-/Startzweigen. Damit gilt das Zurücksetzen auch für die Fortsetzung geladener Partien und nicht nur für ein neues Königreich. Es wird keine gespeicherte Partie verändert.

## Prüfstand

Der bestehende `startup_effect_test --garbage-automation` bestand am 19. November 2026 die Fälle mit einem und zwei Müllhaufen-Einträgen. Der Test zeigt ausschließlich das eigene Fenster auf dem unsichtbaren Desktop, öffnet die native Großansicht des tatsächlichen Effektbanners und prüft die tatsächlich erzeugte native Smartplay-Zeile. Umschalten und Antwortübernahme erfolgten über die nativen Funktionen; keine Maus- oder Tastatureingaben wurden erzeugt.

Die Profil- und Spieler-Smartplay-Daten blieben bytegleich; ein zweiter lokaler Teilnehmer behielt Fragen. Bei zwei Effekten wurden genau zwei Anfragen automatisch abgelehnt, die Hand behielt ihre fünf Karten. Ein tatsächlicher neuer Spielstart über die native Dispatcher-Aktion 9 setzte die Einstellung wieder auf Fragen zurück. Ein anderer Anfragekontext, ein unbekannter Handle, ein fremder Rückruf und fehlende Optionalitätsmerkmale wurden nicht automatisch abgelehnt. Der gemeinsame Ladehook wurde sowohl beim ersten Aufbau als auch beim erneuten nativen Spielstart ausgeführt. Die sichtbare Abnahme im Spiel bleibt separat.

## Bereits vorhandene Automatik für die Reihenfolge

**Statischer Audit vom 19. November 2026:** Dominion hat bereits eine eigene Abhängigkeitsprüfung für Zugbeginn-Fähigkeiten. `AbilitiesCommute` bei `0x17C2F0` ist durch den nativen Assertnamen bei `0x41FD4C` identifiziert. Sie vergleicht zwei kodierte Fähigkeitshandles und verwendet unter anderem `0x17C1B0`, `0x17B720` und `0x17B970` zur Einordnung ihrer Wirkungen.

Die übergeordnete Routine `0x17F120` sucht vor dem Erzeugen einer Auswahl nach einer Fähigkeit, deren Reihenfolge gegenüber **allen übrigen angebotenen Fähigkeiten** unbedenklich ist. Die Paarprüfung erfolgt bei `0x17F1D4`. Werte 0 oder 3 verhindern die automatische Auswahl; sonst schreibt der Zweig `0x17F35D` den gewählten Handle in die Ausgabe und liefert 1 zurück. Erst wenn keine solche Fähigkeit gefunden wurde, wird über `0x17F2CC` → `0x17EC40` die native Reihenfolgeauswahl erzeugt. Eine einzelne nicht freiwillige Fähigkeit kann `0x17EC9D` ebenfalls unmittelbar übernehmen.

Ein isolierter Aufbau mit **Gut beschäftigt + Kauflaune** erreichte bereits ohne zusätzliche Reihenfolgenkorrektur zwei Aktionen und zwei Käufe; es gab keine offene Reihenfolgeauswahl. Der vorhandene Abhängigkeitsalgorithmus bleibt unverändert. Standardmäßig verwendet die Mod ausschließlich diese native Automatik.

| Effekt | Native Anzeigekarte | Ausführungsrückruf | Statisch geprüfte Wirkung |
| --- | --- | --- | --- |
| Gut beschäftigt | `0x1500` (Registrierung `0x24B80`) | `0x0FA4E0`, angelegt durch `0x112570` | `+1 Aktion` über `0x190760`, Statusbit `0x2` setzen |
| Kauflaune | `0x1501` (Registrierung `0x24C8F`) | `0x1125F0`, angelegt durch `0x1125B0` | `+1 Kauf` über `0x190760`, Statusbit `0x8` setzen |

Die zusätzliche Prüfvariante `startup_effect_test --independent-order` kontrolliert reine Bonusgruppen und Wiederholungen. Sie fragt den nativen Paarprüfer und den nativen Auswahlalgorithmus in beiden Eingabereihenfolgen ab und verwendet, falls tatsächlich eine Reihenfolgeauswahl offen ist, ausschließlich den normalen Antwortweg für erste beziehungsweise letzte Option. Vergleichsgrößen sind Aktionen, Käufe, Geld, Handkartenzahl, Statusflags und aktiver Spieler. Die Smartplay-Daten werden dabei auf unveränderte Werte geprüft. Der vollständige Lauf dieses ergänzten Audits steht zunächst aus.

**Keine pauschale Automatik für die gemeldete Mischung:** Bei Große Ideen + Müllhaufen kann Ziehen vor dem Entsorgen andere Handkarten zur Auswahl stellen. Taifun kann Stapel und Zufallsverbrauch verändern. Auch Das gewisse Etwas wird nicht allein aufgrund des gedruckten `+1 Geld` als unabhängig eingestuft: Sein Rückruf `0x1074D0` verwendet `0x161AF0`, das über `0x161D80` abhängig von Modifikatoren und Quelle in Kartenziehen statt Geld verzweigen kann. Ohne die ausdrücklich aktivierte Zusatzoption bleibt die vorhandene Reihenfolgeentscheidung für solche Situationen erhalten.

Die Müllhaufen-Ablehnung ist zusätzlich ausdrücklich auf die native Hand-Auswahlart 1 begrenzt. Eine Reihenfolgeauswahl (Aktionsart 6) mit Müllhaufen darf selbst bei **Off** keine leere Antwort erhalten. Dies ist eine Absicherung der freiwilligen Ablehnung, keine neue Reihenfolgenautomatik.

## Ausdrückliche Option: Kampagneneffekte ohne Reihenfolgenfrage abarbeiten

Die zusätzliche F8-Option ist standardmäßig aus. Sie legt für bereits fällige Kampagneneffekte die **native Angebotsreihenfolge** fest, auch wenn diese strategisch relevant sein kann. Diese Reihenfolge muss nicht der F8-Effektliste entsprechen; das Spiel gruppiert und verarbeitet seine Auslöser selbst. Die vorhandene Abhängigkeitsprüfung bleibt unverändert. Die Option betrifft die anschließend noch offenen Reihenfolgefragen.

Eine Karten-, Ja/Nein-, Ziel- oder Bonusauswahl wird dadurch nicht beantwortet. Beispiel: Große Ideen kann ohne Reihenfolgenfrage ziehen und Taifun den Stapel ersetzen. Sobald Müllhaufen eine Handkarte zum freiwilligen Entsorgen abfragt, bleibt diese echte Entscheidung offen. Die separate Einstellung Fragen/Off für Müllhaufen gilt unabhängig davon.

### Eindeutiger Ursprung statt bloßer Aktionsart

Aktionsart 6 allein genügt ausdrücklich nicht. Der vorhandene Hook bei `0x2229B1` prüft zusätzlich die Rückkehradressen der vier verifizierten nativen Stackframes:

1. `0x22349E`: Rückkehr aus der nativen Automatik in den Aktionsbauer `0x223300`.
2. `0x17EE53`: Dieser Aktionsbauer wurde vom Reihenfolgenwähler `0x17EC40` aufgerufen.
3. `0x17F2D1`: Dieser Wähler wurde vom übergeordneten Abhängigkeitsauflöser `0x17F120` aufgerufen.
4. `0x180B5B` oder `0x18C50B`: Die Abfolge stammt aus einem der beiden untersuchten Schleifenpfade für fällige Fähigkeiten (`0x180700` beziehungsweise `0x18BEE0`). Beide verarbeiten nach der ausgewählten Fähigkeit ihre verbleibende Warteschlange weiter.

Der allgemeine Aktionsbauer `0x223300` hat die direkten Aufrufer `0x163F63`, `0x17EE4E`, `0x186AC9` und `0x187E58`. Insbesondere reicht der dynamische Auswahlbauer `0x1869D0` über `0x186AC9` ebenfalls eine Aktionsart durch; die Zahl 6 darf daher nicht allein als Beweis einer Reihenfolgeentscheidung dienen. Die Herkunftskette akzeptiert ausschließlich `0x17EE4E` und die beiden darüber verifizierten Auslöserschleifen. Andere direkte Aufrufer der nativen Automatik werden bereits durch die erste Rückkehradresse ausgeschlossen.

Stackadressen müssen innerhalb der aktuellen `NT_TIB`-Stackgrenzen liegen. Andere Aufrufer des allgemeinen Aktionsbauers, etwa eine Auswahl zwischen mehreren tatsächlichen Bonusmöglichkeiten, erfüllen diese Herkunftskette nicht und bleiben unverändert. Es ist kein zusätzlicher nativer Hook dafür erforderlich.

### Fällige Kampagnenquellen und freiwillige Aktivierung

`campaign_order_sources.hpp` führt alle 56 verifizierten Anzeigekarten `0x1500` bis `0x1537` mit ihrer tatsächlichen Effekt-ID auf. Die Zuordnung stammt aus den nativen Anzeigekarten-Initialisierungen `0x24B80` bis `0x285B9` und den Effektregistrierungen; sie umfasst 27 Spielvariationen und 29 Stempel. Der abweichende Anzeigename `stamp_treasureified` ist dem registrierten Befehl `stamp_treasurified` zugeordnet. Die Zuordnung ist keine Kartenfreischaltung und keine Behauptung gleicher Wirkung.

**Zwei konservative Ausnahmen:** Die nativen Definitionen für `stamp_also_silver` (Effekt `0x1425`, deklarierte Anzeige `0x1530`) und `stamp_motivational` (Effekt `0x1427`, Anzeige `0x1531`) sind bei der nativen Eigenschaftsabfrage `0x171B30` physisch vertauscht: Abfrage `0x1530` liefert die deklarierte Anzeige `0x1531` und umgekehrt. Der Test prüft exakt diese beiden Metadaten-Aliasse anhand des vollständigen Namens und der vom nativen Parser gelieferten Effekt-ID. Ihre Rückrufe `0x10D3B0` und `0x10D4C0` erzeugen Anfragen über `0x16C680`; deren Quelle stammt aus dem nativen Ausführungskontext, nicht aus einer dort direkt angegebenen Anzeigenummer. Bis diese tatsächliche Anfragequelle gesondert belegt ist, lehnt die automatische Reihenfolge beide Anzeigenummern ausdrücklich ab. Solche Warteschlangen behalten die normale Reihenfolgefrage. Die übrigen 54 Zuordnungen werden unverändert geprüft; beide Effekte bleiben auswählbar und werden vom Spiel regulär ausgeführt.

Die gesamte angebotene Liste muss aus eindeutigen, vorhandenen nativen Anfragehandles bestehen. Jede Anfrage muss vom nativen Fähigkeitstyp 2 sein, einen gültigen nativen Rückruf und eine bekannte Kampagnenquelle beziehungsweise Quellenüberschreibung haben; deren Effekt muss außerdem tatsächlich in `G+0x1280` aktiv sein. Fehlt diese Zuordnung bei einer Karte, bleibt die gesamte Reihenfolgefrage erhalten. Gewöhnliche Karten- und Dauereffekte werden dadurch nicht automatisch gleichgesetzt.

Die native Prüfung bei `0x17EC40` und `0x17F120` behandelt **Bit 0 bei Anfrageoffset `0x60` als freiwillige Aktivierung**: Bei gesetztem Bit bleibt selbst für einen einzelnen Eintrag eine Wahl erhalten. Die Zusatzoption lehnt solche Anfragen ab. Damit wird insbesondere kein optionaler Reserve-Aufruf automatisch aktiviert. Müllhaufens fälliger Auslöser hat dieses Aktivierungsbit nicht; die eigentliche freiwillige Handentscheidung entsteht erst anschließend und wird wegen ihrer anderen Aktionsart weiterhin angezeigt.

Nach erfolgreicher Prüfung wird ausschließlich die erste vorhandene Option mit dem ursprünglichen Pfad `0x2232D7` → `0x2228C0` übernommen. Rückrufe werden nicht selbst ausgeführt, keine weiteren Karten gewählt und keine laufenden Partien um Effekte ergänzt. Die nächste vom Spiel erzeugte Reihenfolgefrage wird erneut geprüft.

### Nutzung und Prüfstand

Die F8-Einstellung gilt für den laufenden Prozess und übersteht einen neuen oder geladenen Spielstart. Sie wird nicht in das Spielprofil oder die persönlichen Smartplay-Standardwerte geschrieben. Das Zurücksetzen von Müllhaufens Fragen/Off-Einstellung beim Laden ändert diese F8-Option nicht.

Der native Antwortpfad hat bereits getrennte Zweige für lokale Partien und Netzwerkpartien: Bei Spieltyp 2 verwendet `0x2228C0` den gewöhnlichen Aktionsversand über `0x1E97A0`, lokal `0x1EE590`. Die neue Reihenfolgenoption kann daher auch für eigene Online-Aktionen verwendet werden; die vorhandenen Empfänger-, Kontroll- und Bereitschaftsprüfungen bleiben bestehen. Der aktuelle Server übernimmt die Kampagneneffekte weiterhin nicht. Die tatsächliche Online-Nutzung dieser Zusatzoption ist deshalb offen; es wird kein Netzwerkformat verändert.

`startup_effect_test --campaign-order` prüft die standardmäßig erhaltene Reihenfolgefrage, alle 56 nativen Quellenzuordnungen, Ablehnung unbekannter beziehungsweise freiwilliger Quellen und fehlender Ursprungserkennung, automatische Kartenziehung und Taifun sowie die anschließende weiterhin offene Müllhaufen-Handentscheidung. Zusätzlich prüft der Test die über einen nativen Neustart erhaltene F8-Einstellung und unveränderte Smartplay-Daten. Der vollständige Lauf vom 19. November 2026 mit Build 3315 bestand alle vier Fälle: Standard aus (Metadaten 0, Negativprüfungen 0, native Reihenfolgefrage 0); Große Ideen + Taifun (Hand 6, Aktionen/Käufe 1/1, Taifun-Payload-Prüfung 0, eine automatische Reihenfolgeantwort); zusätzlich Gut beschäftigt + Kauflaune (Hand 6, Aktionen/Käufe 2/2, Taifun-Payload-Prüfung 0, eine Antwort); mit Müllhaufen (echte freiwillige Handentscheidung bleibt offen, Prüfung 0, zwei Reihenfolgeantworten). Auch die enthaltenen Vergleiche unveränderter Smartplay-Daten und die erhaltene Einstellung nach nativem Neustart bestanden. Zwei fragliche Quellen bleiben wie beschrieben ausdrücklich ausgeschlossen.

### Lokaler Teilnehmer beim ersten Zug

Der native Erstaufbau erzeugt die Reihenfolgefrage bereits mit gesetztem lokalen Spieltyp, während der Anzeigeteilnehmer bei Clientoffset 0x7590 noch 0xFFFFFFFF ist. Die echte Anfrage hatte dabei zwei gültige Quellen (Taifun 0x1517 und Große Ideen 0x1503), Empfänger/Akteur 0 und sämtliche vier verifizierten Ursprungsframes. Ein Vergleich mit dem noch nicht gesetzten Anzeigeschlüssel würde die erste Automatik dauerhaft verhindern.

Für lokale Partien gilt die globale F8-Reihenfolgenoption deshalb für alle lokalen Menschen nach den bereits ausgeführten nativen Spieler-/Kontroll-/Bereitschaftsprüfungen bei 0x22295A bis 0x2229AB. Der dort geprüfte Spielertyp 3 ist anhand der nativen Tabelle bei 0x37F200 als AI belegt (Local = 1000, Guest = 1001). Akteur und Empfänger müssen weiterhin übereinstimmen. In Netzwerkpartien bleibt zusätzlich der Abgleich mit dem tatsächlichen eigenen Teilnehmerschlüssel bestehen. Es werden keine Teilnehmerkennungen geschrieben.

Die getrennte Müllhaufen-Einstellung erzeugt bei einem noch unbekannten Anzeigeschlüssel 0xFFFFFFFF keinen Teilnehmerplatz und liefert Fragen zurück. Damit belegt ein initialer unbekannter Schlüssel keinen der sechs echten lokalen Einstellungsplätze. Der isolierte Negativtest prüft Fragen und unveränderte Platzbelegung.

### Erster Taifun-Auslöser im Automatiktest

Der erste Taifun-Auslöser ersetzt noch keinen Stapel. `0x113740` liest den bislang gespeicherten Stapelindex aus dem Effektkontext; ohne vorherigen Index überspringt `0x1138DB` die spätere Ersetzung. Anschließend speichert `0x113943` über `0x113680` das neu gewählte Indexpaar im Anfragefeld `+0x78` für den Folgezug. Die positive Automatikprüfung verlangt deshalb die tatsächliche erste Payload-Schreibung (oberes Wort `0xFFFF`, unteres Wort als gültiger gewählter Stapelindex), die zusätzliche Handkarte und den verschwundenen Reihenfolgendialog. Sie behauptet keine sofortige Ersetzung beim ersten Spielzug.