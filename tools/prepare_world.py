#!/usr/bin/env python3
"""Reduce an official Natural Earth v5.1.2 GeoJSON without altering its geometry."""
import argparse
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('source', type=Path, help='ne_110m_admin_0_countries.geojson from Natural Earth v5.1.2')
parser.add_argument('--output', type=Path, default=Path('data/world.geojson'))
args = parser.parse_args()
raw = args.source.read_bytes()
source = json.loads(raw)
features = []
for feature in source['features']:
    props = feature['properties']
    features.append({'type': 'Feature', 'properties': {
        'code': props['ADM0_A3'], 'name': props['ADMIN'],
        'population': props['POP_EST'], 'gdp': props['GDP_MD'],
        'label_lon': props['LABEL_X'], 'label_lat': props['LABEL_Y']
    }, 'geometry': feature['geometry']})
assert len(features) == 177, 'Expected the pinned 110m admin-0 dataset'
assert len({f['properties']['code'] for f in features}) == len(features)
output = json.dumps({'type': 'FeatureCollection', 'features': features}, ensure_ascii=False, separators=(',', ':')) + '\n'
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(output, encoding='utf-8')
manifest = {
    'source': 'https://github.com/nvkelso/natural-earth-vector/blob/v5.1.2/geojson/ne_110m_admin_0_countries.geojson',
    'version': '5.1.2', 'scale': '1:110m', 'license': 'Public domain',
    'source_sha256': hashlib.sha256(raw).hexdigest(),
    'output_sha256': hashlib.sha256(output.encode()).hexdigest(),
    'features': len(features), 'geometry_modified': False
}
args.output.with_suffix('.manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(f'Prepared {len(features)} map features in {len(output.encode())} bytes')
