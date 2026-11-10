
"""Embed the installed Steam card catalog in the standalone Dominion generator."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parent
CATALOG = ROOT.parent / "Documentation" / "Card_Properties.json"
XPACK_HASH = "e3cf4242c2756f9120901c28f2dbdbe636b1d7235cbec9616f0c622d65d90047"



# Extra card ids which cannot be selected as ten separate kingdom piles.
RELATED = {
    "sauna": "avanto", "encampment": "plunder", "patrician": "emporium", "settlers": "bustling_village",
    "catapult": "rocks", "gladiator": "fortune", "knights": "", "castles": "",
    "augurs": "", "clashes": "", "forts": "", "odysseys": "", "townsfolk": "", "wizards": "",
    "page": "treasure_hunter warrior hero champion", "peasant": "soldier fugitive disciple teacher",
    "hermit": "madman", "urchin": "mercenary", "bandit_camp": "spoils", "marauder": "spoils", "pillage": "spoils",
    "tournament": "prizes", "joust": "rewards", "exorcist": "will_o_wisp imp ghost",
    "devils_workshop": "imp", "tormentor": "imp", "vampire": "bat", "leprechaun": "wish",
    "secret_cave": "magic_lamp wish", "cemetery": "haunted_mirror ghost", "fool": "lucky_coin lost_in_the_woods",
    "pixie": "goat", "pooka": "cursed_gold", "shepherd": "pasture", "tracker": "pouch",
    "necromancer": "zombie_apprentice zombie_mason zombie_spy", "swamp_hag": "curse",
    "flag_bearer": "flag", "treasurer": "key", "swashbuckler": "treasure_chest", "border_guard": "lantern horn",
}
PILE_TYPES = {
    "knights": "knight", "castles": "castle", "augurs": "augur", "clashes": "clash", "forts": "fort",
    "odysseys": "odyssey", "townsfolk": "townsfolk", "wizards": "wizard",
}
COST_MATERIAL = {
    "horse": r"\bhorses?\b", "coffers": r"\bcoffers?\b", "villagers": r"\bvillagers?\b",
    "vp": r"[+-]\{\d+\}", "exile": r"\bexile\b", "journey": r"\bjourney token\b",
    "debt": r"\bdebt\b|\[\d*D\]", "trade_route": r"trade route mat",
}
# Audited against the English game text. Jewelled Egg also supplies Loot when trashed.
LOOT_SOURCES = set(("cursed cutthroat foray invasion jewelled_egg looting peril pickaxe prosper "
                    "sack_of_loot search wealthy_village").split())


# Count grants from the card itself. Material references can also be costs or checks.
GAIN_FILTERS = {
    "coffers": r"\+\s*[1-9]\d* Coffers?\b",
    "villagers": r"\+\s*[1-9]\d* Villagers?\b",
    "favors": r"\+\s*[1-9]\d* Favors?\b",
    "sun": r"\+\s*[1-9]\d*\[S\]",
    "vp": r"\+\{[1-9]\d*\}",
    "horse": r"\bgains? (?:a|one|two|\d+|that many) Horses?\b",
}
MIXED_PILES = set(PILE_TYPES) | {"sauna", "encampment", "patrician", "settlers", "catapult", "gladiator"}


def enrich_cards(cards):
    by_id = {card["id"]: card for card in cards}
    for card in cards:
        card_id = card["id"]
        power = card["text"]["en"][1].replace("//", " ")
        related = RELATED.get(card_id, "").split()
        pile_type = PILE_TYPES.get(card_id)
        if pile_type:
            related += [other["id"] for other in cards if not other["kingdomPile"] and pile_type in other["types"]]
        # Linked extras include the specific cards named in the game text.
        related += [other["id"] for other in cards if other["id"] != card_id and not other["kingdomPile"]
                    and (set(other["types"]) & {"heirloom", "artifact", "state"})
                    and re.search(r"\b" + re.escape(other["text"]["en"][0]) + r"\b", power)]
        related = [key for key in dict.fromkeys(related) if key in by_id]
        for group in ("prizes", "rewards"):
            if group in related:
                related.extend(other["id"] for other in cards if group[:-1] in other["types"] and other["id"] != group)
        card["related"] = related
        card["lootSource"] = card_id in LOOT_SOURCES
        card["mechanics"] = [key for key, pattern in COST_MATERIAL.items() if re.search(pattern, power, re.I)]
        card["gainFilters"] = [key for key, pattern in GAIN_FILTERS.items() if re.search(pattern, power, re.I)]
    # A mixed pile can introduce material or trigger setup through any of its cards.
    for card in cards:
        if card["id"] in PILE_TYPES or (card["kingdomPile"] and any(by_id[key]["expansion"] == card["expansion"]
                and by_id[key]["cost"] and not ({"traveller", "prize", "reward", "heirloom", "spirit", "state"} & set(by_id[key]["types"]))
                for key in card["related"])):
            members = [by_id[key] for key in card["related"] if key in by_id]
            card["pileMechanics"] = list(dict.fromkeys(kind for member in members for kind in member["mechanics"]))
            card["lootSource"] |= any(member["lootSource"] for member in members)
            if card["id"] in MIXED_PILES:
                card["gainFilters"] = list(dict.fromkeys([
                    *card["gainFilters"], *(key for member in members for key in member["gainFilters"]),
                ]))


def read_index(package):
    magic, version, count = struct.unpack("<4sII", package.read(12))
    if magic != b"KCPX" or version != 7:
        raise ValueError("Erwartet wird Dominion.xpack im KCPX-Format 7.")
    entries = {}
    for _ in range(count):
        row = package.read(168)
        key = row[40:].split(b"\0", 1)[0].decode("utf-8")
        entries[key] = struct.unpack_from("<II", row, 12)
    return entries


def read_resource(package, entries, key):
    offset, size = entries[key]
    package.seek(offset)
    return package.read(size)


def read_strings(data):
    start, count = struct.unpack_from("<QQ", data, 8)
    result = {}
    for index in range(count):
        a, b, c, d = struct.unpack_from("<4Q", data, start + 40 * index)
        key = data[a:b].rstrip(b"\0").decode("utf-8")
        result[key] = data[c:d].rstrip(b"\0").decode("utf-8")
    return result


def load_catalog(xpack):
    properties = json.loads(CATALOG.read_text(encoding="utf-8"))
    if properties["gameBuild"] != 3315:
        raise ValueError("Der Kartenkatalog muss zu Spielbuild 3315 gehören.")
    with xpack.open("rb") as package:
        if hashlib.file_digest(package, "sha256").hexdigest() != XPACK_HASH:
            raise ValueError("Die Dominion.xpack stimmt nicht mit Spielbuild 3315 überein.")
        package.seek(0)
        entries = read_index(package)
        translations = {lang: read_strings(read_resource(package, entries, f"res/translation/{key}"))
                        for lang, key in (("de", "de"), ("en", "en-us"))}
    cards = []
    for card_id, data in properties["cards"].items():
        if not data or data.get("aliasFor"):
            continue
        localized = {}
        for lang, strings in translations.items():
            title = strings.get(f"dom_card_title_{card_id}")
            if not title:
                raise ValueError(f"Titel fehlt: {lang}/{card_id}")
            power = strings.get(f"dom_card_power_{card_id}", "")
            localized[lang] = [title, power]
        cards.append({"id": card_id, **data, "text": localized})
    assert len(cards) == 853 and sum(card["kingdomPile"] for card in cards) == 498
    enrich_cards(cards)
    expansions = {}
    for card in cards:
        expansion = card["expansion"]
        if expansion in expansions:
            continue
        label = translations["de"].get(f"dom_expansion_name_{expansion}")
        expansions[expansion] = label or ("Promos" if expansion == "promos" else expansion)
    types = {
        lang: {key.removeprefix("dom_card_type_"): value for key, value in strings.items()
               if key.startswith("dom_card_type_")}
        for lang, strings in translations.items()
    }
    by_id = {card["id"]: card for card in cards}
    for alias, info in properties["cards"].items():
        if info and info.get("aliasFor") in by_id:
            target = by_id[info["aliasFor"]]
            target.setdefault("alternateTitles", []).extend(
                strings.get(f"dom_card_title_{alias}", alias) for strings in translations.values()
            )
    english_expansions = {
        expansion: translations["en"].get(f"dom_expansion_name_{expansion}", name)
        for expansion, name in expansions.items()
    }
    expansion_info = {}
    for expansion in expansions:
        if expansion == "basic":
            continue
        expansion_info[expansion] = {}
        rule_key = re.compile(r"dom_rules_" + re.escape(expansion) + r"_(\d+)$")
        for lang, strings in translations.items():
            description = strings.get(f"dom_ui_store_desc_{expansion}") or strings[f"dom_taptip_expansion_{expansion}_body"]
            rules = sorted((int(match[1]), text) for key, text in strings.items()
                           if (match := rule_key.fullmatch(key)) and text)
            expansion_info[expansion][lang] = {"description": description, "rules": [text for _, text in rules]}
    return {"expansions": expansions, "expansionsEn": english_expansions, "expansionInfo": expansion_info,
            "types": types, "cards": cards}


def replace_script(source, marker, contents):
    pattern = re.compile(re.escape(marker) + r"[\s\S]*?(?=</script>)")
    updated, count = pattern.subn(lambda _: marker + "\n" + contents + "\n", source)
    if count != 1:
        raise ValueError(f"Generator-Skriptblock fehlt oder ist doppelt vorhanden: {marker}")
    return updated


def script_json(data):
    return json.dumps(data, ensure_ascii=False, separators=(",", ":")).replace("<", r"\u003c")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xpack", type=Path, nargs="?")
    parser.add_argument("--html", type=Path, default=ROOT / "Dominion-Generator.html")
    parser.add_argument("--image-pack", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if not args.xpack and not args.image_pack:
        parser.error("Dominion.xpack oder --image-pack angeben.")
    if args.image_pack and (not args.output or args.output.resolve() == args.html.resolve()):
        parser.error("Für die Bildversion --output mit einem eigenen Dateipfad angeben.")

    output = args.output or args.html
    if any(path and output.resolve() == path.resolve() for path in (args.xpack, args.image_pack)):
        parser.error("Die Ausgabedatei muss sich vom Spielpaket und vom Bildpaket unterscheiden.")

    source = args.html.read_text(encoding="utf-8")
    if args.xpack:
        catalog = load_catalog(args.xpack)
        source = replace_script(source, '<script type="application/json" id="catalog">', script_json(catalog))
        print(f"{len(catalog['cards'])} Kartendefinitionen eingebettet.")
    worker = (ROOT / "generator_solver.js").read_text(encoding="utf-8")
    worker_marker = '<script type="text/plain" id="generator-solver">'
    source = replace_script(source, worker_marker, worker)
    if args.image_pack:
        pack = json.loads(args.image_pack.read_text(encoding="utf-8-sig"))
        if (not isinstance(pack, dict) or pack.get("format") != "dominion-card-images"
                or pack.get("version") != 1 or not isinstance(pack.get("images"), dict)):
            raise ValueError("Das Bildpaket hat ein unbekanntes Format.")
        if 'id="embedded-image-pack"' in source:
            raise ValueError("Für die Bildversion die HTML-Datei ohne eingebettetes Bildpaket verwenden.")
        tag = '<script type="application/json" id="embedded-image-pack">'
        source = source.replace(worker_marker, tag + script_json(pack) + "</script>\n" + worker_marker, 1)
        if output.resolve().parent != ROOT.resolve():
            source = source.replace("extract_card_images.py", (ROOT / "extract_card_images.py").resolve().as_uri())
        print(f"{len(pack['images'])} Kartenillustrationen eingebettet.")

    output.write_bytes(source.replace("\n", "\r\n").encode("utf-8"))


if __name__ == "__main__":
    main()

