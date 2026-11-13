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
        r" \| ([^|\n]*?) \*Einmal – ([^|\n]*?) · Mehrfach – ([^|\n]*?)\* \| ([^|\n]+) \|(?: [^|\n]+ \|)*$"
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


def read_english_effects():
    text=(DOCS/'Steam_Spielanleitung.md').read_text(encoding='utf-8')
    text=text.split('### Campaign effect catalog',1)[1].split('#### Other campaign generation variants',1)[0]
    pattern=r"^\| ([^|]+) \| `((?:twist|stamp|start)_[a-z0-9_]+)(?:\(([^)]+)\))?` \| ([^|\n]*?) \*Once – ([^|\n]*?) · Repeated – ([^|\n]*?)\* \| ([^|\n]+) \|(?: [^|\n]+ \|)*$"
    rows={id:(plain_text(name),plain_text(description),plain_text(timing),f"Once – {once}\r\nRepeated – {repeated}") for name,id,args,description,once,repeated,timing in re.findall(pattern,text,re.M|re.I)}
    assert len(rows)==89, 'Expected 89 English effects'
    return rows


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


TIMING_FILTERS = (
    "Spielaufbau", "Zugbeginn", "Beim Spielen einer Karte", "Beim Spielen der gewählten Karte",
    "Beim Nehmen einer Karte", "Beim Kaufen", "Beim Entsorgen", "Beim Ablegen oder Einmischen",
    "Kaufphase", "Zugende/Aufräumphase", "Dauerhaft", "Wertung", "Besondere Auslöser",
)


def timing_mask(timing):
    # Compile the documented German conditions into language-independent flags.
    text = search_key(timing)
    tests = (
        "spielaufbau" in text or "beginn der kampagnenpartie" in text,
        "zugbeginn" in text or "beginn jedes menschlichen zuges" in text or "beginn menschlicher zuge" in text or "nachsten zug" in text,
        "ausspielen" in text or "spielbar" in text,
        ("ausspielen" in text or "spielbar" in text) and any(word in text for word in ("gewahlte karte", "gewahlten karte", "gewahlten stapel")),
        bool(re.search(r"\bnehmen\b", text)),
        "kaufen" in text or "kauf des" in text,
        "entsorgen" in text,
        "ablegen" in text or "einmischen" in text,
        "kaufphase" in text or "trank-guthaben" in text or "bezahlen" in text,
        "zugende" in text or ("aufraumphase" in text and "ausserhalb der aufraumphase" not in text) or "normalen neuen hand" in text,
        "dauerhaft" in text or "kostenermittlung" in text or "ermittlung von kartenkosten" in text or "trank-guthaben" in text or "wahrend der partie" in text,
        "wertung" in text or "siegpunkte vergibt" in text,
        "prophezeiung" in text or "gabe" in text or "siegpunktmarkern" in text or "provinz" in text or "goldes" in text or "wahrend der partie" in text,
    )
    mask = sum(1 << index for index, active in enumerate(tests) if active)
    if not mask: raise ValueError(f"No trigger category for: {timing}")
    return mask


