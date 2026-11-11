"""Build the editor's C++ catalog from the project's German game reference."""

import base64
import hashlib
from io import BytesIO
from functools import lru_cache
import json
from pathlib import Path
import re
import sys
import unicodedata


DOCS = Path(__file__).resolve().parents[1] / "Documentation"
GENERATOR = DOCS.parent / "SetGenerator"
sys.path.insert(0, str(GENERATOR))
from generator_catalog import load_catalog, read_index, read_resource, read_strings, XPACK_HASH

ARGUMENTS = {
    "Karten-ID": "card",
    "Stapel-ID": "pile",
    "Ereignis-ID": "event",
    "Landmarken-ID": "landmark",
    "Weg-ID": "way",
    "Prophezeiungs-ID": "prophecy",
    "Projekt-ID": "project",
}
TYPE_ICONS = {
    "action": "actions", "attack": "attack", "reaction": "reaction", "duration": "duration",
    "night": "night", "treasure": "treasure", "victory": "vp", "liaison": "favor_rules", "omen": "omen",
    **dict.fromkeys(("event", "landmark", "project", "way", "prophecy"), "landscape"),
}
MENU_ASSETS = {
    "wood": "wood_bar", "panel": "panel_frame_bg_tile", "gold": "button_golden",
    "check": "ico_check_btn", "checkbox": "checkbox", "seal": "seal_campaign", "plus": "ico_plus2gold",
}
MENU_LABELS = {
    "kingdom": "dom_ui_create_game_tab_kingdom", "choose_card": "dom_ui_create_game_cards_input",
    "expansions": "dom_ui_create_game_expansions", "done": "dom_ui_score_done",
    "cancel": "dom_ui_create_game_dismiss", "features": "dom_ui_campaign_features",
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
        r" \| ([^\n]*?) \*Einmal – ([^\n]*?) · Mehrfach – ([^\n]*?)\* \|$"
    )
    effects = []
    for name, command, args, description, once, repeated in re.findall(pattern, catalog, re.MULTILINE | re.IGNORECASE):
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
        status = f"Einmal – {once}\r\nMehrfach – {repeated}"
        effects.append((command, plain_text(name), plain_text(description), status, first, second))
    assert len(effects) == len({row[0] for row in effects}) == 89, "Expected 89 documented effects"
    return effects


def search_key(text):
    return "".join(ch for ch in unicodedata.normalize("NFD", text.casefold()) if not unicodedata.combining(ch))


def rule_text(text):
    text = re.sub(r"-//(?=[a-zäöüß])", "", text)
    text = text.replace("//", "\n").replace("{br}", "\n").replace("{coin_2}", "[2]")
    text = re.sub(r"([\[{])!", r"\1", text)
    text = re.sub(r"\[(\d*)([DPS]?)\]", lambda m: " ".join(filter(None, (
        m[1], {"": "Münzen", "D": "Schulden", "P": "Trank", "S": "Sonnenmarker"}[m[2]]))), text)
    text = re.sub(r"\{([+-]?\d*)\}", lambda m: (m[1] + " ").lstrip() + "Siegpunkte", text)
    for marker, label in {"embargo_rules": "Embargomarker", "coin_token_rules": "Geldmarker",
                          "favor_rules": "Gefallen-Marker"}.items():
        text = text.replace("{" + marker + "}", label)
    text = text.replace("|", "").replace("`", "")
    text = re.sub(r"%([^%]+)%", r"\1", text)
    return "\r\n".join("".join(ch for ch in line if ch >= " " or ch == "\t") for line in text.splitlines())


