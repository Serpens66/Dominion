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

**Nutzertest vom 13. November 2026:** **Privates Spiel → Königreich → Leer** funktioniert; der Modus bleibt privat. Ein anschließender Einladungsablauf sowie die kombinierte Folge mit vorheriger F8-Übernahme sind damit nicht separat bestätigt. Die automatisierte Netzwerkprüfung bestätigt die Client-Konfiguration, keinen echten Server-/Einladungsablauf.

## F8-Übernahme ebenfalls korrigiert

Nach zusätzlicher Nutzerfreigabe wurde auch die Rücksetzung im F8-Speicherpfad von `~0x30u` auf `~0x10u` geändert. Der produktive Übernahmepfad ist in `apply_editor_effects` zusammengefasst, damit die nativen Tests exakt denselben Code ausführen wie die F8-Bestätigung. Es gibt dafür keine neue native Patchstelle. Karten, Reihenfolge, Ziele, Spielerplätze, Zeitsteuerung und andere Konfigurationen bleiben erhalten; nur die gewählten Effekte und die notwendige Set-Verknüpfung werden bearbeitet.

Die Validierung gibt bei einer Ablehnung jetzt den konkreten bekannten Startschutz beziehungsweise eine lesbare Fehlermeldung zurück, bevor echte Konfigurationsdaten geändert werden. Unbekannte IDs oder Ziele, stille Parserkürzungen, 33 Effekte und die bekannten problematischen Startkombinationen erhalten weder einen teilweisen Commit noch eine Erfolgsmeldung.

`clear_kingdom_test.exe <Testkopie> <roundtrip_probe.dll> --editor` prüft 468 echte F8-Übernahmefälle: zwei Profilarten, Patchmodi 2/3/4, drei Konfigurationskontexte, eigenes Königreich/geladenes Set und je 13 Schritte. Geprüft werden unveränderte Übernahme, Dorf/Markt gemeinsam, Reihenfolge und bewusste Wiederholung, Taifun-Ziele, sämtliche Effekte entfernen, 32/33 Einträge, ungültige IDs/Ziele und abgesicherte Startabbrüche. In sechs zusätzlichen frischen Prozessen werden Privat-Kennzeichnung, Karten und Effekte nach Neustart geprüft. Der Lauf besteht mit **468/468 Fällen**. Der echte Netzwerk-Konfigurationsbauer wird ohne Versand kontrolliert.

Bei geladenen Sets kann der native Auflöser bereits vor dem Öffnen von F8 sein Bit `0x10` zurücksetzen. Die Tests vergleichen daher den tatsächlich geöffneten Zustand unmittelbar vor der Übernahme. Die Privat-Kennzeichnung `0x20` muss bereits bei dieser Auflösung erhalten bleiben und bleibt vom F8-Fix ebenfalls unberührt.

Als zusätzliche sichtbare Abnahme offen bleibt die kombinierte Folge: **Privates Spiel → F8 → Effekte ändern → Fertig → Leer → Einladen**. Der Nutzertest vom 13. November bestätigt bisher den eigenständigen Leer-Fix. Private Online-Effektwirkung wird dadurch weiterhin nicht serverseitig ergänzt. Weitere bekannte Einschränkungen werden getrennt in [Bekannte Probleme](Known_Issues.md) geführt.
