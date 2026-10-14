# Supplied research extracts

The local `.test_tmp/research-extracts.json` imports the user-supplied `Tech.txt` and `TechLevels.txt`. The catalog retains 694 technology definitions, localization tokens, image paths, categories, points, technological levels, automatic-unlock metadata and the original `LeadsTo` graph converted into incoming parents. Duplicate definitions are merged in source order and marked with `duplicateDefinitions`. Localization strings were not supplied; supported cards therefore use readable source keys rather than pretending to have the original translated descriptions.

35 technologies with implemented gameplay effects are mapped onto existing stable native IDs. Their names, illustration paths, research points and level contributions now come from the extract. Unimplemented aircraft, satellite, electronic warfare and weapon systems remain reference data and do not become selectable research cards. Existing native effect descriptions remain authoritative: mapping does not claim that an original technology's entire feature set has been implemented.

The imported settings use a starting level of 1, cap of 10, general multiplier of 0.8 and ahead-of-level multiplier of 1.5. Each completed mapped technology contributes its supplied `Add` amount. Cost is `Points × 20 native work × 0.8 × 1.5^(max(0, required level − current level))`. The factor of 20 converts the source's research points to this simulation's work clock; it is our tuning choice. Advanced research is costly, never blocked by the calendar or a level threshold. Unmapped native research retains the existing year-based cost premium and remains available once its prerequisites are complete. Queue costs are frozen at enqueue time.

Existing numeric IDs, native prerequisites and effect implementations are preserved for save compatibility. The full source graph is available in the local catalogue for subsequent system implementations; unsupported prerequisites are not silently considered complete. Replacing native prerequisite links with source links would invalidate already completed saves and requires an explicit migration rather than a cosmetic graph rewrite.

Regenerate with:

```sh
python tools/import_tech.py ../upload/Tech.txt ../upload/TechLevels.txt
```

`src/research_extracts.inc` is generated alongside the local JSON, so runtime builds do not depend on access to the uploads. The user supplied these extracts for this work. No new third-party license is asserted; original rights remain with their owners. The original input SHA-256 values are recorded in the catalog.

The complete 694-record reference catalogue is an ignored local development artifact and is not bundled or published. Only the 35 supported gameplay mappings are compiled into the published game.
