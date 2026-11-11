"""Check the release's catalog labels without generating assets or headers."""
from generate_effect_catalog import read_effects, verification_label, timing_mask, TIMING_FILTERS


def main():
    import json
    from pathlib import Path
    refs=json.loads((Path(__file__).resolve().parents[1]/'Documentation'/'Effect_Expansion_References.json').read_text(encoding='utf-8'))
    effects = read_effects()
    assert set(refs['effects'])=={row[0] for row in effects}
    assert refs['effects']['twist_beakers']==[['alchemy']]
    assert refs['effects']['twist_pennies_saved']==[['guilds']]
    assert refs['effects']['start_5_coffers']==[['guilds','renaissance']]
    assert len(effects) == 89
    by_id = {row[0]: row for row in effects}
    assert len(TIMING_FILTERS) == 13
    masks = {row[0]: timing_mask(row[3]) for row in effects}
    assert all(0 < mask < (1 << len(TIMING_FILTERS)) for mask in masks.values())
    assert all(any(mask & (1 << i) for mask in masks.values()) for i in range(len(TIMING_FILTERS)))
    assert masks["twist_big_ideas"] & (1 << 1)
    assert masks["start_double_pile"] & 1
    assert masks["stamp_durationified"] & (1 << 1) and masks["stamp_durationified"] & (1 << 10)
    assert masks["stamp_treasurified"] & (1 << 3)
    assert masks["stamp_reactionified"] & (1 << 6) and masks["stamp_reactionified"] & (1 << 7)
    assert not masks["stamp_reactionified"] & (1 << 9)  # Outside cleanup is not a cleanup trigger.
    assert not masks["start_replace_copper_from_pile"] & (1 << 4)  # Entnehmen is not Nehmen.
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
