#!/usr/bin/env python3
"""Compile and exercise the dependency-free core with a local C++17 compiler."""
import argparse
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true', help='enable address/undefined sanitizers')
args = parser.parse_args()
build = root / 'build-verify'
build.mkdir(exist_ok=True)
(build / 'tmp').mkdir(exist_ok=True)
env = dict(os.environ, TMPDIR=str(build / 'tmp'))
compiler = os.environ.get('CXX', 'g++')
flags = ['-std=c++17', '-O1' if args.sanitize else '-O2', '-g']
if args.sanitize:
    flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-DESCALATION_SKIP_EXCEPTION_TESTS']
includes = ['-Iinclude', '-Ithird_party/picojson', '-Ithird_party/earcut', '-Ithird_party/stb']
for source, output in [('tests/core_tests.cpp', 'escalation_tests'), ('tests/camera_tests.cpp', 'escalation_camera_tests'), ('src/headless.cpp', 'escalation_headless')]:
    executable = build / (output + ('.exe' if os.name == 'nt' else ''))
    extra = ['src/renderer.cpp', 'src/assets.cpp'] if 'camera' in output else []
    if args.sanitize:
        extra += ['tests/sanitizer_options.cpp']
    subprocess.run([compiler, *flags, *includes, 'src/world.cpp', 'src/simulation.cpp', 'src/simulation_actions.cpp', source, *extra, '-o', str(executable)], cwd=root, check=True)
    parameters = [] if 'tests' in output else ['FRA', '49275']
    subprocess.run([str(executable), *parameters], cwd=root, env=env, check=True)
