# Supplied resources, interaction and optimization

## Resource use

Units.tar.gz and AssetLib.tar.gz contain text definitions with base fields, inheritance and research-dependent cases. Five unambiguous naval weapon ranges now drive gameplay; the reproducible importer and limitations are documented in SOURCE_UNIT_BINDINGS.md. Mount state/save IDs and target/sonar restrictions are retained. Full binary archives are not needed by the game.

Tech(1).txt and TechLevels(1).txt were re-imported into the existing stable-ID research mapping. The source still contains 694 records and 35 supported mappings; the generated supported data is unchanged. Tech_faction.txt defines only a faction dummy; it provides no translated names or actual faction gameplay values, so it does not create a selectable dummy node.

AssetLib's UI/UI.txt and UI_2048.png were inspected. Their distinct button pressed, disabled, active-host and frame hierarchy informed geometric hover/press/keyboard focus feedback, while preserving NATO map counters. The atlas images were not copied into the build. In AI/Default/DynamicDefence.txt, EnemyCheck/CriticalThreshold values describe force relationships, not a timer: we did not repurpose them as a think cadence.

## Interaction comfort

Research categories, custom navigation, ready items and play/pause now activate on release like ordinary buttons. Dragging off cancels activation; keyboard navigation shows a focus indication. Pressed controls show immediate contrast. Standard action-button outlines fade over a short frame-rate-independent interval without delaying click processing. Armed move/attack actions say to choose a destination/target, Stop clears armed commands, and actions provide feedback. Speed extrema are disabled and research cards expose full tooltips when zoomed out.

## Performance work and research

- Research graph topology and lanes are cached per field/catalogue size. Revealed-edge membership is O(1) instead of repeated list searches.
- Immutable road/river corner geometry is prepared once. Projected segments are reused while the camera is unchanged, with immediate invalidation on camera/viewport/projection changes. Dynamic routes and weapon range rings use the existing live path. Vertical as well as horizontal viewport culling skips offscreen segments.
- Defender response classifies incursions once per tick and uses exact nearest-neighbour queries on a unit-sphere k-d tree. Chord-squared distance has the same ordering as great-circle distance, including the longitude seam and poles. Rebuilding after movement prevents stale threat indices.

Primary references consulted:

- https://doc.cgal.org/latest/Spatial_searching/index.html (preprocessing/query separation, nearest-neighbour search and cheaper transformed distances; no new CGAL dependency)
- https://github.com/ocornut/imgui/wiki/Draw-List (draw-list geometry, clipping and batches)
- https://www.glfw.org/docs/3.4/input_guide.html (scroll/input handling)

## Measurements and limits

A Release benchmark of opposing, moving ground formations compares the previous combat implementation and the optimized implementation with the same other source/core objects. It measures whole simulation ticks, not isolated combat.

| Opposing formations | Previous | Optimized |
| --- | ---: | ---: |
| 100 | 3.216 ms/tick | 2.998 ms/tick |
| 500 | 28.672 ms/tick | 19.264 ms/tick |
| 1,000 | 91.251 ms/tick | 50.034 ms/tick |

At 1,000 formations both runs ended with 998 alive, 851 moving and 728 engagement records. This diagnostic shows approximately 45% less tick time in the invasion scenario. Friendly stationary timings did not improve reliably and are not used as evidence of this gain. The benchmark now labels both scenarios and CI records them. Dense battles, long-range targeting, routes and simultaneous world campaigns remain further profiling work, not proven scalability claims.

Narrow local checks passed 119 tactical checks, importer regressions and native input integration. Native input tests also compare cached/uncached geometry vertices and test immediate camera invalidation. GitHub Actions supplies the full Linux/Windows build, simulation and render checks.
