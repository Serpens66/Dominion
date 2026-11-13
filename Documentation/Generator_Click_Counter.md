# Klickzähler der Generator-Website

Stand: 13. November 2026. Webprüfung; kein Bezug zum Spielbuild erforderlich.

## Verhalten

Die Website zeigt rechts neben dem Kartendaten-Hinweis einen kleinen, zweisprachigen Gesamtstand. Der aufklappbare Hinweis **Zum Zähler / About the counter** steht rechts daneben und nennt nur das Startdatum 13. November 2026 sowie die Übermittlung von IP-Adresse und Browserinformationen an Visitor Badge. Auf schmalen Bildschirmen darf die Fußzeile umbrechen. Gezählt werden Klicks auf **Königreich ziehen**, einschließlich wiederholter und erfolgloser Ziehversuche, keine unterschiedlichen Personen. Lokale Kopien bleiben ohne Anfragen. Ausfälle lassen die Ziehung unbeeinträchtigt; die letzte erfolgreiche Anzeige bleibt stehen oder die Zeile verborgen.

## Geprüfter Dienst

- Visitor Badge unter `https://visitor-badge.one9x.com/`; Kennung `serpens66.github.io.Dominion.draw-clicks`.
- **Live geprüft:** `/count` liefert JSON, aber keinen `Access-Control-Allow-Origin`-Header. Deshalb wird der Stand als read-only SVG von `/badge` mit `read=true` angezeigt. Zählen erfolgt separat per `fetch` ohne Cookies und ohne Referrer; Lesen des SVG sendet ebenfalls keinen Referrer.
- **Quelltext geprüft:** `unique` muss fehlen. Die Anwendung verwendet eine boolesche Umwandlung, die auch die Zeichenfolge `false` als wahr interpretiert. Wiedererkennungs-Cookies setzt der geprüfte Anwendungscode nur bei aktiviertem Unique-Modus.
- **Live geprüft:** Das read-only Zahlenbild antwortete mit HTTP 200 ohne `Set-Cookie`. Bildanfragen können dennoch vorhandene Cookies mitsenden. Vorgeschaltete Infrastruktur und spätere Dienständerungen sind damit nicht abschließend erfasst.
- Der externe Dienst erhält IP-Adresse und Browserinformationen. Die Seite erklärt das unter **Zum Zähler / About the counter**; damit ist keine abschließende rechtliche Bewertung oder Zusage zur Einwilligungsfreiheit verbunden.

## Prüfung

- Neue automatisierte Prüfung: `node SetGenerator/test_kingdom_counter.js`, auch im Pages-Workflow. Prüft Herkunftsbeschränkung, read-only Start, wiederholte Klicks, Sprachwechsel, überholte Bildantworten, Fehlertoleranz und fehlende Wiederholungsversuche.
- **Im Browser getestet:** Edge 157.0.4341.46 und Firefox 160.0, jeweils 390 und 1280 Pixel Breite, Deutsch und Englisch. Linksbündig, kein horizontaler Überlauf; Schrift 12,8 Pixel, Zahlenbild rund 14,7 Pixel hoch. Layoutprüfung mit lokalem Testdienst und Stand 12345.
- **Live im Browser getestet:** Beide Browser konnten die Zählanfragen ohne Cookies als opaque Responses abschließen. Insgesamt vier Anfragen an die separate Kennung `serpens66.github.io.Dominion.integration-test-20261113` ergaben danach den Stand 4. Der produktive Zähler wurde nicht erhöht.
- Bestehende Solver-, Export-, Namensumwandlungs-, Gewichtungs-/Losverfahren- und Katalogprüfungen bestanden.

Quellen: [Dienst und Parameter](https://visitor-badge.one9x.com/), [Anwendungsquelltext](https://github.com/ramank775/visitor-badge/blob/master/main.py), [§ 25 TDDDG](https://www.gesetze-im-internet.de/ttdsg/__25.html).
