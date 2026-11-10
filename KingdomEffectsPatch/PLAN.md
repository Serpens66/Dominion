# Patch für kombinierbare Dominion Effekte

Entwurf vom 10. November 2026 für Dominion auf Steam, Spielbuild **3315**, Steam-Build **17683350**. Ziel ist ein Set, das beim ersten Laden seine eigenen Effekte enthält und sich danach über **Privates Spiel** verwenden lässt. Die erste Version übernimmt bis zu **32 aktive Effekte im nativen Kingdom-Format**. Zehn Einträge genügen für die Verdopplung eines normalen Königreichs.

## Umfang und Reihenfolge

1. **Import und Set-Auswahl reparieren.** Mehrere Effekte mit unterschiedlichen Zielen direkt übernehmen. Beim Wechsel von Set A zu Set B die Regeln von B aktivieren. Das zunächst mit festen Königreichen offline abnehmen.
2. **Automatische Verdopplung ergänzen.** Beim Erstellen eines Sets mit der Option „Gezogene Königreichstapel verdoppeln“ zehn konkrete Effekte aus der nativen Ziehung erzeugen. Diese Phase beginnt nach der Abnahme des festen Sets. Ist die endgültige Ziehung vor dem Versand nicht verlässlich verfügbar, bleibt der Schalter für private Partien deaktiviert, bis der Client ein vollständig festgelegtes natives Setup erzeugen kann.
3. **Private Einladung prüfen.** Der Host erstellt das Set mit Patch und lädt anschließend ein. Die Annahme für den Test ist, dass der Dienst gültige native Effekte akzeptiert. Der Gast verwendet zunächst den normalen Client. Serverantwort und Spielverlauf entscheiden, ob das Ergebnis als online kompatibel gilt.

Der Patch setzt an der Set-Konfiguration an. Die Spielengine führt die vorhandenen Effekt-Callbacks aus. Die ältere Untersuchung zum Eingriff in die Vorratslisten bleibt als Option für besondere lokale Regeln relevant. [Befund zum lokalen Vorratsaufbau](../Documentation/Doppelte_Koenigreichstapel_Mod.md)

## Statisch geprüfte Eingriffspunkte

Die RVAs gelten für `Dominion.exe` mit SHA-256 `56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877`. Adressen im Prozess ergeben sich aus geladener Modulbasis plus RVA. Die PE-Dateioffsets dienen der Prüfung der Originaldatei. Die Stellen sind Kandidaten für die Laufzeitmessung. [Quellenregister](../Documentation/Sources/Source_Register.md)

| RVA | Dateioffset | Originalbytes | Zweck |
| --- | --- | --- | --- |
| `0x1F6F17` | `0x1F6317` | `E8 94 6C EE FF` | Parseraufruf im Suchfeld. Rohtext und Grenze vor dem Aufruf messen. |
| `0x1FDFF0` | `0x1FD3F0` | `E8 AB DE 02 00` | Zweiter UI-Importweg vor der Zusammenführung. Herkunft des Textes und Abbruchpfad messen. |
| `0x1F6A90` | `0x1F5E90` | `53 8B DC 83 EC 08` | Gemeinsamer Einstieg. Direkte Aufrufe bei `0x1F6F2F` und `0x1FE032`. Kandidat für die Prüfung vor dem ersten Schreibzugriff. |
| `0x1F6C74` | `0x1F6074` | `81 C7 10 17 00 00` | Start der Effektzusammenführung. Kandidat für die Übernahme der geprüften Liste, danach weiter bei `0x1F6CD3`. |
| `0x20FA39` | `0x20EE39` | `E8 42 F5 FF FF` | Auflösen der neu gewählten Set-Konfiguration in einen temporären Puffer. |
| `0x20FB27` | `0x20EF27` | `E8 84 51 ED FF` | Serialisieren des Set-Wechsels. Kandidat für die Übernahme der neuen Effekte bei Wahl eines Profilsets. |
| `0x1FFA03` | `0x1FEE03` | `E8 E8 C4 ED FF` | Online-Setup prüft Freischaltungen und serialisiert die Konfiguration erneut. Messpunkt vor und nach der Filterung. |
| `0x1FFAF5` | `0x1FEEF5` | `E8 26 BC 08 00` | Versand der nativen Nachricht zur Spielanlage. Messpunkt für den ausgehenden Kingdom-Text. |

