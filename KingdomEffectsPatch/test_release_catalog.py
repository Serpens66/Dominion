"""Check the release's catalog labels without generating assets or headers."""
from generate_effect_catalog import read_effects, verification_label


def main():
    effects = read_effects()
    assert len(effects) == 89
    by_id = {row[0]: row for row in effects}
    for effect_id in ("twist_mirror_loot", "stamp_two_for_one", "start_trash_curses"):
        assert effect_id in by_id
    for effect_id, name, description, timing, status, first, second in effects:
        assert all((effect_id, name, description, timing, status, first, second))
        label = verification_label(status)
        if "Einmal – Offen" in status:
            assert label == "Ungetestet", effect_id
    assert verification_label("Einmal – Ja\r\nMehrfach – Offen") == "Mehrfach ungetestet"
    assert verification_label("Einmal – Teilweise\r\nMehrfach – Offen") == "Teilweise geprüft"
    assert verification_label("Einmal – Zusatztyp bestätigt\r\nMehrfach – Gleicher Zusatztyp") == "Teilweise geprüft"
    assert verification_label("Einmal – Ja\r\nMehrfach – Bis 3×") == ""
    print(f"PASS: {len(effects)} effects, IDs, descriptions, timing and untested labels.")


if __name__ == "__main__":
    main()
