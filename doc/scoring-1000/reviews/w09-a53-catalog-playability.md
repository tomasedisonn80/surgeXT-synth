# Wave 9, agent 53: catalog and musician-facing accuracy

## Result

All **1,000 current catalog entries** agree with their expanded recipes and
native preset metadata. The final check found **zero mismatches**. This review
also produced a [200-family browsing catalog](../CATALOG.md), covering every
preset ID and the family-specific role and variant exceptions.

Final manifest SHA-256:
`67bd4a5dc3fbf6f3794679b24b500f1ef37af83b7a47a863d10cb964a57314a2`.

The [machine-readable evidence](w09-a53-catalog-playability-evidence.json)
records the exact catalog, recipe, generated-document and generator hashes,
the reviewed guide snapshots, all referenced guide IDs, and the corrected
layer descriptions. This is a metadata and usability review; it is not a
listening pass or an assertion that a composer would choose a particular sound.

## Coverage

| Check | Result |
|---|---:|
| Catalog entries compared with recipe expansion | 1,000 / 1,000 |
| Native name, comment, play mode and 16-voice limit checks | 1,000 / 1,000 |
| Exact preset-file hashes checked against catalog | 1,000 / 1,000 |
| Entries with valid inclusive MIDI ranges of at least 24 semitones | 1,000 / 1,000 |
| Entries retaining the eight shared macro names | 1,000 / 1,000 |
| Polyphonic presets | 820 |
| Retriggering mono presets | 180 |
| Guide ID mentions resolved | 27, covering 23 distinct IDs |
| Complete ID/name mentions matched | 22, covering 20 distinct IDs |
| Family rows in the browsing catalog | 200 |
| Category sections | 10 |
| IDs represented by the family ranges | Exactly 1,000, no omissions or duplicates |

The comparison covers category, stable ID, family and variant name, role,
intent, suggested range, tags, amplitude settings, play mode and tempo-sync
flag. Native XML name/comment and mode checks establish that the musician-facing
catalog describes the actual delivered files. The checks use the expanded recipe
as the source of intent; they do not constitute a second independent synthesis
compiler.

The semantic reading covered the 200 family roles, 50 common variant intents,
and every family-specific override. Particular attention went to register
language, attack comparisons, mono phrasing, held-note patterns, pitch intervals,
Sparse and Ripple overrides, and transition behavior. The prior eight Edge
attack corrections are present in this final catalog: those variants no longer
promise a faster attack when the 4 ms minimum leaves the nominal setting
unchanged. Other clamped attack operations do not contradict their current
intent descriptions, which describe compactness, color or shorter decay/tails
without promising a faster attack.

## Ten layer descriptions corrected

The initial review found ten intents describing an **upper layer** while their
changed second oscillator sits one octave below the played register. Changes to
filter brightness can still brighten the whole voice, but they do not make the
second oscillator an upper-register layer. The parent applied narrow
family-specific overrides using **secondary layer** or **secondary-layer**.
The final comparison confirms all ten corrected intents are present in both the
catalog and native comments.

| Preset ID | Preset | Corrected description |
|---|---|---|
| SC05-006 | Cedar Section Close | Reduced secondary-layer blend |
| SC08-006 | Reed Lantern Near | Reduced secondary-layer weight |
| SC08-046 | Soft Voltage Near | Reduced secondary-layer weight |
| SC08-091 | Moonlit Triangle Near | Reduced secondary-layer weight |
| SC09-031 | Orbit Dots Soft | Reduced secondary layer |
| SC09-035 | Orbit Dots Glass | Stronger secondary layer |
| SC09-071 | Hollow Dispatch Soft | Reduced secondary layer |
| SC09-075 | Hollow Dispatch Glass | Stronger secondary layer |
| SC09-081 | Amber Ladder Soft | Reduced secondary layer |
| SC09-085 | Amber Ladder Glass | Stronger secondary layer |

No oscillator, envelope, effect or modulation edit was requested by this review.
The parent retained the measured sound and regenerated the descriptions, then
refreshed byte-bound evidence for the changed files. This review does not claim
to have conducted those separate native audio reruns.

## Sequence, phrase and range interpretation

The four explicitly pitched sequence families are mono and the guide correctly
instructs the player to supply one root at a time. Amber Ladder, Night Thirds,
Open Fifths and Prism Arcs use their declared characteristic intervals outside
Sparse; their Sparse roles describe the substituted root/octave accents. Copper
Transit and Tilted Grid also describe their substituted Sparse accent patterns
without preserving an inaccurate three-three-two or five-five-six promise.

Clockless Room and Deep Listening have explicit Ripple role exceptions that
describe regular synchronized movement. The catalog carries those exceptions
beside the family tables. The guide distinguishes these held-note modulation
patterns from a MIDI arpeggiator and describes Triplet relative to the family's
base rate. It does not promise a universal subdivision or a fixed bar length.

Mono lead and bass descriptions do not promise continuous-envelope legato. The
guide explains that Float entrances may retrigger and that native play mode can
be changed for another phrasing style. The suggested ranges remain starting
registers, not hard key restrictions or guarantees of equal loudness at every
note. Dawn Sweep and Falling Arc describe the return of their pitch envelopes
after note-off and distinguish amplitude Attack from the envelope driving pitch.

## Browsing catalog

[CATALOG.md](../CATALOG.md) presents ten category sections, each with a five-variant
order and 20 family rows. Every row supplies the complete ID range, family name,
inclusive MIDI range, play mode and main role. Family-specific role and intent
exceptions appear after the relevant table, so a family summary does not erase
the meaning of Sparse, Ripple, Swell or the corrected layer/attack cases.

The deterministic [catalog writer](../../../scripts/scoring/write_catalog.py)
uses the manifest as its input, checks the category/family/variant structure,
requires consecutive five-preset ID ranges, and records the input SHA-256. The
generated ranges were independently expanded and compared with the complete
1,000-ID set. The guide links and local catalog links resolve to existing files.
The writer changes only the requested Markdown document.

## Limits

Roles such as dialogue support, fantasy, suspense or pop accompaniment are
authored starting points. This review checks that their descriptions agree with
the delivered definitions and that examples are locatable. It does not establish
universal genre suitability, acoustic realism, perceived distinctness, musical
preference or Hollywood adoption. The catalog retains **musical audition:
pending** and **composer endorsement: not claimed** for every preset.
