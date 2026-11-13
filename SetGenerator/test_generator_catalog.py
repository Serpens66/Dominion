"""Native ID coverage and synthetic ban-list XML; never edit a real profile."""
import hashlib
import json
import re
from pathlib import Path
import subprocess
from types import SimpleNamespace
import unittest
import xml.etree.ElementTree as ET

from generator_catalog import EXE_HASH, ROOT, load_web_name_catalog, read_native_ids


class BanExportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Execute the real formatter, not a Python reimplementation of it.
        script = r'''
const fs = require("node:fs"), vm = require("node:vm");
const html = fs.readFileSync(process.argv[1], "utf8");
const catalog = JSON.parse(html.match(/id="catalog">([\s\S]*?)<\/script>/)[1]);
const byId = new Map(catalog.cards.map(card => [card.id, card]));
const source = html.match(/function formatBanEntries\([\s\S]*?\n\}/)[0];
const format = vm.runInNewContext(source + "\nformatBanEntries", {
  cardById: byId, tr: de => de, uiLanguage: "de", cardTitle: card => card.text.de[0],
});
console.log(JSON.stringify({catalog, snippet: format(["foray", "village", "cellar", "village"]).text}));
'''
        output = subprocess.run(["node", "-e", script, str(ROOT / "Dominion-Generator.html")],
                                capture_output=True, text=True, encoding="utf-8", check=True)
        cls.data = json.loads(output.stdout)

    def test_synthetic_local_and_online_xml(self):
        for field in ("localBans", "bans"):
            with self.subTest(field=field):
                source = ("<ProfileSettings_Save><profiles><" + field + "><bans>771</bans>" +
                          self.data["snippet"] + "</" + field +
                          "><unrelated>preserved</unrelated></profiles></ProfileSettings_Save>")
                root = ET.fromstring(source)
                self.assertEqual([int(item.text) for item in root.findall(f"profiles/{field}/bans")],
                                 [771, 512, 518, 4412])
                self.assertEqual(root.findtext("profiles/unrelated"), "preserved")
                self.assertTrue(all(len(item) == 0 for item in root.findall(f"profiles/{field}/bans")))

    def test_native_catalog_matches_executable(self):
        exe = Path(r"E:\ProgrammeE\Steam\steamapps\common\Dominion\Dominion.exe")
        if not exe.exists():
            self.skipTest("Build-3315-EXE is not installed on this machine")
        cards = self.data["catalog"]["cards"]
        mapping = read_native_ids(exe, {card["id"] for card in cards})
        self.assertEqual(len(mapping), 853)
        self.assertEqual(len(set(mapping.values())), 853)
        self.assertEqual(mapping, {card["id"]: card["nativeId"] for card in cards})
        self.assertEqual(hashlib.sha256(exe.read_bytes()).hexdigest(), EXE_HASH)

    def test_wrong_executable_is_rejected(self):
        exe = SimpleNamespace(read_bytes=lambda: b"test fixture, not a Dominion executable")
        with self.assertRaisesRegex(ValueError, "Spielbuild 3315"):
            read_native_ids(exe, {"cellar"})

    def test_web_name_catalog(self):
        web = load_web_name_catalog(self.data["catalog"])
        self.assertEqual(set(web["languages"]), {"de", "en", "es", "fr", "it", "nl", "pl"})
        self.assertEqual(len(web["names"]), 765)
        self.assertEqual(len(web["cards"]), 703)
        unique_names = {(row["id"], json.dumps(row["titles"], ensure_ascii=False, sort_keys=True))
                        for row in web["names"]}
        self.assertEqual(len(unique_names), len(web["names"]))
        self.assertTrue(all(row["id"] in web["cards"] and row["titles"].get("en") for row in web["names"]))
        canonical = {card["id"]: card for card in self.data["catalog"]["cards"]}
        self.assertEqual(sum(card["kingdom"] for card in web["cards"].values()), 498)
        for card_id, meta in web["cards"].items():
            self.assertEqual(meta["title"], canonical[card_id]["text"]["de"][0])
            self.assertEqual(meta["kingdom"], canonical[card_id]["kingdomPile"])
            self.assertEqual(meta["firstEdition"], canonical[card_id]["edition"] == "first")
        self.assertTrue(web["cards"]["sauna"]["kingdom"])
        self.assertTrue(web["cards"]["way_of_the_mouse"]["landscape"])
        self.assertFalse(web["cards"]["cursed"]["landscape"])
        self.assertFalse(web["cards"]["approaching_army"]["landscape"])
        self.assertFalse(web["cards"]["the_earths_gift"]["landscape"])

    def test_embedded_web_names_match_source(self):
        html = (ROOT / "Dominion-Generator.html").read_text(encoding="utf-8")
        match = re.search(r'id="web-card-names">([\s\S]*?)</script>', html)
        self.assertIsNotNone(match, "the standalone generator must embed its conversion names")
        self.assertEqual(json.loads(match[1]), load_web_name_catalog(self.data["catalog"]))


if __name__ == "__main__":
    unittest.main()
