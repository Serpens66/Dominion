# Dominion Kingdom Effects Patch v0.1.0

[English](#english) · [Deutsch](#deutsch)

## English

An unofficial mod for **Dominion on Windows/Steam, game build 3315**. Choose from 89 campaign effects, combine up to 32 entries, and customize your Kingdom in a bilingual F8 editor. A separately installed copy of Dominion is required; this mod does not unlock DLC.

### Start playing

1. Close Dominion and extract the entire ZIP into a folder.
2. Double-click **Dominion-mit-Mod-starten.cmd**. The launcher finds your Steam installation; if asked, select the Dominion game folder.
3. Wait for **Bereit** (ready). In local play or pass-and-play, open the Kingdom/card selection and press **F8**.
4. Use **Language: Deutsch / English** at the bottom to choose the editor language. The label **Language:** stays English in both languages.

Steam and your internet connection can stay on. No Python or Visual Studio is needed. The launcher uses your normal Steam installation and normal profile, with no separate game copy or test profile.

### Example: double Village and Market

1. Choose a Kingdom containing **Village** and **Market** before opening F8.
2. In F8, search for `start_double_pile` and select **Double Supply pile**.
3. Choose **Choose targets…**, check **Village** and **Market**, and confirm with **Done**. Confirm the F8 editor with **Done**, then start the game.

Both piles start with **20 cards instead of 10**. To intentionally apply the same effect again, enable **Add another copy** in the target picker; two doubling effects on Village produce **40 cards**.

Search by English or German name, effect ID, or description. Filters include effect type and when an effect triggers. The table shows what each effect does, its prerequisites and its test status. **Untested** effects remain selectable; repeated use may have a different test status from a single application.

Card selection supports multiple targets, card previews, and expansion descriptions followed directly by scrollable rules. Reorder active effects or Typhoon targets by dragging, or use **Alt+Up/Down**; **Delete** removes the selected entry while its list has focus. **Cancel** discards your effect changes.

### Online play and bans

Campaign-effect gameplay is supported for **local play and pass-and-play**. F8 also opens in online/private Kingdom selection, but the server currently does not apply these effects. The automatic doubling of randomly drawn piles is local-only.

Online bans use Dominion's existing card collection: right-click a card and choose its **Ban → Online** option. The mod removes the five-ban restriction. Your online list can hold up to **700 cards and landscapes combined**. Larger lists have been tested in private games; matchmaking and the full capacity remain unverified.

The mod also fixes **Clear** changing a private game into a public online game. Private game → Clear has been confirmed by a user test; the full sequence including F8 changes and inviting players still awaits separate confirmation.

### Saves, troubleshooting and removal

The launcher automatically backs up a changed profile before starting, keeping the latest **three backups** in `%LOCALAPPDATA%\DominionKingdomEffectsPatch\profile-backups`. Your progress, sets and bans use your normal profile and are not reset when you quit.

If startup fails, close Dominion and check `%LOCALAPPDATA%\DominionKingdomEffectsPatch\letzter-start.log`. After quitting, `letzter-patch.log` in the same folder may provide more detail. An unsupported Dominion update requires an updated mod.

Some effects and combinations are still untested. The game's campaign-features popup may overflow with many effects; close it or select **Don't show again**. Occasional missing illustrations do not prevent text or card selection. Visual checks at 125%/150% Windows scaling remain open.

To play without the mod, close the game and start it normally through Steam. To remove it, delete the extracted mod folder. Saved sets and bans remain in your profile. Game files are not modified; the download contains no game artwork.

[Effect guide and test status](https://github.com/Serpens66/Dominion/blob/main/Documentation/Steam_Spielanleitung.md) · [Known limitations](https://github.com/Serpens66/Dominion/blob/main/Documentation/Known_Issues.md) · [Technical documentation](https://github.com/Serpens66/Dominion/blob/main/Documentation/README.md)

## Deutsch

Ein inoffizieller Mod für **Dominion unter Windows/Steam, Spielbuild 3315**. Im zweisprachigen F8-Editor stehen 89 Kampagneneffekte zur Auswahl; bis zu 32 Einträge lassen sich kombinieren. Eine eigene Dominion-Installation ist erforderlich. Der Mod schaltet keine DLCs frei.

### Starten

1. Dominion schließen und das ZIP vollständig in einen Ordner entpacken.
2. **Dominion-mit-Mod-starten.cmd** doppelklicken. Der Starter findet die Steam-Installation; falls nötig, den Dominion-Spielordner auswählen.
3. Auf **Bereit** warten. In **Lokal spielen** oder **Weitergeben** die Königreich-/Kartenauswahl öffnen und **F8** drücken.
4. Unten mit **Language: Deutsch / English** die Menüsprache wählen. Die Beschriftung **Language:** bleibt in beiden Sprachen englisch.

Steam und Internet können eingeschaltet bleiben. Python und Visual Studio werden nicht benötigt. Verwendet werden die normale Steam-Installation und dein normales Profil, ohne Spielkopie oder Testprofil.

### Beispiel: Dorf und Markt verdoppeln

1. Ein Königreich mit **Dorf** und **Markt** festlegen und F8 öffnen.
2. Nach `start_double_pile` suchen und **Doppelter Vorratsstapel** wählen.
3. **Ziele wählen** öffnen, **Dorf** und **Markt** anhaken und mit **Fertig** übernehmen. Auch den F8-Editor mit **Fertig** bestätigen und das Spiel starten.

Beide Stapel beginnen mit **20 statt 10 Karten**. Für eine bewusste Wiederholung **Weitere Kopie hinzufügen** in der Zielauswahl aktivieren: Zwei Verdopplungen ergeben beim Dorf **40 Karten**.

Die Suche findet deutsche und englische Namen, Effekt-IDs und Beschreibungstexte. Filter berücksichtigen Effektart und Auslösezeitpunkt. Die Tabelle zeigt Wirkung, Voraussetzungen und Teststand. **Ungetestete** Effekte bleiben auswählbar; einfache und mehrfache Anwendung können unterschiedlich weit geprüft sein.

Die Kartenauswahl bietet mehrere Ziele, Kartenvorschauen sowie Erweiterungsbeschreibungen mit direkt anschließenden scrollbaren Regeln. Aktive Effekte und Taifun-Ziele lassen sich durch Ziehen oder **Alt+Pfeil hoch/runter** sortieren. **Entf** entfernt den gewählten Eintrag bei Fokus in seiner Liste. **Abbrechen** verwirft die Effektänderungen.

### Online-Spiel und Banns

Die Spielwirkung der Kampagneneffekte ist für **Lokal spielen und Weitergeben** vorgesehen. F8 ist auch bei der Online-/Privatspiel-Auswahl verfügbar; der Server übernimmt diese Effekte aktuell jedoch nicht. Das automatische Verdoppeln zufällig gezogener Stapel bleibt auf lokale Partien beschränkt.

Online-Banns werden in der vorhandenen Sammlung bearbeitet: Karte mit Rechtsklick öffnen und **Verbieten → Online** wählen. Der Mod hebt die Fünferbegrenzung auf. Die eigene Online-Liste fasst insgesamt bis zu **700 Karten und Querkarten zusammen**. Größere Listen wurden in privaten Spielen getestet; Matchmaking und die volle Kapazität sind noch ungeprüft.

Außerdem bleibt ein **Privates Spiel** beim Anklicken von **Leer** privat. Dieser Ablauf wurde vom Nutzer bestätigt; die gesamte Folge einschließlich F8-Änderung und anschließendem Einladen bleibt separat zu prüfen.

### Profil, Probleme und Entfernen

Vor dem Start sichert der Starter ein verändertes Profil automatisch. Die neuesten **drei Sicherungen** liegen unter `%LOCALAPPDATA%\DominionKingdomEffectsPatch\profile-backups`. Fortschritt, Sets und Banns verwenden dein normales Profil und werden nach Spielende nicht zurückgesetzt.

Bei Startproblemen Dominion schließen und `%LOCALAPPDATA%\DominionKingdomEffectsPatch\letzter-start.log` prüfen. Nach Spielende kann `letzter-patch.log` im selben Ordner weitere Hinweise enthalten. Nach einem nicht unterstützten Dominion-Update ist eine angepasste Modversion nötig.

Einige Effekte und Kombinationen sind noch ungetestet. Der Hinweis **Kampagnen-Features** kann bei vielen Effekten überlaufen; schließen oder **Nicht wieder anzeigen** wählen. Vereinzelt fehlende Illustrationen verhindern weder Kartentext noch Auswahl. Sichtbare Prüfungen bei 125/150 % Windows-Skalierung stehen noch aus.

Für eine Partie ohne Mod Dominion schließen und normal über Steam starten. Zum Entfernen den entpackten Modordner löschen. Gespeicherte Sets und Banns bleiben im Profil. Die Spieldateien werden nicht verändert; der Download enthält keine Spielgrafiken.

[Spielanleitung und Teststand](https://github.com/Serpens66/Dominion/blob/main/Documentation/Steam_Spielanleitung.md) · [Bekannte Grenzen](https://github.com/Serpens66/Dominion/blob/main/Documentation/Known_Issues.md) · [Technische Dokumentation](https://github.com/Serpens66/Dominion/blob/main/Documentation/README.md)