def rule_text(text, english=False):
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
    result="\r\n".join("".join(ch for ch in line if ch >= " " or ch == "\t") for line in text.splitlines())
    if english:
        for de,en in [('Münzen','Coins'),('Schulden','Debt'),('Trank','Potion'),('Sonnenmarker','Sun tokens'),('Siegpunkte','VP'),('Embargomarker','Embargo tokens'),('Geldmarker','Coin tokens'),('Gefallen-Marker','Favor tokens')]: result=result.replace(de,en)
    return result


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
    with xpack.open('rb') as package: english_strings=read_strings(read_resource(package,entries,'res/translation/en-us'))
    labels.extend(f"inline const wchar_t* {name}() {{ return ui::pick({literal(strings[key],True)},{literal(english_strings["dom_msg_cancel"] if name=="cancel" else english_strings[key],True)}); }}" for name,key in MENU_LABELS.items())
    (output.parent / "menu_labels.generated.hpp").write_text("\n".join(labels + ["}"]), encoding="utf-8")
    def sprite(name):
        path = "res/card_gen/shared/" + name
        return path if path in entries else ""
    def first(*names):
        return next((sprite(n) for n in names if sprite(n)), "")
    english_types=load_catalog(xpack)["types"]["en"]
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
        en_title,en_rules=card['text']['en']
        en_rules=re.sub(r'%([^%]+)%',r'\1',en_rules).replace('{br}','//').replace('{coin_2}','[2]')
        fields += [literal(v,True) for v in (en_title,en_rules,' – '.join(english_types.get(k,k) for k in kinds))]
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
    references=json.loads((DOCS/'Effect_Expansion_References.json').read_text(encoding='utf-8'))
    assert references['exeSha256']==catalog['nativeIds']['exeSha256']
    assert set(references['effects'])=={row[0] for row in read_effects()}
    properties = json.loads((DOCS / "Card_Properties.json").read_text(encoding="utf-8"))
    aliases = {key: data["aliasFor"] for key, data in properties["cards"].items() if data and "aliasFor" in data}
    targets = read_targets(catalog, aliases)
    access_rows = ["// Generated target arities and native identities; no purchase data.", "namespace kingdom::access_catalog {", "inline constexpr TargetAccessRule rules[]{"]
    for command, _, _, _, _, first, second in read_effects():
        count = 0 if first == "none" else (2 if second != "none" else 1)
        access_rows.append("    {" + literal(command) + ", " + str(count) + ", " + str(20 if first == "pile_list" else count) + "},")
    access_rows += ["};", "inline constexpr TargetAccessNativeCard cards[]{"]
    for card in catalog["cards"]:
        access_rows.append("    {" + literal(card["id"]) + ", " + str(card["nativeId"]) + "},")
    access_rows += ["};", "inline constexpr std::pair<std::string_view, std::string_view> aliases[]{"]
    for alias, canonical in aliases.items():
        access_rows.append("    {" + literal(alias) + ", " + literal(canonical) + "},")
    access_rows += ["};", "}"]
    (output.parent / "target_access.generated.hpp").write_text("\n".join(access_rows) + "\n", encoding="utf-8")
    english_effects=read_english_effects()
    requirement_data=json.loads((DOCS/'Effect_Requirements.json').read_text(encoding='utf-8'))
    assert requirement_data['exeSha256']==catalog['nativeIds']['exeSha256']
    requirements=requirement_data['effects']
    assert all(all(row) for row in english_effects.values()), 'Missing English effect text'
    for effect_id,req in requirements.items():
        assert req['kind'] in ('base','mechanic','unclear'), effect_id
        assert all(req[key] for key in ('de','en','detail_de','detail_en','evidence','evidence_detail')), effect_id
    assert set(requirements)==set(english_effects)=={row[0] for row in read_effects()}
    by_id={card['id']:card for card in catalog['cards']}
    ui_pairs=json.loads((Path(__file__).parent/'ui_translations.json').read_text(encoding='utf-8'))
    (output.parent/'ui_translations.generated.hpp').write_text('static constexpr Pair pairs[]{\n'+''.join('{'+literal(de,True)+','+literal(en,True)+'},\n' for de,en in ui_pairs.items())+'};\n',encoding='utf-8')
    rows = ["// Generated from Documentation by generate_effect_catalog.py.",
            "constexpr EffectDefinition effect_catalog[] = {"]
    for command, name, description, timing, status, first, second in read_effects():
        expansion=' oder '.join(catalog['expansions'].get(key,key) for group in references['effects'][command] for key in group)
        en_name,en_description,en_timing,en_status=english_effects[command]
        en_ref=' or '.join(catalog['expansionsEn'].get(key,key) for group in references['effects'][command] for key in group)
        req=requirements[command]
        extra=', '.join(literal(v,True) for v in (en_name,en_description,en_timing,en_status,en_ref,req['de'],req['en'],req['detail_de'],req['detail_en']))
        rows.append(f"    {{{literal(command)}, {literal(name, True)}, {literal(description, True)}, {literal(timing, True)}, {literal(status, True)}, {literal(verification_label(status), True)}, {literal(expansion,True)}, {timing_mask(timing)}, "
                    f"TargetKind::{first}, TargetKind::{second}, {extra}}},")
    rows.extend(("};", "", "constexpr const wchar_t* timing_filters[] = {"))
    rows.extend(f"    {literal(label, True)}," for label in TIMING_FILTERS)
    rows.extend(("};", "", "constexpr CardTarget target_catalog[] = {"))
    for card_id, *metadata, expansion_id, icons, costs, kinds in targets:
        mask = " | ".join(f"kind_mask(TargetKind::{kind})" for kind in kinds)
        fields = ", ".join(literal(value, True) for value in metadata)
        price = ", ".join(map(str, costs))
        props=by_id[card_id]; en_title,en_rules=props['text']['en']; cost=props.get('cost')
        en_cost=[]
        if cost:
            if cost['coins'] or not(cost['potions'] or cost['debt']): en_cost.append(f"{cost['coins']} Coins")
            if cost['potions']: en_cost.append(f"{cost['potions']} Potion")
            if cost['debt']: en_cost.append(f"{cost['debt']} Debt")
        en_extra=', '.join(literal(v,True) for v in (en_title, 'Basic Cards' if expansion_id=='basic' else catalog['expansionsEn'][expansion_id], ' · '.join(catalog['types']['en'].get(k,k) for k in props['types']), ' + '.join(en_cost),rule_text(en_rules,True)))
        rows.append(f"    {{{literal(card_id)}, {fields}, {literal(expansion_id)}, {literal(icons)}, {price}, {mask}, {en_extra}}},")
    rows.extend(("};", "", "constexpr std::pair<std::string_view, std::string_view> target_aliases[] = {"))
    for alias, canonical in aliases.items():
        rows.append(f"    {{{literal(alias)}, {literal(canonical)}}},")
    rows.append("};")
    output.write_text("\n".join(rows) + "\n", encoding="utf-8")
    expansion_rows=['constexpr ExpansionInfo expansion_catalog[]{']
    for exp in catalog['expansions']:
        info=catalog['expansionInfo'].get(exp,{})
        de=info.get('de',{}); en=info.get('en',{})
        values=(exp,'Grundkarten' if exp=='basic' else catalog['expansions'][exp], 'Basic Cards' if exp=='basic' else catalog['expansionsEn'].get(exp,exp),rule_text(de.get('description','')),rule_text(en.get('description',''),True),rule_text('\n\n'.join(de.get('rules',[]))),rule_text('\n\n'.join(en.get('rules',[])),True))
        expansion_rows.append('    {'+literal(values[0])+', '+', '.join(literal(v,True) for v in values[1:])+'},')
    expansion_rows.append('};')
    (output.parent/'expansion_catalog.generated.hpp').write_text('\n'.join(expansion_rows)+'\n',encoding='utf-8')
    target_ids = {entry[0] for entry in targets}
    prepare_resources(output, xpack, [card for card in catalog["cards"] if card["id"] in target_ids], catalog["types"]["de"])



if __name__ == "__main__":
    main(*sys.argv[1:])
