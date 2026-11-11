"""Export individual preview images and their paths from the local Steam image pack."""

import argparse
import base64
import hashlib
import json
from pathlib import Path
import re

from generator_catalog import ROOT, replace_script, script_json


MARKER = '<script type="application/json" id="card-image-files">'


def export_images(pack_path, html_path):
    pack = json.loads(pack_path.read_text(encoding="utf-8-sig"))
    if not isinstance(pack, dict) or pack.get("format") != "dominion-card-images" or pack.get("version") != 1:
        raise ValueError("Das ist kein unterstütztes Dominion-Bildpaket.")
    source = html_path.read_text(encoding="utf-8")
    catalog_match = re.search(r'id="catalog">([\s\S]*?)</script>', source)
    if catalog_match is None:
        raise ValueError("Der Kartenkatalog fehlt in der HTML-Datei.")
    known_ids = {card["id"] for card in json.loads(catalog_match[1])["cards"]}
    manifest = {"images": {}, "cards": {}}
    files = {}

    def collect(images, group):
        if not isinstance(images, dict):
            raise ValueError(f"Ungültige Bildliste: {group}")
        paths = {}
        for card_id, value in images.items():
            if card_id not in known_ids or not re.fullmatch(r"[a-z0-9_]+", card_id):
                raise ValueError(f"Unbekannte Karten-ID: {card_id}")
            match = re.fullmatch(r"data:image/(png|webp);base64,([A-Za-z0-9+/]*={0,2})", value) if isinstance(value, str) else None
            if not match:
                raise ValueError(f"Ungültige Bilddaten: {card_id}")
            extension, encoded = match.groups()
            data = base64.b64decode(encoded, validate=True)
            valid = (data.startswith(b"\x89PNG\r\n\x1a\n") if extension == "png" else
                     data.startswith(b"RIFF") and data[8:12] == b"WEBP")
            if not valid:
                raise ValueError(f"Ungültiger Bildtyp: {card_id}")
            digest = hashlib.sha256(data).hexdigest()[:12]
            filename = f"card-images/{group}/{card_id}-{digest}.{extension}"
            paths[card_id] = filename
            files[filename] = data
        return paths

    manifest["images"] = collect(pack["images"], "illustrations")
    if not isinstance(pack.get("cards", {}), dict):
        raise ValueError("Ungültige Liste der Kartenvorderseiten.")
    for language, images in pack.get("cards", {}).items():
        if language not in ("de", "en"):
            raise ValueError(f"Unbekannte Kartensprache: {language}")
        manifest["cards"][language] = collect(images, language)
    updated = replace_script(source, MARKER, script_json(manifest))
    previous = re.search(re.escape(MARKER) + r"([\s\S]*?)</script>", source)
    previous_manifest = json.loads(previous[1])
    old_files = set(previous_manifest["images"].values())
    for images in previous_manifest.get("cards", {}).values():
        old_files.update(images.values())

    image_root = (html_path.parent / "card-images").resolve()
    for filename, data in files.items():
        target = html_path.parent / filename
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists() or target.read_bytes() != data:
            target.write_bytes(data)
    html_path.write_bytes(updated.replace("\n", "\r\n").encode("utf-8"))
    for filename in old_files - files.keys():
        target = (html_path.parent / filename).resolve()
        if target.is_relative_to(image_root):
            target.unlink(missing_ok=True)
    return len(files), sum(map(len, files.values()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack", type=Path, help="JSON-Bildpaket des Extraktionswerkzeugs")
    parser.add_argument("--html", type=Path, default=ROOT / "Dominion-Generator.html")
    args = parser.parse_args()
    try:
        count, size = export_images(args.pack, args.html)
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f"Bilder konnten nicht exportiert werden. {error}\n")
    print(f"{count} Bilddateien exportiert, {size / 1_000_000:.1f} MB.")


if __name__ == "__main__":
    main()
