"""Create deterministic 24-bit BMP fixtures for the manual MFC regression.

Usage: python scripts/Create-MfcFixtures.py build-mfc-fixtures
No SDK or third-party Python dependency; refuses to overwrite existing files.
"""
import argparse
import json
from pathlib import Path
import struct


def WriteBmp(path, rectangles):
	width = height = 64
	stride = (width * 3 + 3) & ~3
	pixels = bytearray([20] * (stride * height))
	for x, y, w, h in rectangles:
		for row in range(y, y + h):
			for col in range(x, x + w):
				offset = (height - 1 - row) * stride + col * 3
				pixels[offset:offset + 3] = bytes([200] * 3)
	header = struct.pack('<2sIHHI', b'BM', 54 + len(pixels), 0, 0, 54)
	header += struct.pack('<IiiHHIIiiII', 40, width, height, 1, 24, 0, len(pixels), 0, 0, 0, 0)
	with path.open('xb') as output:
		output.write(header + pixels)


def Main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument('directory', type=Path)
	args = parser.parse_args()
	args.directory.mkdir(parents=True, exist_ok=True)
	for name in ('ui-grouping.bmp', 'ui-blank.bmp', 'ui-invalid.bmp', 'expected.json'):
		if (args.directory / name).exists():
			parser.error('Refusing to overwrite ' + name)
	WriteBmp(args.directory / 'ui-grouping.bmp', [(30, 10, 3, 10), (50, 10, 3, 10), (30, 25, 3, 10), (30, 40, 3, 10)])
	WriteBmp(args.directory / 'ui-blank.bmp', [])
	with (args.directory / 'ui-invalid.bmp').open('xb') as output:
		output.write(b'not a BMP')
	expected = {
		'settings': {'contrast': 10, 'minArea': 12, 'gap': 12, 'across': 4},
		'grouping': {'blobs': 4, 'groups': 2, 'merged': {'parts': 3, 'area': 90, 'bbox': [30, 10, 3, 40], 'mean': 200, 'stddev': 0}},
		'gapZero': {'blobs': 4, 'groups': 4, 'eachArea': 30},
		'blank': {'blobs': 0, 'groups': 0},
		'invalid': 'Cannot load BMP preview.',
	}
	with (args.directory / 'expected.json').open('x', encoding='utf-8') as output:
		json.dump(expected, output, indent=2)
	print(args.directory.resolve())


if __name__ == '__main__':
	Main()