def read_targets(catalog, aliases):
    basic_piles = {"copper", "silver", "gold", "estate", "duchy", "province", "curse",
                   "platinum", "colony", "potion", "ruins", "horse"}
    heirloom_parents = {"cemetery", "fool", "pixie", "pooka", "secret_cave", "shepherd", "tracker"}
    landscape_types = ("event", "landmark", "way", "prophecy", "project")
    targets = []
    for props in catalog["cards"]:
        card_id = props["id"]
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
            title, rules = props["text"]["de"]
            terms = [title, props["text"]["en"][0], card_id, *props.get("alternateTitles", []),
                     *(alias for alias, target in aliases.items() if target == card_id)]
            types = " · ".join(catalog["types"]["de"].get(kind, kind) for kind in props["types"])
            cost = props.get("cost")
            parts = []
            if cost:
                if cost["coins"] or not (cost["potions"] or cost["debt"]):
                    parts.append(f"{cost['coins']} Münzen")
                if cost["potions"]:
                    parts.append(f"{cost['potions']} Trank")
                if cost["debt"]:
                    parts.append(f"{cost['debt']} Schulden")
            expansion = "Grundkarten" if props["expansion"] == "basic" else catalog["expansions"][props["expansion"]]
            icons = ",".join(dict.fromkeys(TYPE_ICONS[kind] for kind in props["types"] if kind in TYPE_ICONS))
            costs = (cost["coins"], cost["potions"], cost["debt"]) if cost else (-1, 0, 0)
            targets.append((card_id, title, expansion, search_key("\n".join(terms)), types,
                            " + ".join(parts), rule_text(rules), props["expansion"], icons, costs, kinds))
    return sorted(targets, key=lambda row: (search_key(row[2]), search_key(row[1]), row[0]))


def prepare_images(output, xpack, cards, types):
    source = GENERATOR / "Dominion-Kartenbilder.json"
    folder = output.parent / "card-faces"
    folder.mkdir(exist_ok=True)
    stamp = folder / "source.sha256"
    try:
        from PIL import Image
        from extract_card_images import decode_artwork, decode_texture, read_image_pack, render_catalog_card

        source_data = source.read_bytes() if source.is_file() else None
        digest = hashlib.sha256(Path(__file__).read_bytes() + (GENERATOR / "extract_card_images.py").read_bytes())
        digest.update(XPACK_HASH.encode("ascii"))
        if source_data is not None:
            digest.update(source_data)
        digest.update(json.dumps((cards, types), sort_keys=True).encode("utf-8"))
        version = digest.hexdigest()
        if stamp.is_file() and stamp.read_text(encoding="ascii") == version and all(
                (folder / f"{card['id']}.png").is_file() for card in cards):
            return
        pack = json.loads(source_data) if source_data is not None else read_image_pack(xpack, True)
        if pack["format"] != "dominion-card-images" or pack["version"] != 1:
            raise ValueError("Unbekanntes Bildpaketformat")

        def decode(encoded):
            with Image.open(BytesIO(base64.b64decode(encoded.split(",", 1)[1], validate=True))) as image:
                return image.convert("RGBA")

        full = pack.get("cards", {}).get("de", {})
        with xpack.open("rb") as package:
            entries = read_index(package)

            def read(key):
                return read_resource(package, entries, key)

            @lru_cache(maxsize=None)
            def atlas(key):
                return decode_texture(read(key))

            @lru_cache(maxsize=None)
            def sprite(name):
                key = f"res/card_gen/shared/{name}"
                return decode_artwork(read, atlas, key) if key in entries else None

            @lru_cache(maxsize=None)
            def expansion_icon(expansion):
                path = output.parent / "menu-art" / f"exp-{expansion}.png"
                if not path.is_file():
                    return None
                with Image.open(path) as image:
                    return image.convert("RGBA")

            count = missing_art = 0
            for card in cards:
                card_id = card["id"]
                if not re.fullmatch(r"[a-z0-9_]+", card_id):
                    raise ValueError(f"Ungültige Karten-ID {card_id}")
                path = folder / f"{card_id}.png"
                try:
                    if card_id in full:
                        face = decode(full[card_id])
                    else:
                        encoded = pack["images"].get(card_id)
                        art = decode(encoded) if encoded else None
                        if art is None and card["expansion"] == "basic":
                            art = sprite(card_id)
                        face = render_catalog_card(card, types, art, sprite, expansion_icon(card["expansion"]))
                        missing_art += int(art is None)
                    temporary = path.with_suffix(".tmp")
                    face.save(temporary, "PNG", compress_level=1)
                    temporary.replace(path)
                    count += 1
                except (OSError, ValueError, IndexError, KeyError) as error:
                    path.unlink(missing_ok=True)
                    print(f"Kartenvorschau für {card_id}: {error}", file=sys.stderr)
        if count == len(cards):
            stamp.write_text(version, encoding="ascii")
        print(f"{count} lokale Kartenvorderseiten vorbereitet, davon {missing_art} ohne Illustration.")
    except (ImportError, OSError, ValueError, KeyError, TypeError) as error:
        print(f"Kartenvorschauen verwenden Text, wenn Bilder fehlen: {error}", file=sys.stderr)


