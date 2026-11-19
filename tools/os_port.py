#!/usr/bin/env python3
"""Find a profile's places of change and the firmware addresses the sources name in another firmware file.

A porting aid for moving the open-field base and packs from one OS version of the
OP-1 Field to another. It never writes firmware and never decrypts anything: it is
given two plain `os.ldr` files, each copied off a unit that runs that version, and
for every place the work depends on a fixed address it looks for the same code or
data in the other file by the bytes around it.

What it reads:
  * every place of change in the profile (`--profile`, by default
    the profile `build.json` names, `profiles/op1f-1.7.3.json`). A place's bytes are read from the source file. A
    profile that carries a checksum of them (`sum`) is held by it, one that carries
    the bytes themselves and their file offset (`before`, `file_offset`) by those:
    a source file that is not the profile's own is refused;
  * every firmware address named in the sources (`--module-dir`: `*.s`, `*.cpp`,
    `*.h`) and in the profile (0x08.., 0x11[89ab].., 0x84.. to 0x8f..): the
    functions, table words and data our code binds by a fixed address.

For each, it reports where it lands in the target and sorts the result into three
buckets, exactly the question a port has to answer:
  * `once`  - found at one clear place (a byte-identical anchor, or a single
              high-agreement window); a person still confirms a moved site;
  * `many`  - the anchor occurs at more than one place in the window (repeated
              code: a person must pick);
  * `none`  - no confident place; the site's own bytes changed too much, so the
              address or instruction has to be found again by hand.

It does NOT rewrite the profile or make a new base. A ported profile gets its
checksums from the target file (tools/place_sum.py in the public tree), and each
moved site is read again there. Its report holds short excerpts of the firmware's
bytes, so it is written only under `build/` or outside the repository, never into
a tracked folder.

Standard library only, plus this project's `ldr_inspect` for the file's records.
The source file is the one `--source` names, or the environment's OPEN_FIELD_STOCK.

    python -I tools/os_port.py --source <your 1.7.3 os.ldr> --target <the other os.ldr> --out build/port
    python -I tools/os_port.py --source <your 1.7.3 os.ldr> --self-check     # onto itself: every site must be found once, unmoved
"""
import argparse
import glob
import json
import os
import re
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from ldr_inspect import parse as parse_ldr  # noqa: E402

DEFAULT_PROFILE = ROOT / json.loads((ROOT / 'build.json').read_text(encoding='utf-8'))['profile']     # the tree's own profile
STOCK_VARIABLE = 'OPEN_FIELD_STOCK'     # the environment's name for one's own firmware file, as the public tree's tests take it
MODULE_GLOBS = ('module/*.s', 'module/*.cpp', 'module/*.h')

# Bytes kept either side of a site as its context, and how far around the expected
# place to look (code moves by tens to hundreds of bytes between releases, and whole
# regions can shift further, so the window is generous).
CONTEXT, WINDOW = 48, 48 * 1024
# A window match counts as confident when the context agrees this well and leads the
# next-best place by this margin; identical anchor bytes are accepted with less.
AGREE, MARGIN = 0.82, 0.10

# A Field address in code, data, L1 or L2: 0x08.., 0x11[89ab].., 0x84.. to 0x8f..,
# with or without 0x (the module's comments name some bare). A trailing/leading hex
# digit or word character disqualifies, so 0x8c0b116e matches but a longer run does not.
ADDRESS = re.compile(r'(?<![0-9a-fA-F\w])(?:0x)?((?:8[4-9a-f]|08|11[89ab])[0-9a-fA-F]{5,6})(?![0-9a-fA-F])')


class Image:
    """A plain os.ldr, with address <-> file-offset mapping over its stored records."""

    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        self.records = parse_ldr(self.data)
        # Stored (non-fill, non-empty) records only: these hold bytes to search.
        self.stored = [b for b in self.records if not b['fill'] and b['count']]

    def offset(self, address, size=1):
        """File offset of `size` stored bytes at `address`, or None (a fill, L1/RAM,
        unmapped, or straddling a record boundary)."""
        for b in self.stored:
            start = b['target']
            if start <= address and address + size <= start + b['count']:
                return b['offset'] + 16 + (address - start)
        return None

    def address(self, offset):
        """The load address of a stored file offset, or None."""
        for b in self.stored:
            lo = b['offset'] + 16
            if lo <= offset < lo + b['count']:
                return b['target'] + offset - lo
        return None


