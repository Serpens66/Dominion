"""Build a local image pack from Dominion.xpack."""

import argparse
import base64
from functools import lru_cache
import io
import json
from pathlib import Path
import re
import struct
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    raise SystemExit("Pillow fehlt. Installiere es mit: python -m pip install Pillow") from None


BASE_COSTS = {
    "artisan": 6, "bandit": 5, "bureaucrat": 4, "cellar": 2, "chapel": 2,
    "council_room": 5, "festival": 5, "gardens": 4, "harbinger": 3, "laboratory": 5,
    "library": 5, "market": 5, "merchant": 3, "militia": 4, "mine": 5,
    "moat": 2, "moneylender": 4, "poacher": 4, "remodel": 4, "sentry": 5,
    "smithy": 4, "throne_room": 4, "vassal": 3, "village": 3, "witch": 5, "workshop": 3,
}
CARD_TYPES = dict.fromkeys(("bandit", "bureaucrat", "militia", "witch"), "attack") | {
    "moat": "reaction",
    "gardens": "vp",
}
TYPE_LABELS = {
    "de": {"action": "Aktion", "attack": "Aktion – Angriff", "reaction": "Aktion – Reaktion", "vp": "Punkte"},
    "en": {"action": "Action", "attack": "Action – Attack", "reaction": "Action – Reaction", "vp": "Victory"},
}
ASSET_NAMES = {"throne_room": "throne room"}
EXPANSION_ASSET_NAMES = {
    "acting_troupe": "acting_troup", "changeling": "changling", "sextant": "sextatnt",
    "smugglers": "smuggler", "witchs_hut": "witchhut", "wheelwright": "wheelright",
    "philosophers_stone": "philosopherstone", "summon": "summon-1",
    "flag": "artifact_flag", "key": "artifact_key", "horn": "artifact_horn",
    "lantern": "artifact_lantern", "treasure_chest": "artifact_chest",
}
INDEX_ENTRY_SIZE = 168
IMAGE_POINTER_OFFSET = 128
CARD_SIZE = (344, 524)
RULE_TOKEN = re.compile(r"\||\x60|\[(\d+)\]|\{(\d+)\}|\s+|[^\s|\x60\[\]{}]+|.")


def decode_texture(resource: bytes) -> Image.Image:
    offset = struct.unpack_from("<Q", resource, IMAGE_POINTER_OFFSET)[0]
    if offset < IMAGE_POINTER_OFFSET + 8 or resource[offset:offset + 4] != b"qoif":
        raise ValueError("Die Textur enthält keinen unterstützten QOI-Datenstrom.")
    with Image.open(io.BytesIO(resource[offset:]), formats=["QOI"]) as image:
        # Dominion stores the red and blue channels in BGR order in these textures.
        blue, green, red, alpha = image.convert("RGBA").split()
    return Image.merge("RGBA", (red, green, blue, alpha))


def read_translations(resource: bytes) -> dict[str, str]:
    start, count = struct.unpack_from("<QQ", resource, 8)
    translations = {}
    for i in range(count):
        key_start, key_end, value_start, value_end = struct.unpack_from("<4Q", resource, start + i * 40)
        key = resource[key_start:key_end].rstrip(b"\0").decode("utf-8")
        if key.startswith(("dom_card_title_", "dom_card_power_")):
            translations[key] = resource[value_start:value_end].rstrip(b"\0").decode("utf-8")
    return translations


@lru_cache(maxsize=None)
def card_font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    windows = "timesbd.ttf" if bold else "times.ttf"
    linux = "DejaVuSerif-Bold.ttf" if bold else "DejaVuSerif.ttf"
    for name in (windows, linux):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default(size=size)


def symbol_diameter(symbol: str, size: int) -> int:
    return round(size * 1.32) if symbol == "coin" else size


