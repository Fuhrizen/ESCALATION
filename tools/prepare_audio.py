#!/usr/bin/env python3
"""Export original sound cues for listening/editing; runtime synthesizes the same cues.
No uploaded recording or voice clip is redistributed. Python standard library only.
"""
import math
import pathlib
import struct
import wave

RATE = 24000
CUES = [('select', .075, 740), ('order', .12, 480), ('success', .23, 660),
        ('invalid', .14, 180), ('complete', .42, 540), ('launch', .44, 130),
        ('impact', .5, 75), ('click', .035, 1100)]

def prepare(directory):
    directory.mkdir(parents=True, exist_ok=True)
    for cue, (name, duration, frequency) in enumerate(CUES):
        random = 0x87312345 + cue
        pcm = bytearray()
        for i in range(int(RATE * duration)):
            t = i / RATE
            u = t / duration
            random = (1664525 * random + 1013904223) & 0xffffffff
            noise = random / 4294967295 * 2 - 1
            envelope = min(1, t / .006) * (1 - u) ** 2
            f = frequency
            if name in ('success', 'complete'):
                f *= (1, 1.25, 1.5)[min(2, int(u * 3))]
            signal = math.sin(2 * math.pi * f * t)
            if name == 'launch':
                signal = .65 * noise + .35 * math.sin(2 * math.pi * (130*t + 450*t*t))
            if name == 'impact':
                signal = .65 * noise + .35 * math.sin(2 * math.pi * 75*t)
            pcm += struct.pack('<h', int(signal * envelope * .22 * 32767))
        with wave.open(str(directory / (name + '.wav')), 'wb') as output:
            output.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
            output.writeframes(pcm)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=pathlib.Path, nargs='?',
                        default=pathlib.Path(__file__).resolve().parents[1] / 'data' / 'audio')
    prepare(parser.parse_args().output)
