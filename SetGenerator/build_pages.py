"""Build the public GitHub Pages artifact from the workspace generator."""
import argparse
from html import escape
import json
from pathlib import Path
import re
from urllib.parse import quote

from generator_catalog import ROOT, replace_script, script_json


def icon(key):
    glyphs = {
        "actions": "↻", "cards": "▤", "buy": "+", "trash": "×", "attack": "⚔",
        "reaction": "↩", "potion": "⚗", "sun": "☀", "night": "☾",
        "horse": "♞", "landscape": "▧", "first-edition": "1",
    }
    colors = {"coin": "#edbb53", "debt": "#94603d", "vp": "#65518b", "potion": "#8fc9e5"}
    color = colors.get(key, "#97b29b")
    glyph = escape(glyphs.get(key, "" if key in colors else "◆"))
    svg = (
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32">'
        f'<circle cx="16" cy="16" r="14" fill="{color}" stroke="#494236" stroke-width="2"/>'
        f'<text x="16" y="22" text-anchor="middle" font-size="17" fill="#251f18">{glyph}</text></svg>'
    )
    return "data:image/svg+xml," + quote(svg, safe="")


def build(source):
    def read_json(marker):
        matches = re.findall(re.escape(marker) + r"([\s\S]*?)</script>", source)
        if len(matches) != 1:
            raise ValueError(f"Expected one block for {marker}")
        return json.loads(matches[0])

    catalog_marker = '<script type="application/json" id="catalog">'
    icons_marker = '<script type="application/json" id="game-icons">'
    frames_marker = '<script type="application/json" id="card-frames">'
    catalog = read_json(catalog_marker)
    for card in catalog["cards"]:
        card["text"] = {lang: [parts[0], ""] for lang, parts in card["text"].items()}
    for languages in catalog["expansionInfo"].values():
        for lang in languages:
            description = "Rules are available in Dominion." if lang == "en" else "Die Regeln findest du in Dominion."
            languages[lang] = {"description": description, "rules": []}
    icons = {key: icon(key) for key in read_json(icons_marker)}
    source = replace_script(source, catalog_marker, script_json(catalog))
    source = replace_script(source, icons_marker, script_json(icons))
    source = replace_script(source, frames_marker, "[]")
    style = (
        "<style>\n"
        ".preview-face-title, .preview-face-footer, .preview-face-body { background-color: #eee3ca; }\n"
        ".preview-face-night .preview-face-title, .preview-face-night .preview-face-footer { background-color: #29283c; }\n"
        "</style>\n"
    )
    source, count = re.subn(r'<style>\s*:root\s*\{\s*--sprite-[\s\S]*?</style>\s*', lambda _: style, source)
    if count != 1 or 'id="embedded-image-pack"' in source or "data:image/png" in source or "data:image/webp" in source:
        raise ValueError("Unexpected bundled image resources in Pages output.")
    source, count = re.subn(
        r'<p class="muted" id="image-help"[^>]*>[\s\S]*?</p>',
        '<p class="muted" id="image-help" data-en="You can open your own image pack here.">'
        'Hier kannst du dein eigenes Bildpaket öffnen.</p>', source,
    )
    if count != 1:
        raise ValueError("Image help block missing.")
    return (source.replace("See game setup.", "Card rules are available in Dominion.")
            .replace("Siehe Spielaufbau.", "Die Kartenregeln findest du in Dominion.")
            .replace("Originalbeschreibung aus der Steam-Version", "Erweiterungsübersicht")
            .replace("Original description from the Steam version", "Expansion overview")
            .replace('Dominion-Konverter.html', 'https://github.com/Serpens66/Dominion/tree/main/SetGenerator'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="Path to the generated index.html")
    args = parser.parse_args()
    source_path = ROOT / "Dominion-Generator.html"
    if args.output.resolve() == source_path:
        parser.error("Use a separate destination for the generated Pages artifact.")
    output = build(source_path.read_text(encoding="utf-8"))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