def prepare_menu(output, xpack):
    folder = output.parent / "menu-art"
    folder.mkdir(exist_ok=True)
    stamp = folder / "source.sha256"
    try:
        source = (GENERATOR / "Dominion-Generator.html").read_text(encoding="utf-8")
        match = re.search(r'<script type="application/json" id="game-icons">\s*(.*?)</script>', source, re.S)
        icon_json = match[1] if match else "{}"
        icons = json.loads(icon_json)
    except (OSError, ValueError) as error:
        print(f"Menü verwendet Text für fehlende SetGenerator-Symbole: {error}", file=sys.stderr)
        icon_json, icons = "{}", {}
    with xpack.open("rb") as package:
        entries = read_index(package)

        def read(key):
            return read_resource(package, entries, key)

        de = read_strings(read("res/translation/de"))
        lines = ["#pragma once", "namespace kingdom::menu_text {"]
        lines.extend(f"inline constexpr wchar_t {name}[] = {literal(de[key], True)};"
                     for name, key in MENU_LABELS.items())
        lines.append("}")
        (output.parent / "menu_labels.generated.hpp").write_text("\n".join(lines) + "\n", encoding="utf-8")

        version = hashlib.sha256(Path(__file__).read_bytes() + (GENERATOR / "extract_card_images.py").read_bytes()
                                 + (XPACK_HASH + icon_json).encode("utf-8")).hexdigest()
        if stamp.is_file() and stamp.read_text(encoding="ascii") == version:
            return
        try:
            from PIL import Image
            from extract_card_images import decode_artwork, decode_texture
        except ImportError as error:
            print(f"Menü verwendet die Ersatzfarben: {error}", file=sys.stderr)
            return

        @lru_cache(maxsize=None)
        def atlas(key):
            return decode_texture(read(key))

        complete = True
        for name, key in MENU_ASSETS.items():
            try:
                decode_artwork(read, atlas, f"res/ui_art/{key}").save(folder / f"{name}.png")
            except (OSError, ValueError, KeyError) as error:
                print(f"Menügrafik {name}: {error}", file=sys.stderr)
                complete = False
        for name, data in icons.items():
            if not re.fullmatch(r"[a-z0-9_-]+", name) or name in MENU_ASSETS:
                continue
            try:
                with Image.open(BytesIO(base64.b64decode(data.split(",", 1)[1], validate=True))) as icon:
                    icon.save(folder / f"{name}.png")
            except (OSError, ValueError, IndexError) as error:
                print(f"Menüsymbol {name}: {error}", file=sys.stderr)
                complete = False
        if complete:
            stamp.write_text(version, encoding="ascii")


def main(output, xpack):
    output, xpack = Path(output), Path(xpack)
    catalog = load_catalog(xpack)
    properties = json.loads((DOCS / "Card_Properties.json").read_text(encoding="utf-8"))
    aliases = {key: data["aliasFor"] for key, data in properties["cards"].items() if data and "aliasFor" in data}
    targets = read_targets(catalog, aliases)
    rows = ["// Generated from Documentation by generate_effect_catalog.py.",
            "constexpr EffectDefinition effect_catalog[] = {"]
    for command, name, description, status, first, second in read_effects():
        rows.append(f"    {{{literal(command)}, {literal(name, True)}, {literal(description, True)}, {literal(status, True)}, "
                    f"TargetKind::{first}, TargetKind::{second}}},")
    rows.extend(("};", "", "constexpr CardTarget target_catalog[] = {"))
    for card_id, *metadata, expansion_id, icons, costs, kinds in targets:
        mask = " | ".join(f"kind_mask(TargetKind::{kind})" for kind in kinds)
        fields = ", ".join(literal(value, True) for value in metadata)
        price = ", ".join(map(str, costs))
        rows.append(f"    {{{literal(card_id)}, {fields}, {literal(expansion_id)}, {literal(icons)}, {price}, {mask}}},")
    rows.extend(("};", "", "constexpr std::pair<std::string_view, std::string_view> target_aliases[] = {"))
    for alias, canonical in aliases.items():
        rows.append(f"    {{{literal(alias)}, {literal(canonical)}}},")
    rows.append("};")
    output.write_text("\n".join(rows) + "\n", encoding="utf-8")
    prepare_menu(output, xpack)
    target_ids = {entry[0] for entry in targets}
    prepare_images(output, xpack, [card for card in catalog["cards"] if card["id"] in target_ids],
                   catalog["types"]["de"])


if __name__ == "__main__":
    main(*sys.argv[1:])
