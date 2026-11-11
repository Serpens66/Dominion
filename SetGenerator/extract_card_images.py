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

from generator_catalog import VALUE_CARDS

try:
    from PIL import Image, ImageDraw, ImageFont, ImageOps
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
    "acting_troupe": "acting_troup", "sextant": "sextatnt",
    "smugglers": "smuggler", "witchs_hut": "witchhut", "wheelwright": "wheelright",
    "philosophers_stone": "philosopherstone", "summon": "summon-1",
    "cargo_ship": "carrack",
    "catacombs": "catacomb",
    "cavalry": "cavalary",
    "changeling": "slavin_dominion_changeling",
    "coin_of_the_realm": "coinsoftherealm",
    "count": "count_final",
    "destrier": "desterier",
    "falconer": "falconier",
    "farming_village": "farmvillage",
    "forum": "forums",
    "goatherd": "goatherding",
    "haggler": "haggler_final",
    "hostelry": "holstery",
    "hunting_party": "hunter",
    "noble_brigand": "nobel_brigand",
    "royal_blacksmith": "blacksmith",
    "spices": "spice_market",
    "flag": "artifact_flag", "key": "artifact_key", "horn": "artifact_horn",
    "lantern": "artifact_lantern", "treasure_chest": "artifact_chest",
}
# Exact card-to-texture assignments verified against Steam build 3315.
VERIFIED_ART_RESOURCES = {
    "academy": "renaissance/renaissance project cards-29",
    "advance": "empires/events-1",
    "alliance": "menagerie/event cards02-59",
    "alms": "adventures/adventure event cards-1",
    "annex": "empires/events-8",
    "aqueduct": "empires/landmark-1",
    "arena": "empires/landmark-7",
    "bad_omens": "nocturne/nocturne hex cards-1",
    "ball": "adventures/adventure event cards-4",
    "bandit_fort": "empires/landmark-12",
    "banish": "menagerie/event cards02-30",
    "banquet": "empires/events-16",
    "bargain": "menagerie/event cards02-33",
    "barracks": "renaissance/renaissance project cards-48",
    "basilica": "empires/landmark-16",
    "baths": "empires/landmark-22",
    "battlefield": "empires/landmark-27",
    "bonfire": "adventures/adventure event cards-8",
    "borrow": "adventures/adventure event cards-10",
    "canal": "renaissance/renaissance project cards-57",
    "capitalism": "renaissance/renaissance project cards-32",
    "cathedral": "renaissance/renaissance project cards-1",
    "citadel": "renaissance/renaissance project cards-62",
    "city_gate": "renaissance/renaissance project cards-4",
    "colonnade": "empires/landmark-32",
    "colony": "shared/colony",
    "commerce": "menagerie/event cards02-43",
    "conquest": "empires/events-37",
    "copper": "shared/copper",
    "crop_rotation": "renaissance/renaissance project cards-51",
    "defiled_shrine": "empires/landmark-37",
    "delay": "menagerie/event cards02-1",
    "deluded": "nocturne/nocturne hex cards-3",
    "delusion": "nocturne/nocturne hex cards-3",
    "delve": "empires/events-14",
    "demand": "menagerie/event cards02-46",
    "desperation": "menagerie/event cards02-4",
    "dominate": "empires/events-40",
    "donate": "empires/events-11",
    "duchy": "shared/duchy",
    "enclave": "menagerie/event cards02-56",
    "enhance": "menagerie/event cards02-20",
    "envious": "nocturne/nocturne hex cards-5",
    "envy": "nocturne/nocturne hex cards-5",
    "estate": "shared/estate",
    "expedition": "adventures/adventure event cards-13",
    "exploration": "renaissance/renaissance project cards-17",
    "fair": "renaissance/renaissance project cards-20",
    "famine": "nocturne/nocturne hex cards-7",
    "fear": "nocturne/nocturne hex cards-9",
    "ferry": "adventures/adventure event cards-15",
    "fleet": "renaissance/renaissance project cards-36",
    "fountain": "empires/landmark-43",
    "gamble": "menagerie/event cards02-8",
    "greed": "nocturne/nocturne hex cards-11",
    "guildhall": "renaissance/renaissance project cards-39",
    "haunting": "nocturne/nocturne hex cards-13",
    "inheritance": "adventures/adventure event cards-20",
    "innovation": "renaissance/renaissance project cards-54",
    "invest": "menagerie/event cards02-37",
    "keep": "empires/landmark-47",
    "labyrinth": "empires/landmark-51",
    "locusts": "nocturne/nocturne hex cards-15",
    "lost_arts": "adventures/adventure event cards-23",
    "lost_in_the_woods": "nocturne/lost_in_the_wood",
    "magic_lamp": "nocturne/lamp",
    "march": "menagerie/event cards02-24",
    "miserable": "nocturne/nocturne hex cards-17",
    "misery": "nocturne/nocturne hex cards-17",
    "mission": "adventures/adventure event cards-25",
    "mountain_pass": "empires/landmark-56",
    "museum": "empires/landmark-62",
    "obelisk": "empires/landmark-66",
    "orchard": "empires/landmark-70",
    "pageant": "renaissance/renaissance project cards-7",
    "palace": "empires/landmark-74",
    "pathfinding": "adventures/adventure event cards-27",
    "piazza": "renaissance/renaissance project cards-42",
    "pilgrimage": "adventures/adventure event cards-29",
    "plague": "nocturne/nocturne hex cards-19",
    "plan": "adventures/adventure event cards-31",
    "populate": "menagerie/event cards02-62",
    "poverty": "nocturne/nocturne hex cards-21",
    "pursue": "menagerie/event cards02-11",
    "quest": "adventures/adventure event cards-33",
    "raid": "adventures/adventure event cards-35",
    "reap": "menagerie/event cards02-50",
    "ride": "menagerie/event cards02-14",
    "ritual": "empires/events-19",
    "road_network": "renaissance/renaissance project cards-45",
    "ruins": "dark_ages/ruined_library",
    "salt_the_earth": "empires/events-23",
    "save": "adventures/adventure event cards-37",
    "scouting_party": "adventures/adventure event cards-39",
    "seaway": "adventures/adventure event cards-41",
    "seize_the_day": "menagerie/event cards02-40",
    "sewers": "renaissance/renaissance project cards-11",
    "silos": "renaissance/renaissance project cards-23",
    "sinister_plot": "renaissance/renaissance project cards-26",
    "stampede": "menagerie/event cards02-53",
    "star_chart": "renaissance/renaissance project cards-14",
    "tax": "empires/events-26",
    "the_earths_gift": "nocturne/nocturne boon cards-1",
    "the_fields_gift": "nocturne/nocturne boon cards-4",
    "the_flames_gift": "nocturne/nocturne boon cards-7",
    "the_forests_gift": "nocturne/nocturne boon cards-9",
    "the_moons_gift": "nocturne/nocturne boon cards-12",
    "the_mountains_gift": "nocturne/nocturne boon cards-14",
    "the_rivers_gift": "nocturne/nocturne boon cards-16",
    "the_seas_gift": "nocturne/nocturne boon cards-18",
    "the_skys_gift": "nocturne/nocturne boon cards-20",
    "the_suns_gift": "nocturne/nocturne boon cards-22",
    "the_swamps_gift": "nocturne/nocturne boon cards-24",
    "the_winds_gift": "nocturne/nocturne boon cards-26",
    "toil": "menagerie/event cards02-17",
    "tomb": "empires/landmark-78",
    "tower": "empires/landmark-82",
    "trade": "adventures/adventure event cards-44",
    "training": "adventures/adventure event cards-46",
    "transport": "menagerie/event cards02-27",
    "travelling_fair": "adventures/adventure event cards-50",
    "triumph": "empires/events-4",
    "triumphal_arch": "empires/landmark-86",
    "twice_miserable": "nocturne/nocturne hex cards-17",
    "wall": "empires/landmark-90",
    "war": "nocturne/nocturne hex cards-23",
    "way_of_the_butterfly": "menagerie/way cards02-1",
    "way_of_the_camel": "menagerie/way cards02-4",
    "way_of_the_chameleon": "menagerie/way_of_the_chameleon_final",
    "way_of_the_frog": "menagerie/way cards02-9",
    "way_of_the_goat": "menagerie/way cards02-11",
    "way_of_the_horse": "menagerie/way cards02-13",
    "way_of_the_mole": "menagerie/mole_way",
    "way_of_the_monkey": "menagerie/way cards02-17",
    "way_of_the_mouse": "menagerie/way cards02-20",
    "way_of_the_mule": "menagerie/way cards02-24",
    "way_of_the_otter": "menagerie/way cards02-27",
    "way_of_the_owl": "menagerie/way cards02-29",
    "way_of_the_ox": "menagerie/way cards02-31",
    "way_of_the_pig": "menagerie/way cards02-33",
    "way_of_the_rat": "menagerie/way_of_the_rat_final",
    "way_of_the_seal": "menagerie/way cards02-37",
    "way_of_the_sheep": "menagerie/way of the sheep_final",
    "way_of_the_squirrel": "menagerie/way cards02-43",
    "way_of_the_turtle": "menagerie/way cards02-45",
    "way_of_the_worm": "menagerie/way_of_the_worm_final",
    "wedding": "empires/events-31",
    "windfall": "empires/events-35",
    "wolf_den": "empires/landmark-94",
}
INDEX_ENTRY_SIZE = 168
IMAGE_POINTER_OFFSET = 128
CARD_SIZE = (344, 524)
RULE_TOKEN = re.compile(r"\||\x60|\[(!?\d*)([DPS]?)\]|\{(!?[+-]?\d*)\}|\s+|[^\s|\x60\[\]{}]+|.")
SYMBOLS = {"": "coin", "D": "debt", "P": "potion", "S": "sun"}
LANDSCAPE_TYPES = {"event", "landmark", "project", "way", "trait", "boon", "hex", "state",
                   "artifact", "ally", "prophecy"}
