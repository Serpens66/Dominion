"""Build the editor's C++ catalog from the project's German game reference."""

import json
from pathlib import Path
import re
import sys
import unicodedata


DOCS = Path(__file__).resolve().parents[1] / "Documentation"
GENERATOR = DOCS.parent / "SetGenerator"
sys.path.insert(0, str(GENERATOR))
from generator_catalog import load_catalog, read_index, read_resource, read_strings

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
        r" \| ([^|\n]*?) \*Einmal – ([^|\n]*?) · Mehrfach – ([^|\n]*?)\* \| ([^|\n]+) \|$"
    )
    effects = []
    for name, command, args, description, once, repeated, timing in re.findall(pattern, catalog, re.MULTILINE | re.IGNORECASE):
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
        effects.append((command, plain_text(name), plain_text(description), plain_text(timing), status, first, second))
    assert len(effects) == len({row[0] for row in effects}) == 89, "Expected 89 documented effects"
    return effects


def search_key(text):
    return "".join(ch for ch in unicodedata.normalize("NFD", text.casefold()) if not unicodedata.combining(ch))


def verification_label(status):
    once, repeated = status.split("\r\n", 1)
    if "Offen" in once:
        return "Ungetestet"
    if "Teil" in once or "Teil" in repeated or "bestätigt" in once:
        return "Teilweise geprüft"
    if "Offen" in repeated:
        return "Mehrfach ungetestet"
    return ""


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


