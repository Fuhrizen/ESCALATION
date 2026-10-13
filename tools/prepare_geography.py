#!/usr/bin/env python3
"""Bundle Natural Earth v5.1.2 cities and strategic roads without runtime downloads.

Source checkout:
  git clone --depth 1 --branch v5.1.2 --filter=blob:none --sparse \
    https://github.com/nvkelso/natural-earth-vector.git natural-earth
  git -C natural-earth sparse-checkout set geojson
  python tools/prepare_geography.py natural-earth/geojson

Population estimates are modern reference data, not historical censuses. City
coordinates/names are unchanged. Road lines retain endpoints and are simplified
with a 0.02-degree Douglas-Peucker tolerance (roughly 2 km at the equator).
"""
import argparse
from collections import defaultdict
import hashlib
import json
import math
from pathlib import Path

VERSION = '5.1.2'
COMMIT = 'f1890d9f152c896d250a77557a5751a93d494776'
SOURCE_HASHES = {}
PINNED_HASHES = {
    'ne_10m_populated_places.geojson': '9b8e3de09048ef00dfc70357dbb9fa324493f214b5e0ae4daf1aa79a8d10116b',
    'ne_10m_roads.geojson': '66a0c7b438e92fd124822cc5921cfa11042f48c294ade5e0f03f2c6640fd0248',
}
# The 110m admin-0 map retains the older Natural Earth South Sudan code.
COUNTRY_ALIASES = {'SSD': 'SDS'}


def load(path):
    raw = path.read_bytes()
    SOURCE_HASHES[path.name] = hashlib.sha256(raw).hexdigest()
    if SOURCE_HASHES[path.name] != PINNED_HASHES[path.name]:
        raise ValueError(f'{path}: source does not match pinned Natural Earth v{VERSION}')
    return json.loads(raw)


def distance(a, b):
    dx = math.remainder(a[0] - b[0], 360) * math.cos(math.radians((a[1] + b[1]) / 2))
    return math.hypot(dx, a[1] - b[1])


def simplify(points, tolerance=0.02):
    # Iterative Douglas-Peucker avoids recursion limits on long source roads.
    if len(points) < 3:
        return points
    keep = {0, len(points) - 1}
    stack = [(0, len(points) - 1)]
    while stack:
        first, last = stack.pop()
        a, b = points[first], points[last]
        dx, dy = b[0] - a[0], b[1] - a[1]
        length2 = dx * dx + dy * dy
        best, index = tolerance * tolerance, -1
        for i in range(first + 1, last):
            p = points[i]
            t = max(0, min(1, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / length2)) if length2 else 0
            d2 = (p[0] - a[0] - t * dx) ** 2 + (p[1] - a[1] - t * dy) ** 2
            if d2 > best:
                best, index = d2, i
        if index >= 0:
            keep.add(index)
            stack.extend(((first, index), (index, last)))
    return [points[i] for i in sorted(keep)]


def write(path, obj):
    text = json.dumps(obj, ensure_ascii=False, separators=(',', ':')) + '\n'
    path.write_text(text, encoding='utf-8')
    return hashlib.sha256(text.encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, help='Natural Earth v5.1.2 geojson directory')
    parser.add_argument('--world', type=Path, default=Path('data/world.geojson'))
    parser.add_argument('--output', type=Path, default=Path('data'))
    args = parser.parse_args()
    world_raw = args.world.read_bytes()
    world = json.loads(world_raw)
    populations = {f['properties']['code']: f['properties']['population'] for f in world['features']}
    places = load(args.source / 'ne_10m_populated_places.geojson')
    grouped = defaultdict(list)
    for feature in places['features']:
        p = feature['properties']
        code = COUNTRY_ALIASES.get(p['ADM0_A3'], p['ADM0_A3'])
        if code not in populations:
            continue
        lon, lat = feature['geometry']['coordinates']
        name = p['NAME_EN'] or p['NAME']
        population = max(0, p['POP_MAX'] or 0)
        grouped[code].append({'name': name, 'lon': lon, 'lat': lat,
                              'population': population, 'country': code,
                              'admin': p['ADM1NAME'] or '', 'capital': bool(p['ADM0CAP'])})
    cities = []
    for code, candidates in sorted(grouped.items()):
        # Larger populations receive more objectives; spaced admin-1 seats first
        # maintain geographic coverage without filling remote areas artificially.
        budget = min(192, max(24, round(24 + 8 * math.sqrt(max(0, populations[code]) / 1_000_000))))
        ranked = sorted(candidates, key=lambda c: (-c['capital'], -c['population'], c['name']))
        seats = {}
        for c in ranked:
            seats.setdefault(c['admin'], c)
        primary = sorted(seats.values(), key=lambda c: (-c['capital'], -c['population'], c['name']))
        selected, identities = [], set()
        for c in primary + ranked:
            key = (c['name'], c['lon'], c['lat'])
            if len(selected) >= budget:
                break
            if key in identities:
                continue
            # About 28 km at the equator; major capitals always survive.
            if not c['capital'] and any(distance((c['lon'], c['lat']), (s['lon'], s['lat'])) < 0.25 for s in selected):
                continue
            selected.append(c)
            identities.add(key)
        cities.extend({k: c[k] for k in ('name', 'lon', 'lat', 'population', 'country')} for c in selected)
    source_roads = load(args.source / 'ne_10m_roads.geojson')
    roads = []
    for feature in source_roads['features']:
        p, geometry = feature['properties'], feature['geometry']
        if p['scalerank'] > 7 or p['ignore'] or p['type'] in ('Ferry Route', 'Ferry, seasonal', 'Track'):
            continue
        lines = [geometry['coordinates']] if geometry['type'] == 'LineString' else geometry['coordinates']
        for line in lines:
            points = [[round(x, 5), round(y, 5)] for x, y in simplify(line)]
            points = [p for i, p in enumerate(points) if i == 0 or p != points[i - 1]]
            if len(points) >= 2:
                roads.append(points)
    args.output.mkdir(parents=True, exist_ok=True)
    meta = {'source': 'Natural Earth', 'version': VERSION, 'license': 'Public domain'}
    hashes = {
        'settlements.json': write(args.output / 'settlements.json', {**meta, 'cities': cities}),
        'roads.json': write(args.output / 'roads.json', {**meta, 'roads': roads}),
    }
    manifest = {
        **meta, 'source_commit': COMMIT,
        'source_base': f'https://github.com/nvkelso/natural-earth-vector/tree/v{VERSION}/geojson',
        'source_sha256': SOURCE_HASHES, 'output_sha256': hashes,
        'world_sha256': hashlib.sha256(world_raw).hexdigest(),
        'country_aliases': COUNTRY_ALIASES,
        'cities': len(cities), 'roads': len(roads),
        'settlement_selection': 'Per country: min(192, round(24+8*sqrt(population/1e6))); admin-1 coverage, then population; 0.25-degree spacing; capitals preserved',
        'road_selection': 'scalerank <= 7, ignore == 0; exclude ferry and track; simplify 0.02 degrees; round coordinates to 5 decimal places',
        'limitations': 'Modern reference data; incomplete strategic road and town coverage. Empty regions must not invent city names.'
    }
    (args.output / 'geography.manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Prepared {len(cities)} real settlements and {len(roads)} strategic road lines')


if __name__ == '__main__':
    main()
