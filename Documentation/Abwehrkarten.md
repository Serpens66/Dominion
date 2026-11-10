# Angriffsschutz in Dominion auf Steam

Der Abgleich gilt für Spielbuild 3315. Der Kartenkatalog enthält 853 eigenständige Definitionen, darunter 498 Königreichstapel. Für 838 Definitionen gibt es in beiden Sprachen einen `dom_card_power_*`-Text. Die übrigen Einträge sind Grundkarten oder Hilfseinträge wie Gaben und Mischen. Die deutschen und englischen Texte wurden auf Angriffsschutz, Angriffsauslöser und Reaktionen auf typische Angriffsfolgen wie Nehmen, Ablegen, Aufdecken und Entsorgen geprüft. [Kartenkatalog](Card_Properties.json), [Quellenregister](Sources/Source_Register.md#karteneigenschaften-aus-l01-und-l05)

## Ausdrücklicher Angriffsschutz

Die folgenden Karten besitzen einen ausdrücklichen Schutzeffekt. Das Merkmal `defendsAgainstAttacks: true` im Kartenkatalog beruht auf den geprüften Kartentexten. Der native Typ **Reaktion** beschreibt, wie eine Karte ausgelöst werden kann.

| Karte | Schutz | Königreichstapel |
| --- | --- | --- |
| Burggraben (`moat`) | Vor dem gegnerischen Angriff aus der Hand aufdecken. Der Angriff betrifft dich nicht. | Ja |
| Leuchtturm (`lighthouse`) | Schützt nach dem Ausspielen bis zum Beginn des nächsten eigenen Zuges. | Ja |
| Wächterin (`guardian`) | Schützt nach dem Ausspielen bis zum Beginn des nächsten eigenen Zuges. | Ja |
| Champion (`champion`) | Schützt nach dem Ausspielen für den Rest der Partie. | Sonderkarte aus der Tauschreihe von Page |
| Schild (`shield`) | Vor dem gegnerischen Angriff aus der Hand aufdecken. Der Angriff betrifft dich nicht. | Beute aus dem Beutestapel |

Auch **Page** (`page`) zählt als Abwehrstapel. Seine feste Tauschreihe führt über Schatzsucher, Krieger und Held zu Champion. Die Von-bis-Regel zählt Königreichstapel. Champion und Schild sind Sonderkarten. Beutequellen können Schild zufällig liefern.

## Abgrenzung der übrigen Effekte

| Gruppe | Beispiele und geprüfte Wirkung |
| --- | --- |
| Angriffsreaktion | Bettler kann sich für zwei Silber ablegen, Karawanenwächter und Wachhund ziehen Karten, Diplomatin sortiert die Hand durch Ziehen und Ablegen, Pferdehändler legt sich bis zum nächsten Zug beiseite und Geheimkammer zieht und legt zwei Karten auf den Nachziehstapel. Der Angriff wird anschließend ausgeführt. Diese sechs Stapel tragen `attackReaction: true`. |
| Gezielte Minderung einzelner Folgen | Wachturm kann eine gerade genommene Karte entsorgen, Fahrende Händlerin sie gegen Silber tauschen. Beide müssen dafür auf der Hand sein. Festung kehrt zuverlässig auf die Hand zurück, wenn sie selbst entsorgt wird. Bei einem Angriff, der etwa die oberste Karte des Nachziehstapels trifft, hängt das davon ab, ob Festung getroffen wird. Diese Stapel zählen daher nicht als Abwehr. |
| Vorteile durch einzelne Angriffsfolgen | Getreuer Hund, Dorfanger, Pfad, Tunnel und Weberin können beim Ablegen außerhalb des Aufräumens reagieren. Marktplatz belohnt das Entsorgen einer eigenen Karte, Patron das Aufdecken. Hirtenhund reagiert auf das Nehmen einer Karte. Diese Effekte geben einen Vorteil, wenn die jeweilige Folge eintritt. |
| Vorbereitung oder Unterdrückung einzelner Aktionen | Hafen, Ausrüstung und Schweriner Dom legen im eigenen Zug Handkarten beiseite. Zauberin ersetzt den Effekt der ersten gegnerischen Aktion im jeweiligen Zug. Das kann auch eine Angriffskarte treffen. Zauberin bleibt als Angriffskarte eingestuft. |
| Sonstige Reaktionskarten | Alle übrigen Königreichstapel mit dem Spieltyp **Reaktion**. Dazu gehören zum Beispiel Wachturm, Fahrende Händlerin und Tunnel. Burggraben zählt als Abwehr, Bettler als Angriffsreaktion. |

Unter den 498 Königreichstapeln gibt es vier Abwehrstapel, sechs Angriffsreaktionen und 18 sonstige Reaktionsstapel. Jeder davon zählt für genau eine dieser drei Kategorien.

## Dominion Randomizer

KingdomCreator 7.0.7 benutzt für **Require Reaction** das Merkmal `isReaction`. Sobald ein Angriff im Königreich liegt, genügt der Regel ein beliebiger Reaktionsstapel. Wenn kein solcher Stapel verfügbar ist, entfernt der Generator Angriffe aus dem zulässigen Pool. Der veröffentlichte Kartenbestand enthält 37 Reaktionseinträge für 25 verschiedene Namen, wenn Editionen und Updatepacks zusammengefasst werden. Leuchtturm und Wächterin sind dort Dauer-Karten. Champion und Schild sind keine auswählbaren Königreichstapel. [Ziehregel](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/randomizer/randomizer.ts#L586-L610), [Kartentypen](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/src/dominion/card-type.ts), [Website-Kartenbestand](https://github.com/blakevanlan/KingdomCreator/blob/d9a802db2e41d52786eaa02097279cbf1b64c778/docs/dominion-content.js)

Als Reaktion geführt werden Bettler, Blinder Passagier, Buchhalterin, Burggraben, Diplomatin, Dorfanger, Fahrende Händlerin, Falknerin, Geheimkammer, Getreuer Hund, Hirtenhund, Karawanenwächter, Kartenzeichnerin, Katzengold, Marktplatz, Patron, Pfad, Pferdehändler, Piratin, Schlitten, Schwarze Katze, Tunnel, Wachhund, Wachturm und Weberin. Die Namen sind die deutschen Titel aus der Steam-Version.