Ein geparstes Setup ist `0x1990` Bytes groß. Seine Effekte belegen ab `+0x1710` genau `0x280` Bytes, also 32 Einträge zu `0x14` Bytes. Die ID steht bei `+0`, das erste Kartenziel bei `+4`. Der Parser initialisiert die Einträge und schreibt weitere Argumente unter anderem bei `+8` und `+0x10`. Der Zusammenführungscode vergleicht bei `0x1F6C97` nur die ID. Die Routine für den Set-Wechsel kopiert bei `0x20FA65` hauptsächlich `0x9C8` Bytes. Damit bleiben die Effekte aus dem vorherigen Setup im Zielpuffer stehen. [Bisherige Befunde zu Import und Set-Wechsel](../Documentation/Modding_Overview.md#warum-die-effekte-beim-set-wechsel-hinterherhinken)

Die gleiche Kapazität steckt im Parser (`0x0E6084`), Serializer (`0x0E58CB`), Aufbaukopierer (`0x1EA1D3` bis `0x1EA1E4`) und Effektverteiler (`0x11BC06`). Im Spielzustand füllt die Effektliste `G+0x1280` bis unmittelbar vor `G+0x1500`. Der Aufbau liest dort weitere Zustandsfelder, unter anderem bei `0x18D4D2`. Die Kapazität bleibt daher bei 32. Der Patch kann innerhalb dieser Grenze verständlich auf einen zu großen Import reagieren.

## Implementierung nach Freigabe

**Laufzeit und Messmodus.** Mit den lokal vorhandenen Visual Studio 2022 Build Tools eine x86-DLL und einen kleinen Launcher bauen. Der Launcher erhält Pfad zur Testkopie und PID des dort gestarteten Clients. Er prüft den Prozesspfad, PE-Maschine, EXE-Hash und die vollständigen Originalinstruktionen aller ausgewählten Hooks vor dem Aktivieren. Hook-Installation am Hauptmenü ohne laufende Partie. Bei fehlgeschlagener Prüfung bleibt die Sitzung im Originalzustand. Im Messmodus die zwei temporären Setup-Puffer, den gewählten Set-Typ, Effekt-IDs samt Argumenten und die Aufrufreihenfolge erfassen. Die Messung bestimmt Register, Stack und Rückkehrziele der Hüllen. Ins Log kommen nur Kingdom-IDs, Effektargumente, Build, Modus und Prüfergebnis. Nach dem Beenden des Testprozesses gilt wieder der Originalcode.

**Import.** An den Eingabewegen vor der nativen Syntaxanalyse den `-x`-Abschnitt auf vollständige Klammern, genau einen Abschnitt und höchstens 32 Einträge prüfen. Kommas in Klammern trennen Argumente, keine Effekte. Ein Fehler zeigt einen Hinweis und lässt das aktive Set stehen. Die Suchfeldroutine verwendet einen Puffer von `0x1000` Bytes. Den Schutz vor Abschneiden deshalb am ursprünglichen Text verifizieren. Für den Testlauncher außerdem die Sets des isolierten Profils auf überlange Effektlisten prüfen. Eine allgemeine Änderung des Parsers für beliebige fremde oder bereits gespeicherte Daten ist ein eigener Arbeitsschritt.

- Am Einstieg `0x1F6A90` die beiden Setup-Puffer lesen und die Zusammenführung vollständig vorbereiten, bevor der Originalpfad Karten hinzufügt. `0x1F6410` kann schon vor der späteren Effektschleife serialisieren. Ein Kapazitätskonflikt muss daher vor diesem Aufruf zurück an den jeweiligen UI-Abbruchpfad. Im zweiten Aufrufer folgt nach `0x1FE032` ein weiterer Schreibpfad. Der Abbruch muss auch diesen umgehen.
- Bestehende Einträge in Reihenfolge erhalten. Für jeden eingehenden Eintrag die Zahl identischer Vorkommen im Eingabetext bis zu dieser Position mit der Zahl im bisherigen Setup vergleichen. Nur zusätzliche Vorkommen in Eingabereihenfolge anhängen. Identität umfasst die Effekt-ID und alle semantischen Argumente. Damit bleibt ein wiederholter Import idempotent und explizite Wiederholungen in derselben Liste sind darstellbar. Bei insgesamt mehr als 32 Einträgen den Import vor jeder Änderung ablehnen.
- Die vorbereitete Liste erst am späteren Zusammenführungspunkt in den Zielpuffer schreiben. Für gemeinsam gespeicherte Ziele die Werte aus der Kartenauswahl einbeziehen. `twist_room_for_more` (`0x17D8`) hat einen Zielslot. `twist_typhoon` (`0x17E3`) hat einen eigenen gemeinsamen Bereich mit bis zu 20 Karten. Unterschiedliche Belegungen derselben gemeinsamen Felder als Konflikt melden. Einzelne gültige Belegungen zusammen mit dem Effekt übernehmen. Danach den Originalserializer verwenden.

**Gespeicherte Sets.** Bei Wahl eines regulären Profilsets den Effektblock der neu aufgelösten Konfiguration unmittelbar vor dem Serialisieren in den zusammengeführten Puffer übernehmen. Die statischen Pufferkandidaten am Aufruf `0x20FB27` sind `[EBP-0x19C0]` für das neue und `[EBP-0x3350]` für das zusammengesetzte Setup. Der Zweig ab `0x20FB10` setzt den Profilset-Typ auf 1. Im Messmodus diesen Zweig und die Pufferidentität mit zwei verschieden belegten Sets prüfen. Den vollständigen Block kopieren, damit ein Set ohne Effekte die vorherige Liste leert. Die bereits kopierten Kartenfelder und gemeinsamen Effektziele müssen zum selben neuen Set gehören. Profilsets und eingebaute Empfehlungen am originalen Auswahlzweig unterscheiden.

**Automatik und Online-Setup.** Nach erfolgreichem Test mit einem festen Set den frühesten geeigneten Zeitpunkt für die native Zufallsauswahl messen. Die Option verwendet die zehn Hauptstapel der tatsächlich aufgelösten Auswahl unter den geltenden Freischaltungen. Bei vorab nötiger Auflösung werden neben den zehn IDs auch Landschaften, Zusatzkarten, Zuordnungen und Setup-Optionen aus derselben Ziehung fest übernommen. Die Online-Filterung bei `0x1FFA03` muss anschließend dieselbe Auswahl behalten. Aus jeder eindeutigen Hauptstapel-ID höchstens einen nativen `start_double_pile`-Eintrag erzeugen. Ein bereits ausdrücklich verdoppelter oder verdreifachter Zielstapel bekommt keinen zusätzlichen Faktor. Ist für die ganze Liste kein Platz, die Option mit verständlicher Meldung ablehnen. Als Ergebnis geht ausschließlich der vom Spiel unterstützte Kingdom-Text in die native Nachricht.

## Abnahme

Für den ersten Lauf die Steam-Installation in ein separates Testverzeichnis kopieren und ein isoliertes Windows-Profil oder eine gleichwertige Testumgebung verwenden. Offline zuerst mit deaktiviertem Patch prüfen, dass Steam tatsächlich die Arbeitskopie startet und deren Spielstände im Testprofil liegen. Auf dem geladenen Modul Build, Hash, Originalbytes und die Vorher-/Nachher-Puffer messen. Die Spieltests folgen in dieser Reihenfolge.

| Test | Erwartetes Ergebnis |
| --- | --- |
| Import von Dorf und Markt mit `start_double_pile`, einzeln und gemeinsam | Beide starten mit 20 Karten. Wiederholtes Einfügen desselben Textes fügt keine weitere Kopie der Effekte hinzu. Ein normaler `-k`-Import behält sein bisheriges Verhalten. |
| Zehn Verdopplungen plus ein weiterer Effekt | Alle zehn Ziele stehen in `kingdomData`. Ein zusätzlicher Effekt wie `twist_shoppers` greift ebenfalls. Reihenfolge und Argumente bleiben nach Speichern und Neuladen erhalten. |
| Neues Set A mit Effekten, B ohne Effekte, C mit anderen Effekten | Direkt beim ersten Start aus A gelten seine Regeln. B beginnt ohne Extras aus A. C aktiviert seine eigenen Effekte. Je ein Set mit `twist_room_for_more` prüft, dass Ziel und Effekt gemeinsam wechseln. |
| Grenze und Konflikte | Genau 32 und der Übergang 31 → 32 funktionieren. 33 Eingabeeinträge oder eine Zusammenführung über 32 werden ohne Absturz und ohne Teilübernahme abgelehnt. Widersprüchliche gemeinsame Effektziele ergeben einen konkreten Hinweis. |
| Siegkarten, geteilte und gemischte Stapel | Gärten bei zwei und bei mindestens drei Spielern mit dem Vanilla-Aufbau vergleichen. Bei Katapult/Felsen, Burgen und Rittern Stückzahlen je Typ und sichtbare Reihenfolge prüfen. Zeigt die native Verdopplung einen abweichenden Schichtaufbau, die betroffene Kombination für die Automatik als nicht abgenommen kennzeichnen. |
| Privates Spiel mit festem Set | Host erstellt Set und lädt ein. Gast sieht im normalen Client dieselben zehn Stapel, Extras und Größen. Mehr als zehn Karten aus einem verdoppelten Zehnerstapel gemeinsam erwerben. Nach erneutem Beitritt bleibt der Vorrat konsistent. Stimmt die gesendete Liste, aber das angenommene Setup weicht ab, Serverfilterung als offene Grenze festhalten. |
| Automatik mit freigeschalteten Erweiterungen | Gleicher Seed und gleiche Eingaben liefern dieselbe native Ziehung mit und ohne Option. Andere Seeds ziehen auch aus weiteren verfügbaren Erweiterungen. Nur die gezogenen Hauptstapel erhalten automatisch einen Faktor. Vor Online-Freigabe muss die empfangene Auswahl der gesendeten vollständigen Ziehung entsprechen. |

Für die Abnahme einer neuen EXE-Version die relevanten Aufrufpfade erneut prüfen. Bei Build 3315 den ersten Lauf und die Online-Abnahme getrennt mit Modus, Vorbereitung und Ergebnis festhalten.

## Automatik beim lokalen Auflösen des Königreichs

| RVA | Dateioffset | Originalbytes | Verwendung |
| --- | --- | --- | --- |
| `0x0B1EC6` | `0x0B12C6` | `E8 A5 21 17 00` | Der Aufruf von `GameSpecific_Load` bei einem neuen lokalen Spiel markiert den konkreten Save für die Automatik. |
| `0x1E9F57` | `0x1E9357` | `E8 14 B4 EF FF` | Unmittelbar vor dem nativen Effektserializer werden die Effektliste in EDX und die ersten zehn DWORDs der nativen Ziehung bei `EBP-0x750` verwendet. |

Die Automatik erhält den Save nur für den synchronen Aufruf auf demselben Thread. Sie verlangt den aktiven lokalen Save und ein normales Setup ohne Szenario- oder Tages-/Wochenflags. Dominions Verbindungsstatus spielt für die Automatik keine Rolle. Die Ziehung erfolgt vorher bei `0x1E9E3E`. Der Client fügt den serialisierten Effekttext dem vollständigen `kingdomData` hinzu, prüft die Auswahl beim Wiederaufbau und speichert sie im nativen lokalen Spielstand. Der Hook verändert weder die Zufallsquelle noch die Stapelstruktur direkt. Spieltests prüfen noch Ausführung, Wiederaufnahme und Zusammensetzung der Spezialstapel.

## Privates Spiel mit Patch nur beim Host

**Statisch geprüft.** Dominion.exe, Spielbuild 3315, Steam-Build 17683350. Der SHA-256 entspricht dem oben genannten Modul. Alle Adressen in dieser Tabelle sind RVAs, außer den ausdrücklich genannten Struktur-Offsets.

| Abschnitt | Befund |
| --- | --- |
| Spielanlage durch den Host | `DomCreateClick` ruft bei `0x1FFA03` den Freischaltungsfilter `0x0DBEF0` auf. Der Filter parst `GameConfig_Net.kingdomData` bei Struktur-Offset `+0x30`, prüft bei `0x0DC280` bis `0x0DC315` bis zu 32 Effekte einzeln und schreibt bei `0x0DC393` den neu serialisierten Text zurück. Die Prüfung kann Effekte mit Kartenargumenten aufgrund der Freischaltungen entfernen. Der Schleifenvergleich prüft keine Gleichheit der Effekt-IDs. |
| Nachricht zur Spielanlage | `0x1FFAF5` sendet `CreateGameMessage`. Die Deskriptoren bei `0x4C5148`, `0x4C5294` und `0x4C6CE4` führen zu `GameConfig_Net.kingdomData` bei `0x4C6F28`. Das Feld ist ein nativer Netzwerkstring. Ein vorab festgelegtes Set kann seine `-x`-Liste im selben Feld wie seine Karten mitgeben. |
| Empfang und Spielstart | Der Client dekodiert `GameDescMessage` bei `0x0B7952`. Dessen Deskriptor bei `0x4C5810` enthält `kingdomData`. Bei `0x0B4744` bis `0x0B474D` übernimmt der Client den String aus Struktur-Offset `+0x30`. Über `0x0B488D` → `0x0B1BE9` landet er in `GameSave+0x3C`. Der Netzwerkstart setzt bei `0x0B0D19` den Spieltyp 2 und lädt diesen Save bei `0x0B0DF3` über `GameSpecific_Load` → `DomGameLoad`. |
| Ausführung beim Gast | Bei vorhandenem `kingdomData` springt `DomGameLoad` bei `0x1E9D73` zum Laden des fertigen Kingdoms. `0x1EA131` führt in den nativen Setup-Parser. Der Parser trägt ab `0x0E6084` erkannte Effekte bis zur Kapazität von 32 einzeln ein. `0x0DEDF2` bis `0x0DEE05` übernehmen den ganzen Effektblock aus dem geparsten `kingdomData`. `0x1EA1D3` bis `0x1EA1E4` kopieren ihn in den Spielzustand. Der Pfad umgeht die Zusammenführung aus dem Kartensuchfeld, die gleiche Effekt-IDs zusammenfallen lässt. |

Für ein fertig festgelegtes Set spricht der Clientbefund für einen Patch nur beim Host. Voraussetzung ist, dass der Dienst die vom Host übergebenen Effekte in die aufgelöste Spielbeschreibung übernimmt. Ein Gast mit Build 3315 kann dann dieselben nativen Regeln laden. Im folgenden Spieltest lieferte der Dienst beide Effekte im ursprünglichen `setupData` zurück, aber keinen `-x`-Abschnitt im aufgelösten `kingdomData`. Die Bedingung für einen Patch nur beim Host ist in dieser Partie damit nicht erfüllt. [Netzwerkmessung](TEST.md#ergebnis-der-netzwerkmessung-am-10-november-2026)

Die vorhandene Automatik hängt am Start einer lokalen Partie. Für die Online-Variante muss der Host die zehn konkreten Stapel vor dem Versand festlegen und ihre Verdopplungen mitgeben. Ob und wann die Online-Ziehung bei einem noch offenen Zufallsset feststeht, ist eine getrennte Frage.

## Messpunkte für den Verlust der Online-Effekte

Der Online-Starter aktiviert neben den bestehenden Importkorrekturen fünf beobachtende Messpunkte. Sie lesen nur die relevanten Setup-Felder und rufen danach unverändert die ursprünglichen Funktionen auf. Der Client sendet seine reguläre Spielanlage. Die Messpunkte gelten für die oben per SHA-256 identifizierte `Dominion.exe`, Spielbuild 3315.

| RVA | Dateioffset | Originalbytes | Messung |
| --- | --- | --- | --- |
| `0x1FFA03` | `0x1FEE03` | `E8 E8 C4 ED FF` | `kingdomData` im Host-Setup unmittelbar vor dem Filter, `ECX` mit Struktur-Offset `+0x30`. |
| `0x1FFAF5` | `0x1FEEF5` | `E8 26 BC 08 00` | `setupData` und `kingdomData` aus dem zweiten Stapelargument der nativen `CreateGameMessage`, nach dem Filter. |
| `0x0B7965` | `0x0B6D65` | `E8 86 CF FF FF` | Dieselben Felder aus der erfolgreich dekodierten `GameDescMessage` in `ECX`. |
| `0x0B0DF3` | `0x0B01F3` | `E8 78 32 17 00` | Die Setup-Felder des Netzwerk-Spielstands in `ECX` unmittelbar vor `GameSpecific_Load`. |
| `0x1EA206` | `0x1E9606` | `E8 65 EF 0B 00` | Die nach dem Parsen in den Netzwerk-Spielzustand kopierte Effektliste, `EDX+0x1280`. |

Die Textmessung protokolliert das Vorhandensein von `-x` und der beiden festen Testregeln für Dorf und Markt. Sie speichert keine vollständigen Netzwerkstrings, Kontodaten oder Spielcodes. Eine vorläufige Lobbybeschreibung kann andere Daten enthalten als die Beschreibung der gestarteten Partie. Für die Auswertung zählt daher die Beschreibung zum Testspiel zusammen mit `load network game` und `installed effects`.

## Online-Spielzüge und lokales Mehrspielerspiel

**Statisch geprüft für Dominion.exe 3315, Steam-Build 17683350.** Der SHA-256 entspricht dem oben dokumentierten Modul. Die Analyse betrifft den installierten Client und den protokollierten privaten Spielstart.

| Pfad | Befund |
| --- | --- |
| Beitreten, Starten, Revanche | `JoinGameMessage` bei RVA `0x4C4FBC` beschreibt Spiel-ID, Spielcode, Ausschlüsse und Smartplays. Der Startbefehl bei `0x1FFD3E` und `0x1FFD55` schreibt vier Nutzbytes mit der Spiel-ID. `RematchGameMessage` bei `0x4C420C` hat `replaySet`, aber keinen eigenen Kingdom- oder Effekttext. Ein zweiter vorgesehener Übergabepunkt für `-x` ist auf diesen Pfaden nicht erkennbar. |
| Laden des aufgelösten Kingdoms | `DomGameLoad` übergibt bei `0x1EA131` ursprünglichen und aufgelösten Setup-Text an `DomSetupConfigFromString`. Bei `0x0DEDF2` bis `0x0DEE05` übernimmt die Funktion den Effektblock aus dem aufgelösten `kingdomData`. Das erklärt, warum der im `setupData` erhaltene Effekttext im gemessenen Spiel wirkungslos bleibt. |
| Spielzüge und Abgleich | Der lokale Zweig bei `0x1E991F` bis `0x1E993B` schreibt in den lokalen Spielstand. Der Netzwerkzweig ruft bei `0x1E9958` `SubmitNetworkAction` auf. Dieser sendet bei `0x1E98BC` eine `GameMoveMessage` mit Spiel-ID, Position und Daten des Zugprotokolls. Der Client gleicht in `0x0B76E1` bis `0x0B777C` empfangene Spielerprotokolle mit der lokalen Kopie ab und fordert bei Abweichungen eine Aktualisierung an. Die Nachrichtendeskriptoren enthalten keine neu festgelegte Effektliste. Das Ausmaß der serverseitigen Zugprüfung lässt sich daraus nicht bestimmen. |
| Enthaltene Server-Routinen | `EnterServer` (`0x0AB5B0`) und `ExitServer` (`0x0AB620`) setzen beziehungsweise leeren einen globalen Zeiger. Alle vier direkten Aufruferpaare (`0x0AB740/0x0AB748`, `0x0AB7EA/0x0AB7F2`, `0x0ABA39/0x0ABA41`, `0x0ABF39/0x0ABF41`) rufen beide unmittelbar nacheinander auf. Die Referenzen auf den Zeiger zeigen keinen laufenden Dominion-Server. In der installierten Steam-Version gibt es keine separate Server-EXE. |

Damit ist für **Privates Spiel** kein unterstützter Clientpfad zur Übergabe der getesteten Effektkombination im finalen Setup belegt. Das erneute Einfügen beim Laden könnte die lokalen Vorräte zweier gepatchter Clients ändern. Ob ihr Spiel danach mit dem Zugprotokoll des Dienstes konsistent bliebe, ist offen. Die bisherige Messung erfasst die vom Dienst gelieferte Konfiguration und keine serverseitige Kaufentscheidung.

Der lokale Modus **Weitergeben** ist in den deutschen Ressourcen unter `dom_ui_create_game_pass_and_play` und im Client bei `0x0BE124` belegt. Er verwendet eine gemeinsame Spielinstanz. Dominion ist auf Steam mit **Remote Play Together** gelistet. Damit kann der Host eine lokale Partie über das Internet teilen, während der bestehende Patch in seiner Spielinstanz läuft. Die Kombination aus Arbeitskopie, Steam-Einladung und lokalem Weitergeben ist noch im Spiel zu prüfen. [Dominion auf Steam](https://store.steampowered.com/app/1131620/) · [Steam Remote Play Together](https://store.steampowered.com/remoteplay#together)

## Effektdialog für lokale Partien

**Implementiert und statisch geprüft für Dominion.exe 3315, Steam-Build 17683350.** Der SHA-256 entspricht dem oben dokumentierten Modul. Der x86-Build und Tests der Listenlogik sind erfolgreich. Die Laufzeitabnahme in Dominions Oberfläche steht aus.

| RVA | PE-Dateioffset | Originalbytes | Verwendung |
| --- | --- | --- | --- |
| `0x1FFF35` | `0x1FF335` | `E8 96 4A EE FF` | Die Königreich-Oberfläche ruft `0x0E49D0` auf und kopiert das Ergebnis. Die Hülle lässt den Aufruf zuerst laufen und öffnet den Dialog bei F8 mit der bereits aufgelösten Auswahl. |
| `0x0BBDB0` bis `0x0BBDCF` | – | – | Die lokale Königreich-Einrichtung liegt beim aktiven Profil plus `0x10`. Der Dialog prüft diese Identität und öffnet sich vor dem Partiestart. Dominions Verbindungsstatus ist dafür unerheblich. |
| `0x0DDBB0`, `0x0E0FF0`, `0x23D850` | – | – | Der Editor lässt den Effekttext nativ parsen, ersetzt die Liste im temporären Setup und prüft die Rückausgabe. Der Spielstring im lokalen Setup wird über die native Zuweisung übernommen. |
| `0x1F70B5`, `0x1F70C7`, `0x1F70CE` | – | – | Der native Wechsel zur eigenen Auswahl löst die Bindung an das zuvor ausgewählte Profilset. Der Editor verwendet dieselben Felder, damit die geänderte Liste erhalten bleibt. |

Der Katalog wird beim Build aus den 89 deutschen Tabelleneinträgen der Spielanleitung und dem Kartenkatalog erzeugt. Die Zielauswahl verwendet kanonische IDs mit deutschen Namen und filtert nach Zieltyp. Bei Platz für Mehr und Taifun werden die gemeinsamen Zielfelder mit der Effektliste ersetzt oder beim Entfernen geleert. Auf dem Host bleibt die Automatik beim lokalen Spielstart verfügbar. Die Dialogoption gilt pro Testsitzung, die ausdrücklich gewählte Effektliste lässt sich als Set speichern.

## Native Hintergrundtests für Effekte

Der Runner wurde mit der oben per SHA-256 identifizierten `Dominion.exe` geprüft. Alle Moduladressen sind RVAs. Die Profilisolierung setzt vor `ResumeThread` im suspendiert angelegten Kindprozess den leeren Save-Pfadpuffer bei `0x15E3A98`. `XGetSaveGameFolder` bei `0x23C090` (Dateioffset `0x23B490`) gibt diesen Puffer bei gesetztem Inhalt zurück. Der Runner verifiziert die Originalbytes und den Pfad im Prozess. Erst nach Erscheinen des Spiellogs im temporären Ordner startet der Test die lokale Partie.

`DomCreateClick` setzt bei `0x1FF987` die Startanforderung im Client bei Struktur-Offset `+0x5044` auf 1. Der Runner setzt denselben Wert im isolierten Client. `0x0B3515` ruft den regulären Dispatcher auf. Dessen Fall 1 bei `0x0B25A7` (Dateioffset `0x0B19A7`) springt über `0x0B24E6` zu `0x0B1E20` und lädt die lokale Partie. Die Messung wartet auf den ersten Zug und den lokalen Spielstand. Der Spielzustand `G` liegt bei RVA `0x780AD8`, siehe Übergabe an `DomGameLoad` bei `0x224228` und `0x22424B`.

Der Runner liest die Effektliste aus `G+0x1280`, den aktiven Spieler aus `G+0x19CC` und die Karten aus `G+0x1A48`. Die Kartenrecords sind `0x64` Bytes groß. Typ, Ort und Spieler stehen bei `+4`, `+8` und `+0x28`. Der Handort ist `0x3EA`. Für den Vorrat liest der Runner die verkettete Liste aus dem zugehörigen Stapel. Die erwarteten Beobachtungen stehen in den Testfällen. [Gemessene Fälle](TEST.md#automatisierte-lokale-effektprüfung-im-hintergrund)

`0x190822` bis `0x19084F` berechnet und aktualisiert die Zugressourcen bei `G+0x174F4 + Spielerindex × 0x5A30`. Münzen, Aktionen und Käufe stehen bei `+0`, `+4` und `+8`. Die Token-Anzahl steht bei `G+0x19A4`. Die Token-Records bei `G+0x152C8` haben eine Größe von `0x20` Bytes. Typ, Spieler, Seite und Menge stehen bei `+0`, `+4`, `+0x10` und `+0x14`. Referenzen sind `0x171E43` bis `0x171E6F` für die Felder und `0x16B2D6` bis `0x16B2E4` für das Drehen des Reisemarkers.

Kostenlose Ereigniskäufe per `stamp_got_it_already` lösen im ersten Zug Regeln für Käufe und erhaltene Karten aus. Jeder Vergleich behält den Auslöser in allen Stufen bei und verändert nur die Anzahl des gemessenen Effekts. Für spätere Testzüge ist der lokale Zweig bei `0x1E9900` ein Kandidat. Er übergibt bei `0x1E992E` die Spielaktion an `0x1E94E0`. Die Übergabe von legalen Entscheidungen für beide Spielerplätze wurde im Runner noch nicht umgesetzt.

Für Siegpunktmarker und Projektsteine liest der Runner zusätzlich den Stapelindex im Token-Feld `+8`. `0x16D5F8` bis `0x16D663` löst den Zielstapel auf und übergibt dessen Index beim Anlegen der Siegpunktmarker. `0x10DFB0` verschiebt einen Projektstein zum gewählten Projekt. Der Runner misst dort nur die zugeordneten Steine. `0x1133A0` bearbeitet den Kostbarkeiten-Stapel über dessen verkettete Kartenliste. Der Runner prüft dort sowohl die Stapelgröße als auch die Zahl verschiedener Karten.

Die Abfragen bei `0x176E90` und `0x1754F0` messen effektive Preise und Zusatztypen. Der Runner prüft die Einsprungbytes der verifizierten EXE und führt die Abfragen ausschließlich im selbst gestarteten Kindprozess aus. Die Preisabfrage setzt das Flag für den Pfad ohne kartenindividuelle Rückrufe und liest Münzen und Schulden aus dem gepackten Rückgabewert. Der Typentest reicht eine 64-Bit-Typmaske als zwei Wörter weiter. Beim Stapelvergleich liest der Runner Kartentyp und Nachfolger jeder Karte, wodurch sowohl einheitliche Belohnungen als auch das Mischen des geteilten Stapels beobachtbar sind.
