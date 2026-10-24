import csv
import math

def parse_hex(hex_str):
    """Parse hex string into list of int bytes (0-255)."""
    return [int(hex_str[i:i+2], 16) for i in range(0, len(hex_str), 2)]

def mean(xs):
    return sum(xs) / len(xs)

def stdev(xs):
    m = mean(xs)
    return math.sqrt(sum((v - m)**2 for v in xs) / max(len(xs)-1, 1))

def pearson_r(xs, ys):
    n = len(xs)
    mx = mean(xs)
    my = mean(ys)
    num = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    den = math.sqrt(sum((x - mx)**2 for x in xs)) * math.sqrt(sum((y - my)**2 for y in ys))
    return num / den if den != 0 else 0

# Read CSV manually since header doesn't match field count
# Real columns: label, uptime_s, step, ramp_target, ext_temp, int_temp, power_w, field, raw_hex
rows = []
with open('/mnt/c/temp/_code/tcl_ac_esphome/ramp_20260630_114119.csv') as f:
    # skip header
    f.readline()
    for line in f:
        parts = line.strip().split(',')
        if len(parts) >= 9:
            d = {
                'label': parts[0],
                'uptime_s': int(parts[1]),
                'step': int(parts[2]),
                'ramp_target': float(parts[3]),
                'ext_temp': float(parts[4]),
                'int_temp': float(parts[5]),
                'power_w': float(parts[6]),
                'field': parts[7],
                'raw_hex': parts[8].strip(),
            }
            d['bytes'] = parse_hex(d['raw_hex'])
            rows.append(d)

print(f"Loaded {len(rows)} rows")

# Verify hex lengths
from collections import Counter
hex_lens = Counter(len(r['bytes']) for r in rows)
print(f"Hex byte lengths: {dict(sorted(hex_lens.items()))}")

# Only use rows with 61 bytes
rows = [r for r in rows if len(r['bytes']) == 61]
print(f"Rows with 61 bytes: {len(rows)}")

n_bytes = 61

# --- Single byte correlations ---
print("\n" + "="*80)
print("SINGLE BYTE CORRELATIONS")
print("="*80)

def compute_single_correlations(var_name):
    vals = [r[var_name] for r in rows]
    results = []
    for pos in range(n_bytes):
        byte_vals = [r['bytes'][pos] for r in rows]
        if len(set(byte_vals)) < 2:
            continue
        r_val = pearson_r(byte_vals, vals)
        results.append((abs(r_val), r_val, pos, byte_vals))
    results.sort(key=lambda x: -x[0])
    return results[:10]

for label, var_key in [("ext_temp", "ext_temp"), ("power_w", "power_w"), ("int_temp", "int_temp")]:
    print(f"\n--- Top 10 bytes correlated with {label} ---")
    top = compute_single_correlations(var_key)
    for rank, (abs_r, r_val, pos, byte_vals) in enumerate(top, 1):
        unique_vals = sorted(set(byte_vals))
        val_range = f"{min(byte_vals)}-{max(byte_vals)}"
        sample = byte_vals[:5]
        print(f"  #{rank}: byte {pos:2d}  r={r_val:+.4f}  range={val_range}  uniq={len(unique_vals)}  samples={sample}")

# --- 16-bit little-endian correlations ---
print("\n" + "="*80)
print("16-BIT LITTLE-ENDIAN WORD CORRELATIONS")
print("="*80)

def compute_16bit_correlations(var_name):
    vals = [r[var_name] for r in rows]
    results = []
    for pos in range(n_bytes - 1):
        word_vals = [r['bytes'][pos] + (r['bytes'][pos+1] << 8) for r in rows]
        if len(set(word_vals)) < 2:
            continue
        r_val = pearson_r(word_vals, vals)
        results.append((abs(r_val), r_val, pos, word_vals))
    results.sort(key=lambda x: -x[0])
    return results[:10]

for label, var_key in [("ext_temp", "ext_temp"), ("power_w", "power_w"), ("int_temp", "int_temp")]:
    print(f"\n--- Top 10 16-bit LE words correlated with {label} ---")
    top = compute_16bit_correlations(var_key)
    for rank, (abs_r, r_val, pos, word_vals) in enumerate(top, 1):
        unique_vals = sorted(set(word_vals))
        val_range = f"{min(word_vals)}-{max(word_vals)}"
        sample = word_vals[:5]
        print(f"  #{rank}: bytes {pos:2d}-{pos+1:2d}  r={r_val:+.4f}  range={val_range}  uniq={len(unique_vals)}  samples={sample}")

# --- Quick scatter of best correlated bytes ---
print("\n" + "="*80)
print("DETAILED LOOK AT TOP CORRELATED BYTES")
print("="*80)

# For power, find top single byte
for var_key, var_name in [("ext_temp", "ext_temp"), ("power_w", "power_w"), ("int_temp", "int_temp")]:
    vals = [r[var_key] for r in rows]
    best = None
    for pos in range(n_bytes):
        byte_vals = [r['bytes'][pos] for r in rows]
        if len(set(byte_vals)) < 2:
            continue
        r_val = pearson_r(byte_vals, vals)
        if best is None or abs(r_val) > best[0]:
            best = (abs(r_val), r_val, pos)
    print(f"\nBest byte for {var_name}: byte {best[2]}  r={best[1]:+.4f}")
    # Show mapping
    pos = best[2]
    byte_vals = [r['bytes'][pos] for r in rows]
    unique_vals = sorted(set(byte_vals))
    print(f"  Unique values at byte {pos}: {unique_vals[:30]}{'...' if len(unique_vals)>30 else ''}")
    for v in unique_vals:
        matching = [r for r in rows if r['bytes'][pos] == v]
        pows = [r['power_w'] for r in matching]
        exts = [r['ext_temp'] for r in matching]
        ints = [r['int_temp'] for r in matching]
        print(f"    0x{v:02x} ({v:3d}): power_w={mean(pows):.1f}+-{stdev(pows):.1f}  ext_temp={mean(exts):.1f}  int_temp={mean(ints):.1f}  count={len(matching)}")