def absolute_reference(before):
    """If `before` is a Blackfin+ 8-byte instruction that carries an absolute address
    (a CALL.a/JUMP.a 0xDE00/0xDC00, or a register/memory immediate 0xD8xx-0xDBxx:
    high then low half-word after the opcode), the address it names; else None. A
    lead for the human: a moved site should reference the counterpart address."""
    if len(before) < 8:
        return None
    op, hi, lo, tail = struct.unpack_from('<4H', before, 0)
    if op in (0xde00, 0xdc00) and tail == 0:
        return hi << 16 | lo
    if 0xd800 <= op <= 0xdbff:
        return hi << 16 | lo
    return None


def occurrences(haystack, needle, lo, hi, cap=8):
    """File offsets of `needle` in haystack[lo:hi], up to `cap`."""
    found, at = [], lo
    while len(found) < cap:
        i = haystack.find(needle, at, hi)
        if i < 0:
            break
        found.append(i)
        at = i + 1
    return found


def agreement(a, b):
    """Fraction of equal bytes of two equal-length byte strings."""
    if not a:
        return 0.0
    return sum(x == y for x, y in zip(a, b)) / len(a)


def locate(source, target, offset, length, context=CONTEXT, window=WINDOW, expected=None):
    """Where the source bytes [offset, offset+length) with `context` on each side sit in
    the target. Returns (target_offset or None, agree, margin, count):
      * exact full-context match, exactly one in the window -> that place, agree 1.0,
        count 1 (handles an unmoved site and moved-but-byte-identical code at once);
      * exact full-context match more than once -> the one nearest `expected`, count>1
        (ambiguous, repeated code);
      * no exact context match -> anchor on the site's own `before` bytes, score each
        anchor's full context, take the best and its lead over the rest.
    `expected` (default: the same offset) centres the window and breaks ties."""
    lo_ctx = min(context, offset)
    pattern = source.data[offset - lo_ctx: offset + length + context]
    centre = (expected if expected is not None else offset)
    lo = max(0, centre - window)
    hi = min(len(target.data), centre + window + len(pattern))

    exact = occurrences(target.data, pattern, lo, hi, cap=8)
    if len(exact) == 1:
        return exact[0] + lo_ctx, 1.0, 1.0, 1
    if len(exact) > 1:
        best = min(exact, key=lambda p: abs(p + lo_ctx - centre))
        return best + lo_ctx, 1.0, 0.0, len(exact)

    # The full window did not match: an immediate or a branch displacement inside the
    # site or its neighbours moved. Anchor on the bytes around the site - the leading
    # context, the trailing context, and the replaced bytes themselves - so a site is
    # still pinned when its own bytes changed but its neighbours survived the move.
    before = source.data[offset: offset + length]
    leading = source.data[offset - lo_ctx: offset]
    trailing = source.data[offset + length: offset + length + context]
    sites = set()
    if len(before) >= 6:
        sites.update(occurrences(target.data, before, lo, hi, cap=64))
    if len(leading) >= 12:
        sites.update(h + len(leading) for h in occurrences(target.data, leading, lo, hi, cap=64))
    if len(trailing) >= 12:
        sites.update(h - length for h in occurrences(target.data, trailing, lo, hi, cap=64))
    scored = []
    for site in sites:
        cand = target.data[site - lo_ctx: site + length + context]
        if len(cand) == len(pattern) and site >= 0:
            scored.append((round(agreement(pattern, cand), 6), -abs(site - centre), site))
    if not scored:
        return None, 0.0, 0.0, 0
    scored.sort(reverse=True)
    best_agree, _, best_site = scored[0]
    others = max((s for s, _, _ in scored[1:]), default=0.0)
    return best_site, best_agree, best_agree - others, len(scored)


def bucket(count, agree, margin, identical):
    """`once` / `many` / `none` from a locate result."""
    if count == 0:
        return 'none'
    if count > 1 and not (identical and agree == 1.0 and margin >= MARGIN):
        return 'many'
    if identical or (agree >= AGREE and margin >= MARGIN):
        return 'once'
    return 'none'


def site_of(source, patch):
    """(file offset, the bytes there) of a profile's place in the source image. The bytes are the image's own. A
    profile that carries them (`before`, with `file_offset`) is held against them, and one that carries their
    CRC-32 (`sum`: a profile with no byte of the firmware in it) by that: an image that is not the profile's own is
    refused either way."""
    address, length = int(patch['address'], 16), int(patch['length'])
    offset = source.offset(address, length)
    wrong = offset is None
    if not wrong:
        held = source.data[offset: offset + length]
        wrong = (('file_offset' in patch and int(patch['file_offset'], 16) != offset)
                 or ('before' in patch and bytes.fromhex(patch['before']) != held)
                 or ('sum' in patch and zlib.crc32(held) & 0xffffffff != int(patch['sum'], 16)))
    if wrong:
        raise ValueError('source image is not this profile\'s stock OS at %s' % patch['address'])
    return offset, held


