"""Match approved HD texture brightness to historical originals, offline.

Only RGB values change. Pixel positions, dimensions and alpha remain exact.
Repeated runs always start from the approved uncalibrated PNGs.
"""
from pathlib import Path
from collections import deque
import argparse
import json
from PIL import Image, ImageChops, ImageStat

ROOT = Path(__file__).resolve().parent
WEIGHTS = (.2126, .7152, .0722)
INPUTS = {
    1971: ('grass-atlas.png', 'grass-original.png', 4, 1),
    1970: ('soil-atlas.png', 'soil-original.png', 4, 1),
    3242: ('grass-spot-hd.png', 'grass-spot-original.png', 1, 1),
    6286: ('grass1-spot-hd.png', 'grass1-spot-original.png', 1, 1),
    1906: ('grass-sprites-hd.png', 'grass-sprites-original.png', 4, 4),
}


def stored_luma(im, premultiply=False):
    r, g, b, a = im.convert('RGBA').split()
    if premultiply:
        r, g, b = [ImageChops.multiply(channel, a) for channel in (r, g, b)]
    mean = ImageStat.Stat(Image.merge('RGB', (r, g, b))).mean
    return sum(weight * value for weight, value in zip(WEIGHTS, mean))


def apply_gain(im, gain):
    table = [min(255, max(0, round(value * gain))) for value in range(256)]
    r, g, b, a = im.split()
    return Image.merge('RGBA', (r.point(table), g.point(table), b.point(table), a))


