"""Compare deterministic CPU frame hashes and optional emulator state traces."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("baseline")
parser.add_argument("optimized")
parser.add_argument("--baseline-run")
parser.add_argument("--optimized-run")
args = parser.parse_args()

def records(path, prefix):
    return [line for line in Path(path).read_text().splitlines() if line.startswith(prefix)]

before = records(args.baseline, "[cpu-image]")
after = records(args.optimized, "[cpu-image]")
assert before and before == after, "CPU image hashes differ or are absent"
print(f"PASS: {len(before)} CPU image hashes identical")
if args.baseline_run or args.optimized_run:
    assert args.baseline_run and args.optimized_run, "Provide both state traces"
    before = records(args.baseline_run, "STATE")
    after = records(args.optimized_run, "STATE")
    assert before and before == after, "CPU/VI state traces differ or are absent"
    print(f"PASS: {len(before)} CPU/VI states identical")
