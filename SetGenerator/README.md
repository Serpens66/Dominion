# Dominion-Generator

**[Dominion-Generator direkt im Browser öffnen](https://serpens66.github.io/Dominion/)**

## Königreich ziehen

1. Wähle bei Bedarf Erweiterungen und Ziehregeln aus. Karten kannst du festhalten oder ausschließen.
2. Klicke auf **Königreich ziehen** und dann auf **Für Dominion kopieren**.
3. Füge den Code in das Kartensuchfeld der Königreich-Auswahl in Dominion ein.

Der Kingdom-Code enthält auch die gezogenen Landschaften, Verbündeten, Prophezeiungen und Zusatzkarten-Zuordnungen sowie die konkrete Auswahl für Platin/Kolonien und Unterschlüpfe. Ausgeschaltete Optionen werden ausdrücklich mitgegeben. Automatisch benötigtes Material wie Pferde und Ruinen folgt den ausgewählten Karten.

Ein **Steam-Teilcode** nennt unter dem Ausgabefeld die fehlenden Aufbau-Auswahlen. Das betrifft das noch nicht unterstützte Schwarzmarktdeck, mehr als vier Landschaftseinträge einschließlich Verbündeten und Prophezeiungen sowie Ziele außerhalb der ersten zehn Hauptstapel. Der Hinweis auf ausstehende Steam-Importtests bleibt sichtbar; die Syntax ist statisch geprüft. [Exportbefund und Grenzen](../Documentation/Kingdom_Code_Extras.md)

Unter **Erweiterungen und Editionen** garantiert **Immer** mindestens einen Stapel aus dem Set. **Zufall** nimmt das Set in den Lostopf auf. Mit **Von** und **Bis** bestimmst du die Stapelzahl pro Set. Bei Zufall gelten diese Quoten, sobald das Set ausgelost wurde. Festgehaltene Karten machen ihre Herkunft erforderlich und belegen bei Zufallssets einen Lostopfplatz. Über **Einstellungen sichern oder laden** kannst du deine Auswahl in einen anderen Browser übertragen.

Unter **Festhalten** enthält jeder Erweiterungsreiter Hauptstapel und Landschaften in getrennt beschrifteten Gruppen. Die Suche blendet Gruppen ohne Treffer aus. Bei Promos stehen damit alle elf Hauptstapel und das Ereignis Einladung im selben Reiter.

Bei Erweiterungen auf **Zufall** kannst du unabhängige **Gewichte** von 0 bis 1000 in Schritten von 0,1 eingeben. **5 / 3 / 2** ergibt beispielsweise **50 % / 30 % / 20 %**. Andere Gewichtseingaben bleiben beim Bearbeiten unverändert. Berechnete Chance, schmales Gewichtsfeld und Auswahl stehen in dieser Reihenfolge in einer Zeile. Die Auswahl steht bündig direkt vor Von/Bis. Größere oder dezimale Werte bleiben im Feld bearbeitbar; der Tooltip zeigt den vollständigen Wert. **Lostopf gleich gewichten** setzt alle aktiven Gewichte auf 1. Gewichte bleiben beim Wechsel zu Immer/Aus erhalten und werden mit den Einstellungen gespeichert und geladen.

Mehrere Erweiterungen werden ohne Zurücklegen gezogen. Die Prozentanzeigen gelten für die erste Ziehung, nicht für die Wahrscheinlichkeit, im fertigen Königreich enthalten zu sein; ihre Summe darf durch Rundung von 100 % abweichen. Durch feste Karten erforderliche Erweiterungen sind weiterhin garantiert; für die freien Plätze werden die verbleibenden Chancen neu berechnet. Gewicht 0 schließt eine Erweiterung von der freiwilligen Auslosung aus. Unmögliche Kombinationen werden unter Beibehaltung der relativen Wahrscheinlichkeiten übersprungen. Frühere Prozentverteilungen werden beim Laden mit denselben Chancen übernommen. Dateien ohne Gewichte starten überall mit Gewicht 1.

Mit **Festgehaltene Karten haben Vorrang** dürfen feste Karten Quoten überschreiten und aus gesperrten Erweiterungen oder Editionen stammen. Sie erfüllen weiterhin Mindestquoten; die übrigen Plätze bleiben an die Regeln gebunden. Festhalten allein belegt dann keinen Zufallsset-Platz. Unter den Filtern steht, was noch ergänzt werden darf.

Mit **Zusatzkarten bei Eigenschaften berücksichtigen** zählen feste Kartenreihen und bekannte Zusatzstapel bei den Eigenschaftsquoten mit. Zufällige Aufbaukarten wie beim Weg der Maus bleiben ausgenommen. **Verbunden mit** in der Kartenvorschau zeigt die zugehörigen Karten.

**Bannliste kopieren** gibt die ausgeschlossenen Karten als Einträge für `profiles.xml` aus, in Abschnitt 2 unter dem Kingdom-Code. Ergänze sie im aktiven Profil in `localBans` für KI/Weitergeben oder in `bans` für Online/Privates Spiel. Dominion vorher schließen, die Datei sichern und vorhandene Zahlen nicht doppelt einfügen.

## Web-Namenslisten umwandeln

Öffne in **2. Für Dominion kopieren** den zugeklappten Bereich **Namensliste vom Randomizer umwandeln**. Füge kommagetrennte Kartennamen ein und klicke auf **Umwandeln**. **Code kopieren** liefert den Steam-Code, ohne deine Ziehung oder Einstellungen zu ändern. Die Sprache wird automatisch erkannt; bei mehrdeutigen Namen kannst du sie auswählen. [Dominion Randomizer](https://dominionrandomizer.com/)

Die Umwandlung unterstützt einen bis zehn Hauptstapel und bis zu vier nachfolgende Landschaften. Dominion ergänzt freie Hauptstapelplätze zufällig. Besondere Setup-Zuordnungen werden als noch nicht unterstützt gemeldet.

## Generator offline nutzen

Speichere die HTML-Datei und öffne sie lokal im Browser. Für die Kartenvorschau mit Bildern gehört der Ordner `card-images` daneben. Auf der Website werden die Bilder beim Öffnen einer Vorschau automatisch geladen. [HTML-Datei](Dominion-Generator.html) · [Bilddateien](card-images) · [Bilder aktualisieren](../Documentation/README.md#kartenbilder-erstellen)