def rules_layout(text: str, size: int, max_width: int) -> list:
    """Lay out the game's // line breaks, bold spans and numbered symbols."""
    rows = []
    for line in text.split("//"):
        if not line.strip() or line.strip() == "---":
            rows.append((0, [], size // 2, bool(line.strip())))
            continue
        bold = False
        width = 0
        fragments = []
        space = 0
        row_height = size + 3
        for word in line.split():
            run = []
            word_width = 0
            for match in RULE_TOKEN.finditer(word):
                value = match[0]
                if value == "|":
                    bold = not bold
                    continue
                if value == chr(96):
                    continue
                symbol = "coin" if match[1] else "vp" if match[2] else ""
                value = (match[1] or match[2]) if symbol else value
                font = card_font(size, bold)
                advance = symbol_diameter(symbol, size) + 2 if symbol else font.getlength(value)
                run.append((word_width, value, font, symbol))
                word_width += advance
            if fragments and width + space + word_width > max_width:
                rows.append((width, fragments, row_height, False))
                width, fragments, space = 0, [], 0
                row_height = size + 3
            width += space
            fragments.extend((width + x, value, font, symbol) for x, value, font, symbol in run)
            row_height = max([row_height] + [symbol_diameter(symbol, size) + 3 for _, _, _, symbol in run if symbol])
            width += word_width
            space = card_font(size, bold).getlength(" ")
        rows.append((width, fragments, row_height, False))
    return rows


def render_card(art: Image.Image, sprites: dict, title: str, text: str,
                cost: int, card_type: str, language: str) -> Image.Image:
    face = Image.new("RGBA", CARD_SIZE)
    draw = ImageDraw.Draw(face)
    draw.rounded_rectangle((1, 1, 342, 522), radius=15, fill="#171512")

    def place(sprite: Image.Image, x: int, y: int, width: int, height: int) -> None:
        face.alpha_composite(sprite.resize((width, height), Image.Resampling.LANCZOS), (x, y))

    frame = card_type if card_type in ("vp", "reaction") else "action"
    place(art, 19, 47, 306, 229)
    place(sprites["txt_back"], 19, 274, 306, 206)
    place(sprites["text_back_lip"], 19, 272, 306, 9)
    for x in (13, 320):
        place(sprites["univ_pole"], x, 15, 10, 486)
        place(sprites["univ_topclip"], x - 3, 2, 18, 12)
        place(sprites["univ_baseclip"], x - 3, 504, 18, 7)
    place(sprites[f"frame_{frame}_ribl"], 8, 48, 22, 184)
    place(sprites[f"frame_{frame}_ribr"], 313, 48, 24, 184)
    place(sprites[f"frame_{frame}_top"], 5, 7, 334, 57)
    place(sprites[f"frame_{frame}_bot"], 5, 456, 334, 59)
    place(sprites["exp_base"], 296, 481, 29, 29)

    title = title.upper()
    size = 35
    while card_font(size).getlength(title) > 303 and size > 14:
        size -= 1
    draw.text((172, 34), title, font=card_font(size), fill="#16120e", anchor="mm")
    draw.text((172, 490), TYPE_LABELS[language][card_type].upper(), font=card_font(23), fill="#16120e", anchor="mm")

    def draw_value(symbol: str, value: str, x: int, y: int, diameter: int) -> None:
        place(sprites[symbol], x, y, diameter, diameter)
        ink = "#16120e" if symbol == "coin" else "#fffdf3"
        draw.text((x + diameter / 2, y + diameter / 2), value, font=card_font(int(diameter * .81), True),
                  fill=ink, anchor="mm", stroke_width=int(symbol == "vp"), stroke_fill="#201825")

    for size in range(25, 11, -1):
        rows = rules_layout(text, size, 284)
        height = sum(row[2] for row in rows)
        if height <= 172 and max(row[0] for row in rows) <= 284:
            break
    else:
        raise ValueError(f"Der Regeltext von {title} passt nicht auf die Karte.")
    y = (281 + 453 - height) / 2
    for width, fragments, row_height, separator in rows:
        center = y + row_height / 2
        if separator:
            draw.line((114, center, 230, center), fill="#453c35")
        for offset, value, font, symbol in fragments:
            x = (344 - width) / 2 + offset
            if symbol:
                diameter = symbol_diameter(symbol, size)
                draw_value(symbol, value, round(x), round(center - diameter / 2), diameter)
            else:
                draw.text((x, center), value, font=font, fill="#151311", anchor="lm")
        y += row_height
    draw_value("coin", str(cost), 1, 447, 70)
    return face


def image_url(image: Image.Image, format_name: str, **options) -> str:
    output = io.BytesIO()
    image.save(output, format=format_name, **options)
    return f"data:image/{format_name.lower()};base64," + base64.b64encode(output.getvalue()).decode("ascii")


def read_image_pack(path: Path, expansion_images: bool = False) -> dict:
    package_size = path.stat().st_size
    with path.open("rb") as package:
        magic, version, count = struct.unpack("<4sII", package.read(12))
        index_end = 12 + count * INDEX_ENTRY_SIZE
        if magic != b"KCPX" or version != 7 or index_end > package_size:
            raise ValueError("Das Paket hat kein unterstütztes KCPX-Format (Version 7).")

        entries = {}
        for _ in range(count):
            row = package.read(INDEX_ENTRY_SIZE)
            resource = row[40:].split(b"\0", 1)[0].decode("utf-8")
            entries[resource] = struct.unpack_from("<II", row, 12)

        def read(name: str) -> bytes:
            offset, size = entries[name]
            if offset < index_end or offset + size > package_size:
                raise ValueError(f"Ungültiger Ressourceneintrag für {name}.")
            package.seek(offset)
            return package.read(size)

        @lru_cache(maxsize=None)
        def atlas(name: str) -> Image.Image:
            return decode_texture(read(name))

        def sprite(name: str) -> Image.Image:
            resource = read(f"res/card_gen/shared/{name}")
            width, height = struct.unpack_from("<II", resource, 0x28)
            x, y = struct.unpack_from("<II", resource, 0x5C)
            name = resource[0x78:].split(b"\0", 1)[0].decode("utf-8").removesuffix(".texture")
            texture = atlas(name)
            if not (0 < width <= texture.width - x and 0 < height <= texture.height - y):
                raise ValueError(f"Ungültiger Ausschnitt in der Textur {name}.")
            return texture.crop((x, y, x + width, y + height))

        sprites = {
            name: sprite(name)
            for name in ("txt_back", "text_back_lip", "univ_pole", "univ_topclip", "univ_baseclip",
                         "exp_base", "vp", "ico_coin_smaller")
        }
        sprites["coin"] = sprites.pop("ico_coin_smaller")
        expansion = Image.new("RGBA", sprites["exp_base"].size, "#1f1b14")
        expansion.putalpha(sprites["exp_base"].getchannel("A"))
        sprites["exp_base"] = expansion
        for frame in ("action", "reaction", "vp"):
            for part in ("top", "bot", "ribl", "ribr"):
                name = f"frame_{frame}_{part}"
                sprites[name] = sprite(name)

        translations = {key: read_translations(read(f"res/translation/{language}"))
                        for key, language in (("de", "de"), ("en", "en-us"))}
        illustrations, complete_cards = {}, {"de": {}, "en": {}}
        for card_id, cost in BASE_COSTS.items():
            name = ASSET_NAMES.get(card_id, card_id)
            art = decode_texture(read(f"res/card_gen/base/{name}"))
            if art.size != (292, 216):
                raise ValueError(f"Unerwartete Bildgröße für {card_id}.")
            illustrations[card_id] = image_url(art, "PNG")
            for language, strings in translations.items():
                face = render_card(art, sprites, strings[f"dom_card_title_{card_id}"],
                                   strings[f"dom_card_power_{card_id}"], cost,
                                   CARD_TYPES.get(card_id, "action"), language)
                complete_cards[language][card_id] = image_url(face, "WEBP", quality=94)
        if expansion_images:
            add_expansion_images(path, entries, read, illustrations)
        return {"format": "dominion-card-images", "version": 1,
                "images": illustrations, "cards": complete_cards}


def add_expansion_images(path, entries, read, illustrations):
    from generator_catalog import load_catalog

    resources = {}
    for key in entries:
        parts = key.split("/")
        if len(parts) == 4 and parts[:2] == ["res", "card_gen"]:
            expansion, name = parts[2:]
            canonical = re.sub(r"^[a-z]+2e_", "", name)
            if expansion == "promo":
                canonical = canonical.removesuffix("art")
            resources.setdefault((expansion, normalize_image_name(canonical)), key)

    for card in load_catalog(path)["cards"]:
        card_id = card["id"]
        if card_id in illustrations:
            continue
        expansion = {"promos": "promo", "basic": "base"}.get(card["expansion"], card["expansion"])
        names = (card_id, EXPANSION_ASSET_NAMES.get(card_id, card_id), card["text"]["en"][0])
        resource = next((resources[(expansion, normalize_image_name(name))]
                         for name in names
                         if (expansion, normalize_image_name(name)) in resources), None)
        if resource is None:
            print(f"Illustration fehlt: {card_id}", file=sys.stderr)
            continue
        try:
            art = decode_texture(read(resource))
            art.thumbnail((420, 420))
            illustrations[card_id] = image_url(art, "WEBP", quality=86)
        except (ValueError, OSError, struct.error) as error:
            print(f"Illustration nicht lesbar: {card_id} ({error})", file=sys.stderr)


def normalize_image_name(name):
    return re.sub(r"[^a-z0-9]", "", name.lower())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xpack", type=Path, help="Pfad zu Dominion.xpack")
    parser.add_argument("output", type=Path, help="Pfad für das neue JSON-Bildpaket")
    parser.add_argument("--all-expansions", action="store_true", help="Illustrationen der Steam-Erweiterungen ergänzen")
    args = parser.parse_args()
    try:
        if "QOI" not in Image.registered_extensions().values():
            raise ValueError("Diese Pillow-Version unterstützt QOI noch nicht. Verwende Pillow 10.4 oder neuer.")
        pack = read_image_pack(args.xpack, args.all_expansions)
        with args.output.open("x", encoding="utf-8") as output:
            json.dump(pack, output)
    except (OSError, ValueError, KeyError, struct.error) as error:
        parser.exit(1, f"Bildpaket konnte nicht erstellt werden. {error}\n")
    print(f"{len(pack['images'])} Illustrationen und {len(pack['cards']['de'])} zweisprachige Kartenvorderseiten gespeichert in {args.output}")


if __name__ == "__main__":
    main()
