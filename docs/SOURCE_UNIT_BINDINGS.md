# Unit extract bindings

`tools/import_units.py` reads locally supplied unit extracts and emits a compact
manifest plus `src/source_weapon_ranges.inc`. Original archives and asset binaries
are not required at build or runtime and are not published. SHA-256 hashes in
`data/source-weapons.json` identify the inputs without copying their contents.

The importer accepts only one unconditional `Range` directive for each mapped
missile and rejects duplicates or missing values. It removes comments before
parsing; `ImprovedBy ... Set Range` never overwrites the base value.

| Runtime mount | Source missile | Base range (km) |
|---|---|---:|
| Naval gun | Ship Gun | 500 |
| Anti-ship missile | Anti Ship Missile | 550 |
| Land attack cruise missile | SSM | 650 |
| Submarine torpedo | Torpedo | 200 |
| Depth charges | DepthCharge | 50 |

Existing mount IDs and their ammunition state remain compatible with saves.
Independent target masks, research unlocks, minimum ranges and sonar detection
still apply. A 200 km torpedo envelope does not give a submarine 200 km sonar.
Surface ASW torpedoes retain the existing gameplay profile because associating
an aircraft torpedo definition with those mounts would require inference.

Damage, projectile speed, firing cadence, stocks, armour, unit power and movement
retain the simulation's existing strategic time and health scale. Source damage
can include explosion effects and target-specific modifiers, so copying a number
would not reproduce its behaviour. COPY/MODIFY, conditional equipment cases,
faction modifiers, tech upgrades, precision, interception and collateral damage
are unresolved; the manifest records modifier counts rather than claiming they
are implemented. Ground weapon profiles remain unchanged because an Army_Division
and a Small Arms covert weapon are not direct matches for our separate formation
types.

Reproduce the bindings with:

```sh
python tools/import_units.py /path/to/extracted/Units --root .
```