# --- Focus on specific byte positions of interest ---
print("\n" + "="*80)
print("SPECIFIC BYTES OF INTEREST (29, 34-35, 36, 38, 46-47)")
print("="*80)

interest = [29, 34, 35, 36, 38, 46, 47]
for pos in interest:
    byte_vals = [r['bytes'][pos] for r in rows]
    unique_vals = sorted(set(byte_vals))
    ext_r = pearson_r(byte_vals, [r['ext_temp'] for r in rows])
    pow_r = pearson_r(byte_vals, [r['power_w'] for r in rows])
    int_r = pearson_r(byte_vals, [r['int_temp'] for r in rows])
    print(f"\n  byte {pos}: ext_r={ext_r:+.4f}  pow_r={pow_r:+.4f}  int_r={int_r:+.4f}")
    print(f"  range={min(byte_vals)}-{max(byte_vals)}  uniq_vals={unique_vals[:30]}{'...' if len(unique_vals)>30 else ''}")
    if len(unique_vals) <= 12:
        for v in unique_vals:
            matching = [r for r in rows if r['bytes'][pos] == v]
            pows = [r['power_w'] for r in matching]
            print(f"    0x{v:02x} ({v:3d}): power_w={mean(pows):.1f}+-{stdev(pows):.1f}  count={len(matching)}")

# --- Combined bytes 34-35 as 16-bit LE vs power ---
print("\n" + "="*80)
print("COMBINED BYTES 34-35 (16-bit LE)")
print("="*80)
word_vals = [(r['bytes'][34] + (r['bytes'][35] << 8)) for r in rows]
ext_r = pearson_r(word_vals, [r['ext_temp'] for r in rows])
pow_r = pearson_r(word_vals, [r['power_w'] for r in rows])
int_r = pearson_r(word_vals, [r['int_temp'] for r in rows])
print(f"  ext_r={ext_r:+.4f}  pow_r={pow_r:+.4f}  int_r={int_r:+.4f}")
unique_vals = sorted(set(word_vals))
print(f"  Range: {min(word_vals)}-{max(word_vals)}, unique={len(unique_vals)}")
for v in unique_vals[:20]:
    matching = [r for r in rows if (r['bytes'][34] + (r['bytes'][35] << 8)) == v]
    pows = [r['power_w'] for r in matching]
    print(f"    0x{v:04x} ({v:5d}): power_w={mean(pows):.1f}+-{stdev(pows):.1f}  count={len(matching)}")
if len(unique_vals) > 20:
    print(f"    ... and {len(unique_vals)-20} more values")

# --- Combined bytes 46-47 as 16-bit LE ---
print("\n" + "="*80)
print("COMBINED BYTES 46-47 (16-bit LE)")
print("="*80)
word_vals2 = [(r['bytes'][46] + (r['bytes'][47] << 8)) for r in rows]
ext_r2 = pearson_r(word_vals2, [r['ext_temp'] for r in rows])
pow_r2 = pearson_r(word_vals2, [r['power_w'] for r in rows])
int_r2 = pearson_r(word_vals2, [r['int_temp'] for r in rows])
print(f"  ext_r={ext_r2:+.4f}  pow_r={pow_r2:+.4f}  int_r={int_r2:+.4f}")
unique_vals2 = sorted(set(word_vals2))
print(f"  Range: {min(word_vals2)}-{max(word_vals2)}, unique={len(unique_vals2)}")
for v in unique_vals2[:15]:
    matching = [r for r in rows if (r['bytes'][46] + (r['bytes'][47] << 8)) == v]
    pows = [r['power_w'] for r in matching]
    print(f"    0x{v:04x} ({v:5d}): power_w={mean(pows):.1f}+-{stdev(pows):.1f}  count={len(matching)}")
if len(unique_vals2) > 15:
    print(f"    ... and {len(unique_vals2)-15} more values")

# --- Also check byte 36-37 as 16-bit LE ---
print("\n" + "="*80)
print("COMBINED BYTES 36-37 (16-bit LE)")
print("="*80)
word_vals3 = [(r['bytes'][36] + (r['bytes'][37] << 8)) for r in rows]
ext_r3 = pearson_r(word_vals3, [r['ext_temp'] for r in rows])
pow_r3 = pearson_r(word_vals3, [r['power_w'] for r in rows])
int_r3 = pearson_r(word_vals3, [r['int_temp'] for r in rows])
print(f"  ext_r={ext_r3:+.4f}  pow_r={pow_r3:+.4f}  int_r={int_r3:+.4f}")

# --- Check byte 29 behavior ---
print("\n" + "="*80)
print("BYTE 29 DETAILS")
print("="*80)
byte_vals = [r['bytes'][29] for r in rows]
unique_vals = sorted(set(byte_vals))
print(f"  Unique values: {unique_vals}")
for v in unique_vals:
    matching = [r for r in rows if r['bytes'][29] == v]
    pows = [r['power_w'] for r in matching]
    exts = [r['ext_temp'] for r in matching]
    step = [r['step'] for r in matching]
    print(f"    0x{v:02x} ({v:3d}): power_w={mean(pows):.1f}+-{stdev(pows):.1f}  ext_temp={mean(exts):.1f}  step_range={min(step)}-{max(step)}  count={len(matching)}")
