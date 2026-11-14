# W01 A02 — Musical usability and scoring research

Research date: 2026-11-13 (client date). This is a design research note, not evidence that any resulting preset has been used or endorsed by a professional composer.

## Finding

Professional scoring practice supports a broad palette of playable keys, plucks, pads, basses, pulses, processed acoustic textures and orchestral/electronic combinations. The strongest common requirement is **responsive musical control and suitability to the scene**. None of the consulted sources gives a representative industry-wide preset-usage survey, so there is no defensible numerical ranking of “most used film scoring sounds.” Product pages establish design examples, not market share or objective superiority.

Surge XT can contribute useful synthetic/hybrid instruments to this palette. A synthesis-only bank should label string, brass, choir, piano and acoustic-like patches as stylized layers or evocations. It should not promise a substitute for recorded orchestral articulation, realistic piano mechanics, sung language, or acoustic instrumental performance.

## Eleven primary sources

### 1. Alan Silvestri: hybrid workflow interview

URL: https://blog.native-instruments.com/alan-silvestri-breaks-down-the-composing-workflow-behind-his-blockbuster-scores/

Retrieved: web search result `turn235594560search4` contained the article text. Direct opening currently redirects to Native Instruments' music-production index; preserve this access limitation in final references if needed.

Evidence: Silvestri describes combining electronic material with a live orchestra and retaining MIDI flexibility for picture edits. He identifies electro-acoustic percussion and rhythmic patterns as useful parts of that workflow.

Design implication: include controlled percussion/pulses and supportive harmonic layers; reliable session recall and tempo editing matter as much as impressive isolated sound. This is a named practitioner's workflow, not a claim that every composer works this way.

### 2. Evan Hodges: Sequential artist interview

URL: https://sequential.com/evan-hodges/

Retrieved: `turn172633995view1`, especially lines 84–97.

Evidence: Hodges discusses the OB-6's retro and modern range, quick editing under post-production deadlines, usable presets and an arpeggiator/envelope/filter method for complex drum sequences. He uses analog synthesis to support drama and suspense.

Design implication: presets must have immediate musical defaults, a useful short list of performance controls, and clear rhythmic variants. Analog pads, leads, basses and compact sequences are reasonable components of the bank.

### 3. Kris Bowers: interview about identity and craft

URL: https://composer.spitfireaudio.com/en/articles/kris-bowers-reflects-on-identity-the-industry-and-the-importance-of-connection-with-his-craft

Retrieved: search `turn235594560search1`; direct open timed out.

Evidence: Bowers relates film scoring to shifting between soloing and accompaniment: some moments feature music while others serve dialogue. He stresses listening, rapid iteration and agility.

Design implication: an excellent bank must include restrained, thin and intimate options as well as foreground sounds. Provide brightness and density reduction that work musically, rather than assuming all patches need maximal width and reverb.

### 4. Chloe Flower: piano, composition and visual storytelling interview

URL: https://composer.spitfireaudio.com/en/articles/chloe-flower-on-her-love-for-pianos-the-transformative-power-of-composition

Retrieved: `turn172633995view4`, `turn257577248view4`.

Evidence: Flower starts at a keyboard while watching the picture, considers melodic placement, dialogue gaps, space and instrumentation, and then layers. She discusses both percussive and melodic uses of piano.

Design implication: dedicate meaningful space to playable, velocity-sensitive key and mallet sounds; neutral default harmony and clear note articulation help the composer write their own melody. Do not hard-code major/minor chords into every preset.

### 5. Alex Somers: Siblings and film-scoring interview

URL: https://composer.spitfireaudio.com/en/articles/alex-somers-on-sibling-songs

Retrieved: `turn110958067view2`, especially lines 10–15.

Evidence: Somers discusses processing material through amps, tape echoes and cassette machines to relate timbre to decayed archival images in Dawson City: Frozen Time.

Design implication: textural noise, drift, filtering and time effects can carry narrative character. Supply controllable degradation and restrained defaults so the same source can move from clean songwriting to distressed underscore. This illustrates a particular aesthetic, not general superiority of lo-fi sound.

### 6. Cornel Wilczek: hybrid sound-design interview