# Same priority as cardFrameStyle in the SetGenerator.
COMBINED_FRAMES = (
    ("duration_victory_treasure", ("duration", "victory", "treasure")),
    ("reaction_treasure_duration", ("reaction", "treasure", "duration")),
    *((frame, tuple(frame.split("_"))) for frame in (
        "duration_reaction", "duration_victory", "duration_treasure", "reserve_victory", "reserve_treasure",
        "treasure_reaction", "victory_reaction", "shelter_reaction", "shelter_victory", "action_shelter",
        "curse_treasure", "ruins_treasure", "treasure_victory", "action_treasure", "action_victory")),
)
LANDSCAPE_NAMES = {"trait": "plun_name_trait", "ally": "allies_name_ally", "prophecy": "rising_sun_name_prophecy"}

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
                if match[1] is not None:
                    symbol = SYMBOLS[match[2]]
                    value = match[1].lstrip("!")
                elif match[3] is not None:
                    symbol, value = "vp", match[3].lstrip("!").replace("-", "−")
                else:
                    symbol = ""
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


def render_card(art, sprites, title, text, costs, type_line, *, landscape=False, dark=False, value=None):
    width, height = CARD_SIZE[::-1] if landscape else CARD_SIZE
    face = Image.new("RGBA", (width, height))
    draw = ImageDraw.Draw(face)
    draw.rounded_rectangle((1, 1, width - 2, height - 2), radius=15, fill="#171512")

    def place(name, x, y, w, h):
        if sprite := sprites.get(name):
            face.alpha_composite(sprite.resize((w, h), Image.Resampling.LANCZOS), (x, y))

    art_top = 39 if landscape else 47
    footer = height - (48 if landscape else 68)
    full_art = value and not text
    art_height = footer + 18 - art_top if full_art else (146 if landscape else 229)
    rules_top = art_top + art_height if art else art_top
    place("txt_back", 19, art_top, width - 38, footer + 24 - art_top)
    if art:
        fitted = ImageOps.fit(art.convert("RGBA"), (width - 38, art_height), Image.Resampling.LANCZOS)
        face.alpha_composite(fitted, (19, art_top))
    if art and not full_art:
        place("text_back_lip", 19, rules_top - 4, width - 38, 9)

    if landscape:
        place("left", 6, 31, 15, height - 67)
        place("right", width - 21, 31, 15, height - 67)
        place("top", 5, 7, width - 10, 43)
        place("bottom", 5, footer, width - 10, 40)
    else:
        for x in (13, width - 24):
            place("univ_pole", x, 15, 10, height - 38)
            place("univ_topclip", x - 3, 2, 18, 12)
            place("univ_baseclip", x - 3, height - 20, 18, 7)
        place("left", 8, art_top + 1, 22, 184)
        place("right", width - 31, art_top + 1, 24, 184)
        place("top", 5, 7, width - 10, 57)
        place("bottom", 5, footer, width - 10, 59)
    place("expansion", width - 48, height - (37 if landscape else 43), 29, 29)

    def centered(label, x, y, size, limit, ink):
        while card_font(size).getlength(label) > limit and size > 10:
            size -= 1
        draw.text((x, y), label, font=card_font(size), fill=ink, anchor="mm")

    ink = "#fff8eb" if dark else "#16120e"
    centered(title.upper(), width / 2, 28 if landscape else 34, 35, width - 41, ink)
    cost_size = (54 if landscape else 70) if len(costs) == 1 else (41 if landscape else 43)
    cost_end = cost_size + 5 if len(costs) == 1 else len(costs) * (cost_size - 5) + 10
    footer_start = max(57, cost_end)
    centered(type_line.upper(), (footer_start + width - 52) / 2, height - (26 if landscape else 34),
             23, width - 54 - footer_start, ink)

    def draw_value(symbol, label, x, y, diameter):
        place(symbol, x, y, diameter, diameter)
        if label:
            draw.text((x + diameter / 2, y + diameter / 2), label,
                      font=card_font(int(diameter * (.62 if len(label) > 1 else .81)), True),
                      fill="#16120e" if symbol in ("coin", "sun") else "#fffdf3",
                      anchor="mm", stroke_width=int(symbol == "vp"), stroke_fill="#201825")

    if value:
        draw_value(*value, width // 2 - 44, art_top + 8, 88)
    if text:
        text = re.sub(r"-//(?=[a-zäöüß])", "", text).replace("{br}", "//").replace("{coin_2}", "[2]")
        text = re.sub(r"%([^%]+)%", r"\1", text)
        text_width = width - 60
        for size in range(25, 10, -1):
            rows = rules_layout(text, size, text_width)
            rules_height = sum(row[2] for row in rows)
            if rules_height <= footer - rules_top - 11 and max(row[0] for row in rows) <= text_width:
                break
        else:
            raise ValueError(f"Der Regeltext von {title} passt nicht auf die Karte.")
        y = (rules_top + 5 + footer - 3 - rules_height) / 2
        for row_width, fragments, row_height, separator in rows:
            center = y + row_height / 2
            if separator:
                draw.line((width / 3, center, width * 2 / 3, center), fill="#453c35")
            for offset, label, font, symbol in fragments:
                x = (width - row_width) / 2 + offset
                if symbol:
                    diameter = symbol_diameter(symbol, size)
                    draw_value(symbol, label, round(x), round(center - diameter / 2), diameter)
                else:
                    draw.text((x, center), label, font=font, fill="#151311", anchor="lm")
            y += row_height
    for i, (symbol, label) in enumerate(costs):
        draw_value(symbol, label, 1 + i * (cost_size - 5), height - cost_size - (6 if landscape else 9), cost_size)
    return face


def image_url(image: Image.Image, format_name: str, **options) -> str:
    output = io.BytesIO()
    image.save(output, format=format_name, **options)
    return f"data:image/{format_name.lower()};base64," + base64.b64encode(output.getvalue()).decode("ascii")


def decode_artwork(read, atlas, path: str) -> Image.Image:
    resource = read(path)
    offset = struct.unpack_from("<Q", resource, IMAGE_POINTER_OFFSET)[0]
    if resource[offset:offset + 4] == b"qoif":
        return decode_texture(resource)
    width, height = struct.unpack_from("<II", resource, 0x28)
    x, y = struct.unpack_from("<II", resource, 0x5C)
    name = resource[0x78:].split(b"\0", 1)[0].decode("utf-8").removesuffix(".texture")
    texture = atlas(name)
    if not (0 < width <= texture.width - x and 0 < height <= texture.height - y):
        raise ValueError(f"Ungültiger Ausschnitt in der Textur {name}.")
    return texture.crop((x, y, x + width, y + height))


def render_catalog_card(card, types, art, sprite, expansion_icon):
    """Compose a localized face using the same shared textures as the image pack."""
    kinds = card["types"]
    landscape = next((kind for kind in kinds if kind in LANDSCAPE_TYPES), None)
    combined = next((frame for frame, required in COMBINED_FRAMES if all(kind in kinds for kind in required)), None)
    basic = next((kind for kind in ("night", "curse", "ruins", "duration", "reserve", "reaction", "treasure", "victory")
                  if kind in kinds), "action")
    basic = "vp" if basic == "victory" else basic
    frame = combined or basic

    sprites = {name: sprite(name) for name in
               ("txt_back", "text_back_lip", "univ_pole", "univ_topclip", "univ_baseclip", "vp")}
    sprites.update(coin=sprite("ico_coin_smaller"), debt=sprite("ico_debt"),
                   potion=sprite("ico_potion"), sun=sprite("ico_sun"))
    if landscape:
        name = LANDSCAPE_NAMES.get(landscape, f"name_{landscape}")
        sprites["top"] = sprites["bottom"] = sprite(name) or sprite("name_event")
        side = sprite("landscape_frame_side_detail")
        if side:
            sprites["left"], sprites["right"] = side, ImageOps.mirror(side)
    else:
        for key, part in (("top", "top"), ("bottom", "bot"), ("left", "ribl"), ("right", "ribr")):
            names = [f"frame_{frame}_{part}"]
            if part == "top" and "treasure" in frame:
                names.append(f"frame_{frame}_frame_treasuretop")
            if part.startswith("rib") and frame == "treasure":
                names.append(f"frame_treasure_nonstandard_{part}")
            names.extend((f"frame_{basic}_{part}", f"frame_action_{part}"))
            sprites[key] = next((asset for name in names if (asset := sprite(name))), None)

    expansion = card["expansion"]
    expansion = {"basic": "base", "promos": "promo", "plunder": "plun_plunder", "allies": "14_allies"}.get(expansion, expansion)
    edition = "_1e" if card.get("edition") == "first" and expansion in ("base", "intrigue") else ""
    emblem = sprite(f"exp_{expansion}{edition}") or expansion_icon
    dark = "night" in kinds and not landscape
    if emblem:
        sprites["expansion"] = Image.new("RGBA", emblem.size, "#fff8eb" if dark else "#1f1b14")
        sprites["expansion"].putalpha(emblem.getchannel("A"))

    costs = []
    if cost := card.get("cost"):
        if cost["coins"] or not (cost["potions"] or cost["debt"]):
            costs.append(("coin", str(cost["coins"])))
        if cost["potions"]:
            costs.append(("potion", str(cost["potions"]) if cost["potions"] > 1 else ""))
        if cost["debt"]:
            costs.append(("debt", str(cost["debt"])))
    title, rules = card["text"]["de"]
    type_line = " – ".join(types.get(kind, kind) for kind in kinds)
    return render_card(art, sprites, title, rules, costs, type_line,
                       landscape=bool(landscape), dark=dark, value=VALUE_CARDS.get(card["id"]))


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

        def artwork(path: str) -> Image.Image:
            return decode_artwork(read, atlas, path)

        def sprite(name: str) -> Image.Image:
            return artwork(f"res/card_gen/shared/{name}")

        sprites = {
            name: sprite(name)
            for name in ("txt_back", "text_back_lip", "univ_pole", "univ_topclip", "univ_baseclip",
                         "exp_base", "vp", "ico_coin_smaller")
        }
        sprites["coin"] = sprites.pop("ico_coin_smaller")
        expansion = Image.new("RGBA", sprites["exp_base"].size, "#1f1b14")
        expansion.putalpha(sprites["exp_base"].getchannel("A"))
        sprites["expansion"] = expansion
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
                card_type = CARD_TYPES.get(card_id, "action")
                frame = card_type if card_type in ("vp", "reaction") else "action"
                assets = sprites | {key: sprites[f"frame_{frame}_{part}"] for key, part in
                                    (("top", "top"), ("bottom", "bot"), ("left", "ribl"), ("right", "ribr"))}
                face = render_card(art, assets, strings[f"dom_card_title_{card_id}"],
                                   strings[f"dom_card_power_{card_id}"], [("coin", str(cost))],
                                   TYPE_LABELS[language][card_type])
                complete_cards[language][card_id] = image_url(face, "WEBP", quality=94)
        if expansion_images:
            add_expansion_images(path, entries, artwork, illustrations)
        return {"format": "dominion-card-images", "version": 1,
                "images": illustrations, "cards": complete_cards}


def add_expansion_images(path, entries, artwork, illustrations):
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
        verified = VERIFIED_ART_RESOURCES.get(card_id)
        resource = f"res/card_gen/{verified}" if verified else next(
            (resources[(group, normalize_image_name(name))]
             for group in (expansion, "shared") for name in names
             if (group, normalize_image_name(name)) in resources), None
        )
        if resource is None:
            print(f"Illustration fehlt: {card_id}", file=sys.stderr)
            continue
        try:
            art = artwork(resource)
            art.thumbnail((420, 420))
            illustrations[card_id] = image_url(art, "WEBP", quality=86)
        except (ValueError, OSError, KeyError, struct.error) as error:
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
