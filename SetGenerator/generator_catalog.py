
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
EXE_HASH = "56e8d563320b7d6c0cf68ae11cdf2c0aa971dbff44bb296651b630815c548877"


def read_native_ids(exe, card_ids):
    """Read Build 3315 definition IDs, including CRT-initialized definitions.

    The card-name pointer is at +0x58 and the saved numeric ID at +0x8C.
    Only reconstruct immediate writes into writable PE sections, never run the
    executable. The hash and complete, unique mapping are mandatory guards.
    """
    data = exe.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXE_HASH:
        raise ValueError("Die Dominion.exe stimmt nicht mit Spielbuild 3315 überein.")
    import pefile

    pe = pefile.PE(data=data, fast_load=True)
    image = bytearray(pe.get_memory_mapped_image())
    base = pe.OPTIONAL_HEADER.ImageBase
    writable = [(section.VirtualAddress, section.VirtualAddress + section.Misc_VirtualSize)
                for section in pe.sections if section.Characteristics & 0x80000000]
    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        for match in re.finditer(rb"\xc7\x05(.{8})", section.get_data(), re.S):
            address, value = struct.unpack("<II", match[1])
            rva = address - base
            if any(start <= rva and rva + 4 <= end for start, end in writable):
                struct.pack_into("<I", image, rva, value)
    pointers = {}
    for match in re.finditer(rb"[a-z][a-z0-9_]*\x00", image):
        if match.start() and image[match.start() - 1] != 0:
            continue
        name = match[0][:-1].decode("ascii")
        if name in card_ids:
            pointers[base + match.start()] = name
    candidates = {key: set() for key in card_ids}
    for start, end in writable:
        for index, (pointer,) in enumerate(struct.iter_unpack("<I", image[start:end - (end - start) % 4])):
            if pointer not in pointers:
                continue
            name_rva = start + index * 4
            if name_rva + 0x38 > len(image):
                continue
            native_id = struct.unpack_from("<I", image, name_rva + 0x34)[0]
            # Build 3315 card groups 1..19; group 20 is expansion selectors,
            # not cards (some selectors share a card's textual name).
            if 0x100 <= native_id < 0x1400:
                candidates[pointers[pointer]].add(native_id)
    invalid = [key for key, values in candidates.items() if len(values) != 1]
    if invalid:
        raise ValueError("Native Karten-IDs fehlen oder sind mehrdeutig: " + ", ".join(sorted(invalid)))
    result = {key: next(iter(values)) for key, values in candidates.items()}
    if len(set(result.values())) != len(result):
        raise ValueError("Die nativen Karten-IDs sind nicht eindeutig.")
    return result

VALUE_CARDS = {
    "copper": ("coin", "1"), "silver": ("coin", "2"), "gold": ("coin", "3"), "platinum": ("coin", "5"),
    "estate": ("vp", "1"), "duchy": ("vp", "3"), "province": ("vp", "6"), "colony": ("vp", "10"),
    "curse": ("vp", "−1"), "potion": ("potion", ""),
}

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

# Setup choices are deliberately separate from the fixed card family. Our Steam
# export does not yet fix these choices, so their properties cannot be promised.
RANDOM_SETUP = {
    "young_witch": "bane", "ferryman": "ferryman", "riverboat": "riverboat",
    "way_of_the_mouse": "mouse", "druid": "boons", "black_market": "black-market",
    "approaching_army": "attack-pile",
}
TRAVELLER_CHAINS = [
    "page treasure_hunter warrior hero champion".split(),
    "peasant soldier fugitive disciple teacher".split(),
]
# Audited supplemental effects. Native randomizer tags omit some non-Supply
# cards. Do not infer trashing from 'when you trash' or from an opponent's Hex.
SUPPLEMENTAL_FILTERS = {
    "ExtraActions": "courser demesne".split(),
    "ExtraCards": "avanto fugitive imp mercenary warrior zombie_apprentice horse figurehead stronghold warlord housecarl the_winds_gift battle_plan old_map garrison will_o_wisp".split(),
    "Trashing": "bat goat prize_goat spell_scroll small_castle zombie_apprentice zombie_mason the_flames_gift magic_lamp".split(),
}


