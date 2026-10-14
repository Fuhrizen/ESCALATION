# Terrain surface pass

The attached ICBM: Escalation screenshots informed the density and contrast of the terrain surface: a subdued satellite-like palette, recognisable mountain relief, darker woodland, and detail remaining visible beneath tactical overlays. ESCALATION uses original shader code and the already bundled public-domain Natural Earth relative elevation atlas. This is illustrative cartographic rendering, not newly acquired satellite imagery or measured land-cover data.

## Rendering

- Geographic elevation gradients provide stronger directional mountain shading.
- Earth-space noise adds darker canopy clusters, meadow colour variation, warmer arid surfaces, and fine rocky ridges inside elevated or sloping terrain.
- A filtered agricultural mosaic appears on flatter, temperate surfaces. Its cells are artistic surface detail rather than surveyed farmland.
- Each detail octave fades before its frequency becomes smaller than a screen pixel. Geographic textures stay fixed while panning and use the same coordinates in map and globe mode, including continuity at the longitude seam.
- The existing mipmapped elevation/coast atlas remains the geographic foundation. Terrain layers do not affect movement, damage, ownership, roads or rivers.

## Uploaded AssetLib inspection

`AssetLib.tar.gz` was inspected by enumerating archive members, without extracting executable content or writing archive paths. It contains 445 entries, including 188 PNGs, 15 JPEGs, 161 WAV files, 8 FBX files, commercial game branding, unit imagery, interface atlases, effects and water images. No redistribution license or README was found in the archive. Accordingly none of its image, model, sound or interface files were copied into this public repository. The supplied screenshots and inventory served only as design references.

All retained source attribution and public-domain terrain provenance remain in `data/assets.json`. Procedural surface features are original ESCALATION shader work and are deliberately identified as illustrative rather than geographic evidence.
