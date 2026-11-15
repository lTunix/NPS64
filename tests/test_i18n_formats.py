"""Translations must retain printf arguments and fit the ASCII UI font."""
import json
import re
from pathlib import Path
rows=[json.loads('['+line.strip()[1:-2]+']') for line in Path('src/i18n_strings.inc').read_text().splitlines() if line.startswith('{')]
formats=re.compile(r'%(?:[-+ #0]*\d*(?:\.\d+)?(?:hh|ll|[hljztL])?[diuoxXfFeEgGaAcspn]|%)')
seen=set()
assert rows
for spanish,english in rows:
    assert spanish not in seen,spanish
    seen.add(spanish)
    assert formats.findall(spanish)==formats.findall(english),(spanish,english)
    assert english.isascii(),english
print(f'PASS: {len(rows)} translations preserve printf arguments, unique keys and ASCII font compatibility')