def enrich_relations(cards):
    """One directed relation model for previews, property quotas and setup help."""
    by_id = {card["id"]: card for card in cards}
    titles = {card["text"]["en"][0].lower(): card["id"] for card in cards}
    named_gain = re.compile(r"\bgains? (?:an? |one |two |three |four |five |\d+ )?(" +
                            "|".join(re.escape(title) for title in sorted(titles, key=len, reverse=True)) +
                            r")(?:s)?\b", re.I)
    for tag, ids in SUPPLEMENTAL_FILTERS.items():
        for key in ids:
            if tag not in by_id[key]["nativeFilters"]:
                by_id[key]["nativeFilters"].append(tag)
    # Both Loot cards offer +1 Buy immediately, though an alternative is allowed.
    for key in ("orb", "amphora"):
        by_id[key]["immediateBuys"] = 1
    for card in cards:
        key = card["id"]
        power = card["text"]["en"][1].replace("//", " ").replace("`", "")
        groups = []

        def group(kind, ids):
            ids = list(dict.fromkeys(value for value in ids if value in by_id and value != key
                                     and value not in {"prizes", "rewards", "boons", "hexes", "loot_pile"}))
            if ids:
                groups.append({"kind": kind, "ids": ids})
        for chain in TRAVELLER_CHAINS:
            if key in chain:
                group("traveller", chain[chain.index(key) + 1:])
        group("cards", card["related"])
        # Named gains are fixed links; generic 'gain an Action' is not one.
        named = [titles[match[1].lower()] for match in named_gain.finditer(power)]
        group("cards", named)
        kinds = []
        if (("fate" in card["types"] and key != "druid") or
                re.search(r"\b(?:receive|receives) (?:a|the next) Boon\b", power)):
            kinds.append("boon")
        if "doom" in card["types"]:
            kinds.append("hex")
        if "looter" in card["types"]:
            kinds.append("ruins")
        if card["lootSource"]:
            kinds.append("loot")
        if "horse" in card["gainFilters"]:
            group("cards", ["horse"])
        for kind in kinds:
            group(kind, [other["id"] for other in cards if kind in other["types"]])
        if key in {"delusion", "envy", "misery"}:
            group("cards", {"delusion": ["deluded"], "envy": ["envious"],
                            "misery": ["miserable", "twice_miserable"]}[key])
        # Present direct links once, in order; transitive expansion happens below.
        seen = set()
        for item in groups:
            unique = [value for value in item["ids"] if value not in seen]
            seen.update(unique)
            item["ids"] = unique
        card["relatedGroups"] = [item for item in groups if item["ids"]]
        card["related"] = list(dict.fromkeys(value for item in groups for value in item["ids"]))
        card["randomSetup"] = RANDOM_SETUP.get(key) or (
            "ally" if "liaison" in card["types"] else "prophecy" if "omen" in card["types"] else None)
    for card in cards:
        visited = {card["id"]}
        pending = list(card["related"])
        family = []
        while pending:
            key = pending.pop(0)
            if key in visited:
                continue
            visited.add(key)
            family.append(key)
            pending.extend(by_id[key]["related"])
        card["propertyRelated"] = family


def enrich_cards(cards):
    by_id = {card["id"]: card for card in cards}
    for card in cards:
        card_id = card["id"]
        if card_id in VALUE_CARDS:
            card["printedValue"] = VALUE_CARDS[card_id]
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


def load_catalog(xpack, exe=None):
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
    native_ids = read_native_ids(exe or xpack.with_name("Dominion.exe"), {
        key for key, info in properties["cards"].items() if info and not info.get("aliasFor")
    })
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
        cards.append({"id": card_id, **data, "nativeId": native_ids[card_id], "text": localized})
    assert len(cards) == 853 and sum(card["kingdomPile"] for card in cards) == 498
    enrich_cards(cards)
    enrich_relations(cards)
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
    return {"nativeIds": {"gameBuild": 3315, "steamBuild": 17683350,
                          "rulesVersion": properties["rulesVersion"], "exeSha256": EXE_HASH},
            "expansions": expansions, "expansionsEn": english_expansions, "expansionInfo": expansion_info,
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
    parser.add_argument("xpack", type=Path)
    parser.add_argument("--exe", type=Path, help="Dominion.exe; standardmäßig neben Dominion.xpack")
    parser.add_argument("--html", type=Path, default=ROOT / "Dominion-Generator.html")
    args = parser.parse_args()
    exe = args.exe or args.xpack.with_name("Dominion.exe")
    if args.html.resolve() in {args.xpack.resolve(), exe.resolve()}:
        parser.error("HTML-Datei und Spieldateien müssen unterschiedliche Dateien sein.")

    catalog = load_catalog(args.xpack, exe)
    source = args.html.read_text(encoding="utf-8")
    source = replace_script(source, '<script type="application/json" id="catalog">', script_json(catalog))
    worker = (ROOT / "generator_solver.js").read_text(encoding="utf-8")
    source = replace_script(source, '<script type="text/plain" id="generator-solver">', worker)
    args.html.write_bytes(source.replace("\n", "\r\n").encode("utf-8"))
    print(f"{len(catalog['cards'])} Kartendefinitionen eingebettet.")


if __name__ == "__main__":
    main()