URL: https://www.native-instruments.com/blogs/blog/cornel-wilczek

Retrieved: `turn110958067view1`, `turn257577248view5`.

Evidence: Wilczek describes practical recall advantages of software effects, global modulation, printing affected audio for editing, and performing effects to picture. He also explicitly recognizes traditional, unprocessed scoring as valid.

Design implication: performance-ready movement/space controls, reliable saved states, useful dry versions and renderable tails are valuable. Avoid a universal “everything must be hybrid” claim.

### 7. Christopher Larkin: game, film and TV interview

URL: https://www.native-instruments.com/blogs/blog/christopher-larkin-on-scoring-for-game-film-and-tv

Retrieved: `turn172633995view0`, `turn257577248view1`.

Evidence: Larkin discusses thematic writing and planning complex orchestral pieces on piano, including recurrent character-associated motifs. This source spans games and film/TV, so its game-specific details must not be presented as film-industry statistics.

Design implication: playable lead and key patches with clear pitch and controlled onset support thematic writing. Supply one-note instruments that retain interval clarity and are useful in repeated motifs.

### 8. Spitfire Originals Media Toolkit: composer-curated product documentation

URL: https://www.spitfireaudio.com/products/originals-media-toolkit

Retrieved: `turn235594560view1`, lines 250–252 and 308–313.

Evidence: Paul Thomson's curated toolkit includes piano, marimba, guitar, analog pads, leads, bass and varied percussion. The controls include expression, dynamics and adjustable space. This is vendor product documentation, not proof of most-used status.

Design implication: balance ordinary building blocks with special textures. Separate broad timbre change from loudness control and make dry-to-spacious adjustment easy.

### 9. Spitfire Contemporary Drama Toolkit: performance design documentation

URL: https://www.spitfireaudio.com/products/contemporary-drama-toolkit

Retrieved: `turn257577248view2`, especially lines 350–378 and 545–550.

Evidence: Samuel Sim's toolkit organizes processed synths, guitars, electric strings and voices. Velocity reveals texture/soft/loud layers, with expression available as an additional control. The design supports playing with both hands.

Design implication: velocity must alter more than volume for many patches; map it to brightness, excitation or a complementary layer. Sustain sounds can additionally use wheel/aftertouch to shape a phrase. Do not copy patch names, recordings or factory presets from this commercial product.

### 10. Native Instruments LORES: official product documentation

URL: https://www.native-instruments.com/products/lores

Retrieved: `turn172633995view3`; overview search `turn866217215search2`.

Evidence: LORES is described as an organic-texture instrument developed around Clinton Shorter's workflow and instrument combinations. Its acoustic recordings are materially different from a synthesis-only bank.

Design implication: blend contrasting but complementary components (stable pitched body plus airy texture, transient plus sustain), while making the synthetic provenance clear. Do not claim to replicate its recorded instrument inventory.

### 11. Surge XT official manual

URL: https://surge-synthesizer.github.io/manual-xt/

Retrieved: `turn110958067view5`, `turn866217215view2`, `turn866217215view4`.

Evidence: the manual documents modulation sources including modwheel, expression, breath, velocity and MPE-related timbre, controller smoothing and eight macros. Its published manual says it covers XT 1.3, so source-code inspection of this repository remains authoritative for implementation details.

Design implication: use consistent macro semantics and documented controller routing. Test behavior with actual engine rendering rather than assuming parameter names or normalized values.

## Recommended 10-category balance (100 each)

This allocation is a design proposal, not an empirical usage distribution. Parent may combine it with other research.