def prepare_resources(output, xpack, cards, load_types):
    from extract_card_images import (ASSET_NAMES, EXPANSION_ASSET_NAMES, VERIFIED_ART_RESOURCES,
        normalize_image_name, COMBINED_FRAMES, LANDSCAPE_TYPES, LANDSCAPE_NAMES)
    from generator_catalog import VALUE_CARDS
    with xpack.open("rb") as package:
        entries = read_index(package)
        strings = read_strings(read_resource(package, entries, "res/translation/de"))
    labels = ["#pragma once", "namespace kingdom::menu_text {"]
    labels.extend(f"inline constexpr wchar_t {name}[] = {literal(strings[key], True)};" for name, key in MENU_LABELS.items())
    (output.parent / "menu_labels.generated.hpp").write_text("\n".join(labels + ["}"]), encoding="utf-8")
    def sprite(name):
        path = "res/card_gen/shared/" + name
        return path if path in entries else ""
    def first(*names):
        return next((sprite(n) for n in names if sprite(n)), "")
    resources = {}
    for key in entries:
        parts = key.split("/")
        if len(parts) == 4 and parts[:2] == ["res", "card_gen"]:
            name = re.sub(r"^[a-z]+2e_", "", parts[3])
            if parts[2] == "promo": name = name.removesuffix("art")
            resources.setdefault((parts[2], normalize_image_name(name)), key)
    rows = ["// Resource paths and layout metadata only; no image bytes.", "constexpr ResourceCard resource_cards[] = {"]
    for card in cards:
        cid, kinds = card["id"], card["types"]
        expansion = {"basic":"base", "promos":"promo"}.get(card["expansion"], card["expansion"])
        verified = VERIFIED_ART_RESOURCES.get(cid)
        art = "res/card_gen/" + verified if verified else next((resources[(group, normalize_image_name(name))]
            for group in (expansion, "shared") for name in (ASSET_NAMES.get(cid,cid), EXPANSION_ASSET_NAMES.get(cid,cid),card["text"]["en"][0])
            if (group,normalize_image_name(name)) in resources), "")
        landscape = next((k for k in kinds if k in LANDSCAPE_TYPES), "")
        basic = next((k for k in ("night","curse","ruins","duration","reserve","reaction","treasure","victory") if k in kinds), "action")
        basic = "vp" if basic == "victory" else basic
        frame = next((f for f, required in COMBINED_FRAMES if all(k in kinds for k in required)), basic)
        if landscape:
            top = bottom = first(LANDSCAPE_NAMES.get(landscape, "name_"+landscape), "name_event")
            left = right = sprite("landscape_frame_side_detail")
        else:
            parts=[]
            for part in ("top","bot","ribl","ribr"):
                parts.append(first(f"frame_{frame}_{part}",
                    f"frame_{frame}_frame_treasuretop" if part=="top" and "treasure" in frame else "",
                    f"frame_treasure_nonstandard_{part}" if part.startswith("rib") and frame=="treasure" else "",
                    f"frame_{basic}_{part}", f"frame_action_{part}"))
            top,bottom,left,right=parts
        exp = {"plunder":"plun_plunder","allies":"14_allies"}.get(expansion,expansion)
        edition = "_1e" if card.get("edition")=="first" and exp in ("base","intrigue") else ""
        emblem = first("exp_"+exp+edition, "exp_"+exp)
        title,rules = card["text"]["de"]
        rules = re.sub(r"-//(?=[a-zäöüß])", "", rules).replace("{br}","//").replace("{coin_2}","[2]")
        rules = re.sub(r"%([^%]+)%",r"\1",rules)
        types = " – ".join(load_types.get(k,k) for k in kinds)
        cost=card.get("cost") or {"coins":-1,"debt":0,"potions":0}
        value=VALUE_CARDS.get(cid,("",""))
        fields=[literal(v) for v in (cid,art,top,bottom,left,right,emblem)]
        fields += [literal(v,True) for v in (title,rules,types)]
        fields += [str(cost[k]) for k in ("coins","potions","debt")]
        fields += [str(bool(landscape)).lower(), str("night" in kinds and not landscape).lower(),literal(value[0]),literal(value[1],True)]
        rows.append("    {" + ", ".join(fields) + "},")
    rows += ["};", "constexpr ResourceAlias resource_aliases[] = {"]
    aliases={name:"res/ui_art/"+key for name,key in MENU_ASSETS.items()}
    aliases.update({"coin":sprite("ico_coin_smaller"),"potion":sprite("ico_potion"),"debt":sprite("ico_debt"),"sun":sprite("ico_sun"),"vp":sprite("vp"),
        "actions":"res/ui_art/ico_action","attack":"res/ui_art/ico_modifier_requireattacks","reaction":"res/ui_art/ico_modifier_reaction","night":"res/ui_art/ico_night"})
    for name in ("duration","landscape","omen","favor_rules"):
        aliases[name] = first(name, "ico_"+name)
    aliases["duration"] = sprite("ico_pile_duration")
    aliases["favor_rules"] = sprite("ico_exp14_favor")
    for card in cards:
        exp=card["expansion"]
        key={"basic":"base","nocturne":"nocturne_white"}.get(exp,exp)
        aliases["exp-"+key if exp=="basic" else "exp-"+exp]="res/ui_art/exp_"+key+"_big"
    for name,key in aliases.items():
        if key in entries: rows.append("    {"+literal(name)+", "+literal(key)+"},")
    rows.append("};")
    (output.parent / "resource_catalog.generated.hpp").write_text("\n".join(rows)+"\n",encoding="utf-8")


def main(output, xpack):
    output, xpack = Path(output), Path(xpack)
    catalog = load_catalog(xpack)
    properties = json.loads((DOCS / "Card_Properties.json").read_text(encoding="utf-8"))
    aliases = {key: data["aliasFor"] for key, data in properties["cards"].items() if data and "aliasFor" in data}
    targets = read_targets(catalog, aliases)
    rows = ["// Generated from Documentation by generate_effect_catalog.py.",
            "constexpr EffectDefinition effect_catalog[] = {"]
    for command, name, description, timing, status, first, second in read_effects():
        rows.append(f"    {{{literal(command)}, {literal(name, True)}, {literal(description, True)}, {literal(timing, True)}, {literal(status, True)}, {literal(verification_label(status), True)}, "
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
    target_ids = {entry[0] for entry in targets}
    prepare_resources(output, xpack, [card for card in catalog["cards"] if card["id"] in target_ids], catalog["types"]["de"])



if __name__ == "__main__":
    main(*sys.argv[1:])
