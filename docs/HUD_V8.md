# Native HUD reference pass

The supplied ICBM: Escalation screenshots establish the new layout. Gameplay uses a map-first presentation, transparent upper information, a lower-left selection card, a lower-right simulation dial, adjacent navigation controls, and a narrow ready inventory rail. The former two-card lower-right inspector and large circular lower-left navigation are replaced.

All graphics are original native ImGui vectors. Unit symbols remain NATO-style counters; no proprietary game screenshots, portraits, unit renders, logos or icons are bundled.

## Gameplay

- A 44 logical-pixel transparent header contains menu, nation, full date and the blue/orange research/construction allocation slider.
- The selected object occupies a single lower-left card, 360 by at most 410 logical pixels. Its NATO symbol, health, weapon/ammunition tiles, range toggle, arrival estimate and command buttons remain in an expected location. Hover a weapon tile for target classes, damage, range and behaviour; expand the capability section for complete statistics.
- Silos expose conventional and nuclear point-targeting. Nuclear targeting requires an explicit warning confirmation before choosing a map target. The simulation still validates each requested launch.
- The lower-right segmented circular control pauses/resumes; adjacent plus/minus buttons adjust simulation speed. Four small hexagonal buttons open research, production, forces and log.
- Completed inventory uses a slim hexagonal icon rail with quantity badges and names on hover. Empty inventory has no full-height border.
- A subtle footer displays the full date. The tactical map remains visible behind common HUD elements.

## Research

The dark translucent research page retains the prerequisite DAG and hides locked technologies until prerequisites complete. A category column, compact icon cards, branching splines and a dedicated right-hand science/production queue column follow the reference's composition. Allocation remains accessible at the upper right. Selected technology details explain effects, duration, prerequisite status and queuing. Pan/zoom, fit, search, current research focus and explicit exit remain available.

## Input and scale

All gameplay anchors scale with the existing automatic/manual UI scale. The lower-left card and lower-right controls have matching map-input exclusion rectangles. Research and production remain management pages; the tactical camera cannot consume their clicks. The inspection card scrolls when a formation has many capabilities, rather than overflowing outside the window. Nuclear targeting is an explicit mode and can be cancelled with Escape.
