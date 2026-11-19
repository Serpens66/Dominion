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
