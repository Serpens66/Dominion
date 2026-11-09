"""Convert the text copied by dominionrandomizer.com into a TGG Kingdom string."""

import argparse
from collections import defaultdict
from pathlib import Path
import json
import sys
import unicodedata


DOCUMENTATION = Path(__file__).resolve().parents[1] / "Documentation"
KINGDOM_SIZE = 10
LANDSCAPE_TYPES = {"event", "landmark", "project", "way"}
SETUP_DEPENDENCIES = {
    "young_witch": "Bane-Stapel",
    "ferryman": "Zusatzstapel",
    "riverboat": "beiseitegelegte Aktionskarte",
    "way_of_the_mouse": "beiseitegelegte Aktionskarte",
    "obelisk": "zugeordneter Aktionsstapel",
    "druid": "zugeordnete Gaben",
    "approaching_army": "zusätzlicher Angriffsstapel",
}


def normalize_title(title):
    title = unicodedata.normalize("NFC", title).replace("’", "'")
    return " ".join(title.casefold().split())


def resolve_titles(titles, catalog, language):
    if language != "auto" and language not in catalog["languages"]:
        raise ValueError(f"Unbekannte Sprache: {language}")
    languages = catalog["languages"] if language == "auto" else [language]
    keys = [
        (index < KINGDOM_SIZE, normalize_title(title))
        for index, title in enumerate(titles)
    ]
    attempts = []
    for lang in languages:
        lookup = defaultdict(set)
        for row in catalog["cards"].values():
            if row["kind"] == "other" or row["id"] is None:
                continue
            name = row["titles"].get(lang, row["titles"]["en"])
            key = (row["kind"] == "kingdom", normalize_title(name))
            lookup[key].add(row["id"])
        attempts.append((lang, [lookup[key] for key in keys]))

    complete = [matches for _, matches in attempts if all(matches)]
    if not complete:
        lang, closest = max(attempts, key=lambda attempt: sum(map(bool, attempt[1])))
        unknown = ", ".join(repr(title) for title, matches in zip(titles, closest) if not matches)
        raise ValueError(f"Unbekannte Titel oder falsche Position für Sprache {lang}: {unknown}")

    result = []
    for title, matches in zip(titles, zip(*complete)):
        ids = set.union(*matches)
        if len(ids) > 1:
            options = ", ".join(sorted(ids))
            raise ValueError(f"Mehrdeutiger Titel {title!r}: {options}. Sprache oder Kartentitel prüfen.")
        result.append(ids.pop())
    return result


def convert(text, catalog, properties, *, language="auto", colonies=False, shelters=False):
    titles = [title.strip() for title in text.strip().split(",")]
    if len(titles) < KINGDOM_SIZE or not all(titles):
        raise ValueError("Erwartet werden zehn kommagetrennte Hauptstapel, danach optionale Zusatzkarten.")
    if catalog["gameBuild"] != properties["gameBuild"]:
        raise ValueError("Web-Zuordnung und lokaler Kartenkatalog gehören zu verschiedenen Spielbuilds.")

    ids = resolve_titles(titles, catalog, language)
    if len(set(ids)) != len(ids):
        raise ValueError("Die Namensliste enthält dieselbe Karte oder denselben Stapel mehrfach.")

    piles, extras = ids[:KINGDOM_SIZE], ids[KINGDOM_SIZE:]
    cards = properties["cards"]
    if not all((cards.get(card_id) or {}).get("kingdomPile") for card_id in piles):
        raise ValueError("Die ersten zehn Einträge müssen auswählbare Königreichstapel sein.")
    dependencies = [
        f"{card_id} ({SETUP_DEPENDENCIES[card_id]})"
        for card_id in ids if card_id in SETUP_DEPENDENCIES
    ]
    unsupported = [
        card_id for card_id in extras if not LANDSCAPE_TYPES.intersection(cards[card_id]["types"])
    ]
    if dependencies or unsupported:
        names = ", ".join(dependencies + unsupported)
        raise ValueError(f"Zusatzsetup für diese Karten wird noch nicht exportiert: {names}")
    if len(extras) > 4:
        raise ValueError("Für dieses Format sind höchstens vier Landschaften vorgesehen.")

    tokens = ids.copy()
    if colonies:
        tokens.append("colonies")
    if shelters:
        tokens.append("shelters")
    kingdom = "-k " + ", ".join(tokens)
    if extras:
        kingdom += f", -m Landscapes(0,{len(extras)})"

    first_edition = [card_id for card_id in piles if cards[card_id]["edition"] == "first"]
    return kingdom, first_edition


def main():
    catalog = json.loads((DOCUMENTATION / "KingdomCreator_Card_Names.json").read_text(encoding="utf-8"))
    properties = json.loads((DOCUMENTATION / "Card_Properties.json").read_text(encoding="utf-8"))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("text", nargs="?", help="Kopierte Namensliste. Ohne Argument von stdin lesen.")
    parser.add_argument("--lang", choices=["auto", *catalog["languages"]], default="auto")
    parser.add_argument("--colonies", action="store_true", help="Platin und Kolonien ausdrücklich aktivieren.")
    parser.add_argument("--shelters", action="store_true", help="Unterschlüpfe ausdrücklich aktivieren.")
    args = parser.parse_args()
    try:
        text = args.text
        if text is None:
            if sys.stdin.isatty():
                print("Namensliste einfügen und Enter drücken:", file=sys.stderr)
                text = input()
            else:
                text = sys.stdin.read()
        kingdom, first_edition = convert(
            text, catalog, properties, language=args.lang, colonies=args.colonies, shelters=args.shelters,
        )
    except (EOFError, ValueError) as error:
        parser.error(str(error) or "Keine Namensliste erhalten.")
    if first_edition:
        print(f"Erste Edition im Spiel prüfen: {', '.join(first_edition)}", file=sys.stderr)
    print(kingdom)


if __name__ == "__main__":
    main()