| Category | Intended role and differentiated content |
|---|---|
| 01 Keys and Intimate Instruments | Soft FM keys, rounded tine sounds, muted resonators, woody attacks, synthetic felt-like keys, restrained sustained hybrids. Avoid realistic grand-piano claims. |
| 02 Plucks and Articulated Strings | Short subtractive plucks, physical-model-inspired resonators, muted synthetic guitar-like tones, pizzicato-like layers, metallic plucks. Keep tuning and release predictable. |
| 03 Bells and Mallets | Harmonic/inharmonic FM bells, wood-like mallets, glass, celesta-like glints, low tuned strikes. Bell ratios and envelope designs should vary meaningfully. |
| 04 Pads and Harmonic Beds | Warm analog, airy digital, slowly evolving, narrow intimate, broad luminous, dark reduced-band pads. Chord clarity at useful registers is essential. |
| 05 Hybrid Strings and Brass | Stylized string ensembles, reed-like sustain, breathy brass, soft swells, controlled bold stabs. Identify as synthetic support and blending layers. |
| 06 Vocal Air and Atmospheres | Vowel-like textures, breathy noise, filtered air, nonverbal choir-like layers, controlled drones. Distinguish pitched vs atonal and note useful register. |
| 07 Basses and Low Foundations | Clean sub, analog round, plucked bass, expressive growl, restrained pulsing low foundation. Mono compatibility and sensible low-frequency limits matter. |
| 08 Leads and Melodic Voices | Soft mono, lyrical vibrato, reed/flute-like, singing analog, brighter featured voices, polyphonic melody layers. Provide a useful pitch-bend range. |
| 09 Pulses and Sequences | Even and syncopated pulses, sparse ostinati, evolving filter sequences, clear and soft gates, independently playable rhythmic layers. Test tempo changes and retrigger behavior. |
| 10 Percussion and Transitions | Tuned drums, short ticks, noise percussion, controlled impacts, risers, falls, reverses, transition swells. State one-shot/held-key behavior and tail duration. |

Alternative: if drones require a full category, combine keys/plucks or bells/plucks. Do not let horror, trailer impacts and huge pads consume most of the bank at the expense of everyday composing tools.

## Performance and cross-genre requirements

These are engineering/design recommendations inferred from the sources, not facts about unbuilt presets.

- Every patch needs a concise role, useful register and performance note. Mood labels help browsing but must not imply that a sound only works in one film genre.
- Keep the default patch harmonically neutral unless a deliberately named interval/cluster is essential to its identity. Provide keytracking for tonal instruments and explain fixed-pitch/atonal patches.
- Use a common macro layout where feasible: tone/brightness, movement amount, movement rate or articulation, texture/body balance, attack, release, space, width. Semantic consistency is more valuable than forcing a meaningless control into a slot.
- Many patches need velocity-dependent timbre; sustained patches benefit from continuous wheel/expression shaping. Assignments should preserve usable low-velocity notes and avoid abrupt timbral jumps.
- Dry defaults or a strong dry option help pop, dance, jazz-adjacent production and dialogue underscore; long spacious tails may suit standalone ambience but should be controllable.
- Broad usability belongs to the collection, not literally every preset. An atonal impact will not become an authentic solo violin by tweaking it. State that clearly without diminishing its legitimate musical role.
- Avoid “Hollywood-certified,” composer endorsement, guaranteed adoption, or claims of recorded-instrument realism unless actually evidenced.

## Five distinct per-preset QA passes

1. **Definition and load:** parse valid file, resolve all assets, finite/range-valid parameters, exact category/name, independent identity, successfully load in the actual synth; reject silent/default-fallback patches.
2. **Default audio:** render representative notes or a category-appropriate phrase; measure finite output, onset, peak/RMS, silence, DC, release tail and gross clipping. Record the test stimulus and sample rate.
3. **Performance range:** low/mid/high velocity; relevant low/mid/high registers; wheel/expression extremes; sustain and overlapping notes; ensure timbral controls are useful and do not unexpectedly kill audio or create extreme spikes.
4. **Mix and variation:** mono fold-down, repeated-note behavior, chord clarity for polyphonic categories, tempo alternatives for pulses, dry/wet/width extremes, compare audio and parameter fingerprints for near duplicates. Numerical similarity is a review cue, not a universal rejection criterion.
5. **Musical audition:** listen to each preset at normal level in at least one appropriate short musical phrase and against a sparse accompaniment/dialogue-proxy context; assess harshness, relevance, fatigue, gesture quality, naming and practical control. This cannot honestly be replaced by five automated reports, nor can it prove Hollywood adoption. If actual listening is unavailable, mark this pass pending rather than passed.

No arbitrary claim of universal genre fitness follows from passing these tests. A published audition checklist, defects/fixes log and retained audio snippets give a meaningful basis for composer feedback.
