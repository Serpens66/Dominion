# „Leer“ erhält den privaten Spielmodus

Stand: 12. November 2026. Dominion **3315**, Steam-Build **17683350**. EXE-SHA-256: `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Alle nachfolgenden Adressen sind **RVAs**, keine virtuellen Adressen oder Dateioffsets.

## Ursache und geprüfter Löschpfad

Der Nutzer meldete: **Privates Spiel → Königreich → Leer** wechselt tatsächlich zur öffentlichen Online-Spielanlage. Der native `btn_clear`-Zweig ruft bei `0x1FF7CF` die Routine `0x1F7090` auf. Eine Suche der direkten CALL-Ziele findet für diese Routine genau diesen Aufrufer.

`0x1F70B5` führt `and dword ptr [esi+0x18], 0xFFFFFFCF` aus. Dadurch werden `0x10` und `0x20` der Konfigurationsflags gelöscht. Die Vorbereitung eines privaten Online-Spiels setzt `0x20` bei `0x22B842`; der zugrunde liegende Zweig prüft zuvor den privaten Auswahlkontext bei `0x22B7BB`. Die Oberfläche und die Fortsetzung nach der Online-Spielanlage verwenden dasselbe Bit, unter anderem bei `0x200039` und `0x1FFB08`. `0x0B3B80` übernimmt die Flags unverändert in die Netzwerk-Konfiguration. Es handelt sich daher nicht lediglich um eine falsche Überschrift.

| Wirkung | Entscheidung |
| --- | --- |
| UI-Zwischendaten der Auswahl freigeben | Beibehalten |
| Geladenes Set über die beiden Felder bei `+0x0C/+0x10` lösen | Beibehalten; gespeichertes Set nicht löschen |
| Königreich über `0x0E49D0` auflösen und Auswahl mit `0x0E42D0` leeren | Beibehalten |
| Gewählte Karten/Querkarten, zusätzliche Ziele, Auswahlvorgaben und 32 Effekte zurücksetzen | Beibehalten |
| Erweiterungsbezogene Königreich-Mengenregeln auf native Leer-/Standardwerte setzen | Beibehalten; allgemeine Erweiterungsfreigabe nicht verändern |
| Ergebnis über `0x0E4CB0` serialisieren und Profil speichern | Beibehalten |
| Privat-Bit `0x20` löschen | Korrigieren |
| Bit `0x10` löschen | Unverändert; Bedeutung nicht belastbar zugeordnet, weitere konkrete Fehlwirkung nicht belegt |

Die native Löschroutine verändert weder die Spielerplatzstruktur noch die Zeitsteuerung oder Bannlisten unmittelbar. Leere Einträge werden im nativen Setup durch eine leere ID markiert; unbenutzte Payload-Bytes müssen nicht vollständig genullt sein. Die Tests prüfen deshalb aktive IDs, serialisierten Inhalt und die nativen Standardgrenzen, nicht ein pauschales Nullsetzen des gesamten Arbeitsspeichers.

## Korrektur

Die bestehende Site-Tabelle prüft die Originalinstruktion `83 66 18 CF` vollständig und ersetzt sie durch `83 66 18 EF`. Nur das Maskenbyte ändert sich. Das Privat-Bit bleibt überall in der gemeinsamen Routine erhalten, während `0x10` weiterhin gelöscht wird. Es gibt keine neue Sprungstelle, keinen Funktions-Hook und keine nachträgliche Profilspeicherung.

Die Änderung gehört zu den Patchmodi 2/3 für lokale beziehungsweise normale Nutzung und 4 für Online-Diagnose. Messmodus 1 behält die Originalinstruktion. Build-/Hashprüfung und transaktionale Installation einschließlich Rücknahme verwenden das bestehende Verfahren.

## Prüfung

`clear_kingdom_test.exe <getrennte Dominion.exe> <roundtrip_probe.dll>` verwendet ausschließlich temporäre Profile und einen unsichtbaren Desktop. Die originale Installation und das normale Profil werden nicht beschrieben. Es werden keine Tastatur-/Mauseingaben und keine Netzwerknachrichten erzeugt.

Der Runner verwendet Einzelspieler- und Weitergeben-Profile und prüft jeweils lokale, öffentliche Online- und private Konfigurationen. Pro Kontext werden leere Auswahl, Karten/Querkarten mit Effekten einschließlich Taifun-Zielen und ein geladenes gespeichertes Set geprüft. Jeder Fall wird geleert, nochmals geleert und nach erneutem Befüllen wieder geleert. Die vier Varianten sind Vanilla, normale Nutzung, Online-Diagnose und Messmodus: zusammen **216 native Vergleichsfälle**.

Zusätzlich werden alle übrigen Konfigurationsbytes, Spielerplätze, Zeitsteuerung, beide Bannlisten, gespeicherte Sets und der benachbarte Profilbereich verglichen. Andere Königreich-Konfigurationen müssen unverändert bleiben. Der tatsächliche native Netzwerk-Konfigurationsbauer wird aufgerufen, ohne dessen Ergebnis zu senden. Die serialisierten Königreiche müssen über alle vier Varianten identisch sein; nur die vorgesehene Privat-Kennzeichnung darf abweichen. Frische Spielprozesse prüfen die gespeicherte private Auswahl nach Neustart.

Der abschließende Lauf besteht mit **216/216 nativen Vergleichsfällen**, einschließlich tatsächlich geladenem gespeichertem Set, leeren Querkarten-/Ziel-IDs und nativen Standardgrenzen. Die vier erneuten Prozessstarts bestätigen den Erhalt der gespeicherten privaten Konfiguration für normale Nutzung und Online-Diagnose mit beiden Profilarten. Alle serialisierten Königreich-Ausgaben stimmen mit Vanilla überein; Messmodus verhält sich unverändert.

Die vorhandenen 356 Parser-/Speicherprüfungen, zwölf lokalen Spielstartfälle sowie nativen F8-Dialogprüfungen bestehen ebenfalls. Der echte Release-Injector installiert die finale DLL im normalen Modus und im Online-Diagnosemodus; eine Prüfung des tatsächlich installierten Instruktionswerts bestätigt jeweils `83 66 18 EF`. Der normale Engine-Regressionslauf mit Große Ideen 3× erreicht weiterhin acht Handkarten im ersten Zug. Die UI-Dialogtests ersetzen keine echte private Online-Partie.

Die sichtbare Abnahme von Überschrift und Einladen bleibt beim Nutzer: **Privates Spiel → Königreich → Leer**. Die automatisierte Netzwerkprüfung bestätigt die Client-Konfiguration, keinen echten Server-/Einladungsablauf.

## Getrennter weiterer Befund

Der vorhandene F8-Übernahmepfad in `patch.cpp` enthält ebenfalls eine Rücksetzung mit `~0x30u`. Dieser Pfad gehört nicht zur hier freigegebenen Änderung an **Leer** und bleibt unverändert. Eine mögliche entsprechende Modusänderung beim tatsächlichen Übernehmen eines F8-Entwurfs ist damit nicht durch diesen Fix behoben und benötigt eine eigene Abnahme.
