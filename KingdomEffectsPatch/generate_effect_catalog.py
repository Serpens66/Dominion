"""Build the editor's C++ catalog from the project's German game reference."""

import json
from pathlib import Path
import re
import sys


DOCS = Path(__file__).resolve().parents[1] / "Documentation"
ARGUMENTS = {
    "Karten-ID": "card",
    "Stapel-ID": "pile",
    "Ereignis-ID": "event",
    "Landmarken-ID": "landmark",
    "Weg-ID": "way",
    "Prophezeiungs-ID": "prophecy",
    "Projekt-ID": "project",
}


def plain_text(markdown):
    markdown = re.sub(r"!?\[([^\]]+)\]\([^)]*\)", r"\1", markdown)
    return markdown.replace("**", "").replace("`", "")


def literal(value, wide=False):
    return ("L" if wide else "") + json.dumps(value, ensure_ascii=False)


def read_effects():
    guide = (DOCS / "Steam_Spielanleitung.md").read_text(encoding="utf-8")
    catalog = guide.split("### Katalog der Kampagneneffekte", 1)[1]
    catalog = catalog.split("#### Weitere Varianten aus der Kampagnenerzeugung", 1)[0]
    pattern = (
        r"^\| ([^|]+) \| `((?:twist|stamp|start)_[a-z0-9_]+)(?:\(([^)]+)\))?`"
        r" \| ([^\n]*?) \*Einmal – [^\n]*? · Mehrfach – [^\n]*?\* \|$"
    )
    effects = []
    for name, command, args, description in re.findall(pattern, catalog, re.MULTILINE | re.IGNORECASE):
        first = second = "none"
        if command == "twist_typhoon":
            first = "pile_list"
        elif command == "twist_room_for_more":
            first = "kingdom_pile"
        elif command == "start_in_play":
            first = "duration"
        elif command == "start_double_heirloom":
            first = "heirloom_parent"
        elif args:
            kinds = [ARGUMENTS[part.strip()] for part in args.split(",")]
            first, second = (kinds + ["none"])[:2]
        effects.append((command, plain_text(name), plain_text(description), first, second))
    assert len(effects) == len({row[0] for row in effects}) == 89, "Expected 89 documented effects"
    return effects


def read_targets():
    cards = json.loads((DOCS / "Card_Properties.json").read_text(encoding="utf-8"))
    names = json.loads((DOCS / "Card_Titles_de.json").read_text(encoding="utf-8"))
    assert cards["gameBuild"] == names["gameBuild"] == 3315
    titles = {card_id: title for title, card_id in names["titleToId"].items()}
    basic_piles = {"copper", "silver", "gold", "estate", "duchy", "province", "curse",
                   "platinum", "colony", "potion", "ruins", "horse"}
    heirloom_parents = {"cemetery", "fool", "pixie", "pooka", "secret_cave", "shepherd", "tracker"}
    landscape_types = ("event", "landmark", "way", "prophecy", "project")
    targets = []
    for card_id, props in cards["cards"].items():
        if not props or "types" not in props:
            continue
        kinds = [kind for kind in landscape_types if kind in props["types"]]
        if {"action", "treasure", "victory", "night", "curse"}.intersection(props["types"]):
            kinds.append("card")
        if "duration" in props["types"]:
            kinds.append("duration")
        if card_id in heirloom_parents:
            kinds.append("heirloom_parent")
        if props["kingdomPile"]:
            kinds.append("kingdom_pile")
        if props["kingdomPile"] or card_id in basic_piles:
            kinds.append("pile")
        if kinds:
            targets.append((card_id, titles.get(card_id, card_id), kinds))
    return sorted(targets, key=lambda row: (row[1].casefold(), row[0]))


def main(output):
    rows = ["// Generated from Documentation by generate_effect_catalog.py.",
            "constexpr EffectDefinition effect_catalog[] = {"]
    for command, name, description, first, second in read_effects():
        rows.append(f"    {{{literal(command)}, {literal(name, True)}, {literal(description, True)}, "
                    f"TargetKind::{first}, TargetKind::{second}}},")
    rows.extend(("};", "", "constexpr TargetDefinition target_catalog[] = {"))
    for card_id, name, kinds in read_targets():
        mask = " | ".join(f"kind_mask(TargetKind::{kind})" for kind in kinds)
        rows.append(f"    {{{literal(card_id)}, {literal(name, True)}, {mask}}},")
    rows.append("};")
    Path(output).write_text("\n".join(rows) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main(sys.argv[1])
