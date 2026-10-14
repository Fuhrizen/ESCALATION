#!/usr/bin/env python3
"""Pack existing public-domain elevation and Natural Earth coast proximity.

Green records ocean distance to the existing country geometry, not bathymetry.
The native shader uses it for illustrative continental shelf colour only.
Run from the repository root; inputs and output are bundled, with no network.
"""
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
from scipy.ndimage import distance_transform_edt

root = Path(__file__).resolve().parents[1]
height = Image.open(root / 'data/terrain-height.png').convert('L')
w, h = height.size
mask = Image.new('L', (w, h))
draw = ImageDraw.Draw(mask)
for feature in json.loads((root / 'data/world.geojson').read_text())['features']:
    geometry = feature['geometry']
    polygons = geometry['coordinates'] if geometry['type'] == 'MultiPolygon' else [geometry['coordinates']]
    for polygon in polygons:
        for i, ring in enumerate(polygon):
            points = [((lon + 180) * w / 360, (90 - lat) * h / 180) for lon, lat in ring]
            draw.polygon(points, fill=255 if i == 0 else 0)
land = np.asarray(mask) > 0
# Three copies keep longitude periodic at the antimeridian.
distance = distance_transform_edt(~np.tile(land, (1, 3)))[:, w:2*w]
coast = np.uint8(np.clip(distance / 24, 0, 1) * 255)
Image.merge('RGBA', (height, Image.fromarray(coast), mask, Image.new('L', (w,h),255))).save(root / 'data/terrain-surface.png', optimize=True)
print('Packed terrain surface:', w, h)