def port_sites(source, target, profile, anchors_out):
    """One row per profile patch. After the first pass, sites that did not land are
    searched again around the shift of the nearest patch that did (a region that moved
    further than the window is still reached)."""
    rows = [place_site(source, target, p) for p in profile['patches']]
    anchors = [(site_of(source, p)[0], r['delta']) for p, r in zip(profile['patches'], rows)
               if r['status'] == 'once' and r.get('delta') is not None]
    anchors_out.extend(anchors)
    if anchors:
        for i, (p, r) in enumerate(zip(profile['patches'], rows)):
            if r['status'] != 'once':
                off = site_of(source, p)[0]
                shift = min(anchors, key=lambda a: abs(a[0] - off))[1]
                again = place_site(source, target, p, expected=off + shift)
                if again['status'] == 'once':
                    rows[i] = again
    return rows


def place_site(source, target, patch, expected=None):
    length = int(patch['length'])
    offset, before = site_of(source, patch)
    found, agree, margin, count = locate(source, target, offset, length, expected=expected)
    row = dict(kind=patch.get('kind', 'place'), group=patch.get('group'), name=patch.get('name') or patch.get('symbol'),
               address=patch['address'], agree=round(agree, 3), margin=round(margin, 3), count=count)
    ref = absolute_reference(before)
    if ref is not None:
        row['references'] = hex(ref)
    if found is None:
        return dict(row, status='none', reason='no anchor for the replaced bytes in the window')
    identical = target.data[found: found + length] == before
    row.update(target_address=hex(target.address(found)), target_offset=hex(found),
               target_before=target.data[found: found + length].hex(),
               delta=found - offset, identical=identical)
    status = bucket(count, agree, margin, identical)
    row['status'] = status
    if status == 'many':
        row['reason'] = '%d equally good places in the window' % count
    elif status == 'none':
        row['reason'] = 'best place agrees %.0f%% (margin %.0f%%): the site\'s bytes changed' % (
            agree * 100, margin * 100)
    elif not identical:
        row['note'] = 'moved and the replaced bytes differ (an address or branch): read %s' % row['target_before']
    return row


def collect_addresses(profile, module_dir):
    """{address: sorted [where it is named]} over the module sources and the profile.
    The profile's own patch sites are left out (port_sites covers them)."""
    sites = {int(p['address'], 16) for p in profile['patches']}
    named = {}
    for pattern in MODULE_GLOBS:
        for f in sorted(glob.glob(str(Path(module_dir) / Path(pattern).name))):
            text = Path(f).read_text(encoding='utf-8', errors='replace')
            for m in ADDRESS.finditer(text):
                v = int(m.group(1), 16)
                if v & 0xffff:  # a round region base (low half 0) is a constant, not a site
                    named.setdefault(v, set()).add(Path(f).name)
    # Addresses the profile names beyond the patch sites (protected ranges, symbols).
    text = json.dumps(profile)
    for m in ADDRESS.finditer(text):
        v = int(m.group(1), 16)
        if v & 0xffff and v not in sites:
            named.setdefault(v, set()).add('profile')
    return {a: sorted(w) for a, w in named.items() if a not in sites}


def port_addresses(source, target, named, anchors):
    """One row per named stock address. A stored address is located by the 16 bytes at
    it (a function entry, a table row, a vtable); an address in L1/RAM/fill is not in
    the file, so it is reported as `carry` (the port must re-derive or confirm it by
    the code that loads it, which this tool leaves to the human)."""
    shift = lambda off: min(anchors, key=lambda a: abs(a[0] - off))[1] if anchors else 0
    rows = []
    for address in sorted(named):
        row = dict(address=hex(address), named_in=named[address])
        off = source.offset(address, 16)
        if off is None:
            rows.append(dict(row, status='carry', reason='not stored in the image (L1, RAM or a fill): re-derive by hand'))
            continue
        found, agree, margin, count = locate(source, target, off, 16, context=16, expected=off + shift(off))
        identical = found is not None and target.data[found: found + 16] == source.data[off: off + 16]
        status = bucket(count, agree, margin, identical)
        if found is None:
            rows.append(dict(row, status='none', agree=0.0,
                             reason='no anchor in the window: the bytes at it changed'))
        else:
            rows.append(dict(row, status=status, target_address=hex(target.address(found)),
                             delta=found - off, agree=round(agree, 3), margin=round(margin, 3), count=count,
                             **({'reason': '%d equally good places' % count} if status == 'many' else {})))
    return rows


