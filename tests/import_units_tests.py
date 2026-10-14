#!/usr/bin/env python3
"""Prevent conditional/source comments from corrupting shipped base envelopes."""
import importlib.util
import pathlib
import unittest

spec = importlib.util.spec_from_file_location('import_units', pathlib.Path(__file__).parents[1] / 'tools/import_units.py')
import_units = importlib.util.module_from_spec(spec)
spec.loader.exec_module(import_units)


class ImportUnitTests(unittest.TestCase):
    def source(self):
        return '\n'.join('[MISSILE] "%s"\nRange %d\nImprovedBy "Upgrade" Set Range 9999\n// Range 8888\n/* Range 7777 */' % (name, n + 100)
                         for n, name in enumerate(import_units.MAPPINGS.values()))

    def test_base_directives_ignore_upgrades_and_comments(self):
        entries = import_units.extract(self.source())
        self.assertEqual([x['rangeKm'] for x in entries], list(range(100, 105)))
        self.assertTrue(all(x['unresolvedModifierLines'] == 1 for x in entries))

    def test_duplicate_missile_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Ambiguous duplicate'):
            import_units.extract(self.source() + '\n[MISSILE] "Ship Gun"\nRange 22\n')

    def test_missing_unconditional_range_rejected(self):
        with self.assertRaisesRegex(ValueError, 'base Range'):
            import_units.extract(self.source().replace('Range 100\n', '', 1))

    def test_conflicting_base_ranges_rejected(self):
        with self.assertRaisesRegex(ValueError, 'base Range'):
            import_units.extract(self.source().replace('Range 100\n', 'Range 100\nRange 200\n', 1))


if __name__ == '__main__':
    unittest.main()