def apply_gain_with_ordered_rounding(im, gain):
    # A shared RGB lookup can change a large flat region by a whole level at
    # once. Sub-level rounding avoids that brightness jump while preserving
    # the same uniform gain, pixel layout and alpha. No source art is added.
    import numpy as np
    pixels = np.asarray(im).copy()
    bayer = np.array([[0, 2], [3, 1]], dtype=np.float32)
    for _ in range(2):
        bayer = np.block([[4*bayer, 4*bayer+2], [4*bayer+3, 4*bayer+1]])
    threshold = (bayer + .5) / 64
    height, width = pixels.shape[:2]
    threshold = np.tile(threshold, ((height+7)//8, (width+7)//8))[:height, :width]
    values = np.minimum(pixels[:, :, :3].astype(np.float32) * gain, 255)
    floors = np.floor(values)
    pixels[:, :, :3] = (floors + ((values-floors) > threshold[:, :, None])).astype(np.uint8)
    return Image.fromarray(pixels)


def pad_rgb_under_source_mask(im, source_alpha):
    """Extend RGB into transparent texels before restoring the native mask.

    This is texture edge padding, independently within each atlas cell.
    It does not move visible pixels or alter the generated PNG alpha.
    Without padding, an AI-cutout's empty RGB can punch black holes in
    thin source stems when the exact historical silhouette is restored.
    """
    width, height = im.size
    pixels = bytearray(im.tobytes())
    alpha = pixels[3::4]
    wanted = source_alpha.tobytes()
    # The image service can leave saturated RGB at almost-zero alpha.
    # Ordinary cutout materials store straight RGB, so those hidden colours
    # would leak into their BOX mipmaps after restoring the historical mask.
    # Clear only generator-empty texels that are also exactly source-empty.
    # An all-zero source alpha can belong to an alpha-ignoring RGB material.
    cleared = 0
    if any(wanted):
        for i, (a, ref) in enumerate(zip(alpha, wanted)):
            if a < 16 and ref == 0 and any(pixels[i * 4:i * 4 + 3]):
                pixels[i * 4:i * 4 + 3] = b'\x00\x00\x00'
                cleared += 1
    missing = [i for i, (a, ref) in enumerate(zip(alpha, wanted)) if a < 16 and ref]
    if not missing:
        result = Image.frombytes('RGBA', im.size, bytes(pixels)) if cleared else im
        return result, 0, cleared
    owner = [-1] * len(alpha)
    frontier = deque()
    for i, a in enumerate(alpha):
        if a >= 32:
            owner[i] = i
            frontier.append(i)
    if not frontier:
        raise ValueError('Generated atlas cell contains no usable foreground')
    while frontier:
        i = frontier.popleft()
        x = i % width
        neighbours = []
        if x: neighbours.append(i - 1)
        if x + 1 < width: neighbours.append(i + 1)
        if i >= width: neighbours.append(i - width)
        if i + width < len(alpha): neighbours.append(i + width)
        for j in neighbours:
            if owner[j] < 0:
                owner[j] = owner[i]
                frontier.append(j)
    for i in missing:
        start = owner[i] * 4
        pixels[i * 4:i * 4 + 3] = pixels[start:start + 3]
    result = Image.frombytes('RGBA', im.size, bytes(pixels))
    assert result.getchannel('A').tobytes() == im.getchannel('A').tobytes()
    return result, len(missing), cleared


def match_cell(im, target_luma, premultiply, alpha=None):
    # Find the gain after 8-bit quantization and native premultiplication,
    # rather than assuming floating-point RGB multiplication stays exact.
    low, high = 0., 8.
    # Very dark, thin plant sprites can need a larger gain once their
    # original premultiplied alpha is restored for native rendering.
    while high < 256:
        probe = apply_gain(im, high)
        if alpha is not None:
            probe.putalpha(alpha)
        if stored_luma(probe, premultiply) >= target_luma:
            break
        high *= 2
    best = None
    for _ in range(22):
        gain = (low + high) / 2
        corrected = apply_gain(im, gain)
        measurement = corrected.copy()
        if alpha is not None:
            measurement.putalpha(alpha)
        actual = stored_luma(measurement, premultiply)
        error = abs(actual - target_luma)
        if best is None or error < best[0]:
            best = error, gain, corrected, actual
        if actual < target_luma:
            low = gain
        else:
            high = gain
    rounding = 'Nearest 8-bit RGB lookup'
    if best[0] > .05:
        low, high = 0., max(8., high)
        for _ in range(22):
            gain = (low + high) / 2
            corrected = apply_gain_with_ordered_rounding(im, gain)
            measurement = corrected.copy()
            if alpha is not None:
                measurement.putalpha(alpha)
            actual = stored_luma(measurement, premultiply)
            error = abs(actual-target_luma)
            if error < best[0]:
                best = error, gain, corrected, actual
                rounding = '8x8 ordered sub-level RGB rounding'
            if actual < target_luma:
                low = gain
            else:
                high = gain
    return best[2], best[1], best[3], rounding


def main(ids=None):
    manifest = json.loads((ROOT / 'sources.json').read_text(encoding='utf-8'))
    reports = []
    for asset in manifest['textures']:
        if ids is not None and asset['id'] not in ids:
            if asset.get('brightness_calibration'):
                reports.append({'id':asset['id'], **asset['brightness_calibration']})
            continue
        if asset['id'] in INPUTS:
            filename, reference, columns, rows = INPUTS[asset['id']]
        else:
            filename = asset['uncalibrated_png']
            reference = asset['reference_png']
            columns, rows = asset.get('calibration_grid', [1, 1])
        image = Image.open(ROOT / filename).convert('RGBA')
        original = Image.open(ROOT / reference).convert('RGBA')
        assert image.width % columns == original.width % columns == 0
        assert image.height % rows == original.height % rows == 0
        premultiply = asset.get('alpha_encoding') == 'premultiplied'
        native_alpha = None
        if asset.get('native_alpha_reference'):
            native_alpha = Image.open(ROOT / asset['native_alpha_reference']).convert('RGBA').getchannel('A').resize(image.size, Image.Resampling.LANCZOS)
        def measure(im, alpha=None):
            measured = im.copy()
            if alpha is not None:
                measured.putalpha(alpha)
            return stored_luma(measured, premultiply)
        output = image.copy()
        cells = []
        for y in range(rows):
            for x in range(columns):
                dst = (x*image.width//columns, y*image.height//rows,
                       (x+1)*image.width//columns, (y+1)*image.height//rows)
                src = (x*original.width//columns, y*original.height//rows,
                       (x+1)*original.width//columns, (y+1)*original.height//rows)
                cell = image.crop(dst)
                # Historical native sprite PNG already has premultiplied RGB.
                target = stored_luma(original.crop(src))
                cell_alpha = native_alpha.crop(dst) if native_alpha is not None else None
                padded = cleared = 0
                if cell_alpha is not None:
                    cell, padded, cleared = pad_rgb_under_source_mask(cell, cell_alpha)
                corrected, gain, actual, rounding = match_cell(cell, target, premultiply, cell_alpha)
                output.paste(corrected, dst)
                cells.append({'cell': [x, y], 'rgb_gain': gain, 'quantization': rounding,
                              'rgb_padding_texels': padded,
                              'rgb_hidden_clear_texels': cleared,
                              'original_luma': target,
                              'before_luma': measure(cell, cell_alpha),
                              'corrected_luma': actual})
        assert output.size == image.size
        assert output.getchannel('A').tobytes() == image.getchannel('A').tobytes()
        calibrated = str(Path(filename).with_name(Path(filename).stem + '-balanced.png')).replace('\\', '/')
        output.save(ROOT / calibrated)
        asset['png'] = calibrated
        asset.pop('brightness_edit', None)
        calibration = {'method': 'Uniform RGB gain per original atlas cell, measured in native encoding',
                       'before_png': filename, 'reference_png': reference,
                       'grid': [columns, rows], 'alpha_unchanged': True, 'cells': cells,
                       'original_luma': stored_luma(original),
                       'before_luma': measure(image, native_alpha),
                       'corrected_luma': measure(output, native_alpha)}
        if native_alpha is not None:
            calibration['native_alpha'] = 'Original source mask resampled at HD density; generated alpha preserved in PNG intermediates'
        asset['brightness_calibration'] = calibration
        reports.append({'id': asset['id'], **calibration})
    manifest['name'] = 'HD world textures — brightness matched to originals'
    (ROOT / 'sources.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    (ROOT / 'brightness-report.json').write_text(json.dumps(reports, indent=2)+'\n')
    for report in reports:
        if ids is not None and report['id'] not in ids:
            continue
        print(f"{report['id']}: luma {report['before_luma']:.3f} -> {report['corrected_luma']:.3f}, "
              f"original {report['original_luma']:.3f}, alpha unchanged")


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ids',nargs='+',type=int,help='Calibrate only these newly generated assets')
    args=parser.parse_args()
    main(set(args.ids) if args.ids is not None else None)
