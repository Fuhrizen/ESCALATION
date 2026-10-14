# Frontline performance diagnostic

Build the optional `escalation_frontline_benchmark` target in Release mode and run it from the repository root. It loads actual world geography, starts France's campaign, spreads 100, 500 and 1,000 friendly infantry formations across the campaign region centres, warms one tick, then measures 100 ticks at war. It reports milliseconds per simulation tick and deliberately has no machine-dependent timing assertion.

On the working Linux environment, `g++ -O2` produced:

| Formations | Original combat, new uncached territory update | Spatial combat and occupation grouping, uncached territory | Final event-driven territory update |
| --- | ---: | ---: | ---: |
| 100 | 2.92 ms | 1.26 ms | 0.46 ms |
| 500 | 14.48 ms | 5.36 ms | 2.59 ms |
| 1,000 | 32.62 ms | 10.71 ms | 6.61 ms |

The first two measurements used the same new territory implementation to isolate the combat/occupation improvements. The original combat source was extracted from the repository's starting HEAD. The final column adds change detection so stationary ground does not rebuild influence each tick. These are one-run diagnostics, not controlled statistical results.

This scenario stresses geographic ownership and empty-target searches with stationary friendly troops. It does not measure opposing army AI, moving formations, projectile-heavy fighting, GPU rendering, frame rates, or simultaneous wars between all countries. It therefore does not establish world-war scalability. Target selection uses conservative geographic buckets; long-range weapons and densely concentrated battles can still visit many formations. Defender response logic still compares opposing formations.

The UI ownership field rebuilds only after camera, region ownership, campaign identity or geographic ownership revision changes. It no longer schedules pressure reconstruction every 125 ms or scans troops per sampled pixel. Drawing cached front segments also avoids geographic polygon picking on each frame.

Captured ground is stored in sparse 0.1-degree geographic cells. Ownership persists after troops leave, across camera changes, and through saves. Ground formations must supply opposing local influence to retake a cell. Region settlement objectives continue to govern region-level infrastructure ownership. The cell spacing is approximately 11 km north/south and narrows east/west near the poles; this is a strategic approximation, not exact terrain boundaries.