def summary(rows):
    out = {'once': 0, 'many': 0, 'none': 0, 'carry': 0}
    for r in rows:
        out[r['status']] = out.get(r['status'], 0) + 1
    return out


def run(source_path, target_path, profile_path, module_dir):
    source, target = Image(source_path), Image(target_path)
    profile = json.loads(Path(profile_path).read_text(encoding='utf-8'))
    anchors = []
    sites = port_sites(source, target, profile, anchors)
    named = collect_addresses(profile, module_dir)
    addresses = port_addresses(source, target, named, anchors)
    return dict(source=str(source_path), target=str(target_path),
                source_bytes=len(source.data), target_bytes=len(target.data),
                sites=sites, site_summary=summary(sites),
                addresses=addresses, address_summary=summary(addresses))


def in_repo_but_not_build(path):
    p = path.resolve()
    try:
        rel = p.relative_to(ROOT)
    except ValueError:
        return False
    return rel.parts[:1] != ('build',)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--source', type=Path, default=os.environ.get(STOCK_VARIABLE) or None,
                        help='the os.ldr the profile was made for (1.7.3); without it, the file the environment\'s %s names' % STOCK_VARIABLE)
    parser.add_argument('--target', type=Path, help='stock os.ldr of the release to port to; omitted with --self-check')
    parser.add_argument('--profile', type=Path, default=DEFAULT_PROFILE)
    parser.add_argument('--module-dir', type=Path, default=ROOT / 'module')
    parser.add_argument('--out', type=Path, help='directory for the detailed JSON report (under build/ or outside the repo)')
    parser.add_argument('--self-check', action='store_true', help='port the source onto itself; exit nonzero if any site moved')
    args = parser.parse_args(argv)
    if args.source is None:
        parser.error('give --source, your own os.ldr of the version the profile is for (or set %s)' % STOCK_VARIABLE)
    args.source = Path(args.source)
    target = args.source if args.self_check else args.target
    if target is None:
        parser.error('give --target, or --self-check to port the source onto itself')
    report = run(args.source, target, args.profile, args.module_dir)

    s, a = report['site_summary'], report['address_summary']
    print('sites:     once %d, many %d, none %d, of %d' % (s['once'], s['many'], s['none'], len(report['sites'])))
    print('addresses: once %d, many %d, none %d, carry %d, of %d'
          % (a['once'], a['many'], a['none'], a['carry'], len(report['addresses'])))

    if args.self_check:
        # The property: pointed at the same image, the tool must pick each place as
        # itself (delta 0, byte-identical). A `many` bucket is fine when the place it
        # picks is still the original; a stored site that does not land is the failure.
        def strayed(r):
            if r['status'] == 'carry':
                return False
            if r['status'] == 'none':
                return True
            return r.get('delta') != 0 or r.get('identical') is False
        moved = [r for r in report['sites'] + report['addresses'] if strayed(r)]
        if moved:
            print('SELF-CHECK FAILED: %d site(s)/address(es) did not map to themselves' % len(moved))
            for r in moved[:20]:
                print('  ', r.get('address'), r['status'], r.get('reason', ''))
            return 1
        print('SELF-CHECK OK: every site and stored address is picked as itself, unmoved and byte-identical '
              '(%d carried: L1/RAM/fill)' % report['address_summary']['carry'])
        return 0

    if args.out:
        if in_repo_but_not_build(args.out):
            parser.error('the report holds stock-byte excerpts: write it under build/ or outside the repository')
        args.out.mkdir(parents=True, exist_ok=True)
        (args.out / 'port-report.json').write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
        lines = []
        for title, rows in (('sites', report['sites']), ('addresses', report['addresses'])):
            for want in ('many', 'none'):
                hit = [r for r in rows if r['status'] == want]
                if hit:
                    lines.append('')
                    lines.append('%s to review (%s):' % (title, want))
                    lines += ['  %s %s %s' % (r.get('address'), r.get('kind', ''), r.get('reason', '')) for r in hit]
        (args.out / 'port-review.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')
        print('report: %s' % args.out)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print('ERROR: %s' % error, file=sys.stderr)
        sys.exit(2)
