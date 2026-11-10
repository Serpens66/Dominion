"""Build the editor's C++ catalog from the project's German game reference."""

import base64
import hashlib
from io import BytesIO
import json
from pathlib import Path
import re
import sys
import unicodedata


DOCS = Path(__file__).resolve().parents[1] / "Documentation"
GENERATOR = DOCS.parent / "SetGenerator"
sys.path.insert(0, str(GENERATOR))
from generator_catalog import load_catalog

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
            targets.append((card_id, title, expansion, search_key("\n".join(terms)), types,
                            " + ".join(parts), rule_text(rules), kinds))
    return sorted(targets, key=lambda row: (search_key(row[2]), search_key(row[1]), row[0]))


def prepare_images(output, xpack, targets):
    source = GENERATOR / "Dominion-Kartenbilder.json"
    folder = output.parent / "card-previews"
    folder.mkdir(exist_ok=True)
    stamp = folder / "source.sha256"
    try:
        from PIL import Image
        source_data = source.read_bytes() if source.is_file() else None
        digest = hashlib.sha256(Path(__file__).read_bytes() + (GENERATOR / "extract_card_images.py").read_bytes())
        digest.update(source_data if source_data is not None else b"Dominion.xpack build 3315")
        digest.update("\n".join(sorted(targets)).encode("ascii"))
        version = digest.hexdigest()
        if stamp.is_file() and stamp.read_text(encoding="ascii") == version:
            return
        if source_data is not None:
            pack = json.loads(source_data)
        else:
            from extract_card_images import read_image_pack
            pack = read_image_pack(xpack, True)
        if pack["format"] != "dominion-card-images" or pack["version"] != 1:
            raise ValueError("Unbekanntes Bildpaketformat")
        count = 0
        for card_id, encoded in pack["images"].items():
            if card_id not in targets or not re.fullmatch(r"[a-z0-9_]+", card_id):
                continue
            try:
                with Image.open(BytesIO(base64.b64decode(encoded.split(",", 1)[1], validate=True))) as image:
                    image.thumbnail((320, 240), Image.Resampling.LANCZOS)
                    temporary = folder / (card_id + ".tmp")
                    image.save(temporary, "PNG")
                    temporary.replace(folder / (card_id + ".png"))
                count += 1
            except (OSError, ValueError, IndexError) as error:
                print(f"Kartenvorschau für {card_id}: {error}", file=sys.stderr)
        stamp.write_text(version, encoding="ascii")
        print(f"{count} lokale Kartenvorschauen vorbereitet.")
    except (ImportError, OSError, ValueError, KeyError, TypeError) as error:
        print(f"Kartenvorschauen verwenden Text, wenn Bilder fehlen: {error}", file=sys.stderr)


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
    for card_id, *metadata, kinds in targets:
        mask = " | ".join(f"kind_mask(TargetKind::{kind})" for kind in kinds)
        fields = ", ".join(literal(value, True) for value in metadata)
        rows.append(f"    {{{literal(card_id)}, {fields}, {mask}}},")
    rows.extend(("};", "", "constexpr std::pair<std::string_view, std::string_view> target_aliases[] = {"))
    for alias, canonical in aliases.items():
        rows.append(f"    {{{literal(alias)}, {literal(canonical)}}},")
    rows.append("};")
    output.write_text("\n".join(rows) + "\n", encoding="utf-8")
    prepare_images(output, xpack, {entry[0] for entry in targets})


if __name__ == "__main__":
    main(*sys.argv[1:])
