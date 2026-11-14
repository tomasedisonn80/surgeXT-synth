# Wave 1 / Agent 04: Surge patch implementation contract

Read-only inspection of `/workspace/scratch/9118c5c72ad8/surgeXT-synth` at main baseline dfdd2f94a. AGENTS read; no repository files modified.

## FXP/container

Authoritative `src/common/PatchFileHeaderStructs.h` and `SurgeSynthesizerIO.cpp:681`:

- Big-endian FXP header is 60 bytes: `CcnK`, 32-bit byteSize (native writer uses 0), `FPCh`, version1, `cjs3`, fxVersion1, numPrograms1, 28-byte program name, 32-bit chunkSize.
- Payload starts with 32-byte little-endian patch header: `sub3`, uint32 XML byte count, six uint32 wavetable byte counts for scenes A/B ×3 oscillators. When no wavetable oscillator is used these counts are all zero.
- XML starts at file offset92; use explicit declared XML length, never parse all trailing bytes if tables/arbitrary data exist. Header values must exactly match payload.
- `SurgeStorage.h:156`: current streaming revision **31**. Init Saw file is revision22; writing current semantics under22 triggers compatibility mutations, so prefer31 after building modern parameter state.
- `<patch revision="31"><meta name="..." category="..." comment="..." author="..." license="..."><tags>...</tags></meta><parameters>...</parameters>...`.
- Parameter tag examples: `a_osc1_type`, `a_osc1_param0`..`param6`, `a_env1_attack`, `a_filter1_cutoff`; scene B uses `b_`. Numeric `type="0"`=integer, `type="2"`=float. `value` is native stored value. Controls may use `temposync="1"`, `extend_range`, `absolute`, `deactivated`, `deform_type`.
- Factory template is CC0; a structural initialized patch may be derived from it, but preserve exact parameter types when changing oscillators. Best proof is native load and save/reload.

## Modulation and macros

`SurgePatch.cpp:2183–2321,4148–4187`; `SurgeVoice.cpp:1271`; `Parameter.cpp:4523`:

```xml
<a_filter1_cutoff type="2" value="12">
  <modrouting source="7" depth="24" muted="0" source_index="0" />
</a_filter1_cutoff>
```

Depth in XML is **native parameter units**, not normalized. Voice engine adds `depth * source_output`. Thus cutoff depth12 shifts one octave; envelope depth1 doubles time; percent mix depth0.3 changes mix30 percentage points. `setModDepth01` API multiplies normalized depth by native parameter span before storage.

`ModulationSource.h`: velocity1, keytrack2, polyAT3, channelAT4, pitchbend5, modwheel6, macros7..14, AmpEG15, FilterEG16, voiceLFO1..6=17..22, sceneLFO1..6=23..28, timbre29, releaseVelocity30, random bipolar31/unipolar32, alternate33/34, breath35, expression36, sustain37. Use source_index0 full LFO output. Global destination routes should include source_scene0 unless a specifically per-scene modulator is needed. Float destinations only. Avoid integer target routes.

```xml
<customcontroller><entry i="0" bipolar="0" v="0" label="Brightness" />...</customcontroller>
```

Eight macro entries i0..7; label buffer20 means ≤19 ASCII characters. Global controller storage is in sceneA and shared with both scenes. Bipolar0 gives0..1, bipolar1 gives−1..1. Each named macro should actually route to at least one audible control. Macros/global controllers can target FX; voice modulators cannot.

## Native units

`Parameter.cpp:665,808,1551`:

- ADSR time = log2(seconds), range−8..5 (~3.9ms..32s), minimum displayed as0. Envelope sustain0..1.
- Filter cutoff = `12*log2(Hz/440)` (0=440Hz,12=880Hz), normal range−60..70. Lowcut normal types same conversion.
- Reverb time log2(seconds), range−4..6; predelay log2(seconds),−8..1.
- LFO rate is log2(Hz) unsynced. Tempo-synced notation is based on120BPM; verify by native rendering at multiple BPM rather than assuming displayed text.
- Volume parameter is dB; oscillator mixer `a_level_o1` etc and scene `a_volume` are amplitude-like values, not dB. Set muted unused oscillators explicitly.

## Oscillators and correct defaults

`SurgeStorage.h:296` IDs and respective `dsp/oscillators/*Oscillator.cpp::init_ctrltypes/init_default_values`.

| ID | Model | p0..p6 native defaults / meaning |
|---|---|---|
|0|Classic|shape0; width1=.5; width2=.5; submix0; sync0; detune.1; unison integer1|
|1|Sine|shape integer0; feedback0; behavior integer1; lowcut−60 **deactivated1**; highcut70 **deactivated1**; detune.1; unison integer1|
|5|FM3|M1 amount0; ratio float1; M2 amount0; ratio float1; M3 amount0; M3 freq0; feedback0|
|6|FM2|M1 amount0; ratio **integer1**; M2 amount0; ratio **integer1**; M1/2 offset0; phase0; feedback0|
|8|Modern|saw.5; pulse0; triangle0; width.5; sync0; detune.2; unison integer1|
|9|String|exciter integer0; level1; decay1=.95; decay2=.95; detune.1; balance0; stiffness0|

Modern p0,p1,p2 must not be deactivated; p2 deform_type0. String p1 deform_type34 (=os_onex2|interp_sinc32); p6 deform_type64 (=filter_fixed). String decay is not envelope seconds, defaults .95; avoid pushing feedback decay to unity. String exciters0..6 are burst noise/pink/sine/triangle/ramp/square/sweep,7..13 continuous versions,14 audio input,15 sceneA. Avoid14/15 for a self-contained bank. FM2 ratios1..32 integer; FM3 ratios0..32 float. No wavetable assets are required by these six models.

## Effects and slots

`SurgeStorage.h:344,422`, `dsp/effects/DelayEffect.cpp`, `libs/sst/sst-effects/include/sst/effects/{Delay,Reverb2}.h`:

FX XML is `fxN_type` and `fxN_p0`..p11, with N1-based. Slots1/2 are sceneA inserts,3/4 sceneB inserts,5/6 sends,7/8 global inserts; newer slots9..16 add corresponding third/fourth positions. For common space and delay use **fx7/fx8 global inserts**; these avoid send setup complexity.

Delay ID1: p0 left log2 seconds, p1 right log2 seconds, p2 feedback, p3 crossfeed, p4 lowcut, p5 highcut, p6 modulation log2Hz, p7 detune depth, p8 input channel, p9 reserved, **p10 mix**, p11 width dB. Native defaults: −2,−2,.5,0,−24,30,−2,0,0,[reserved],.5,0. Feedback deform_type1; keep feedback/crossfeed conservative. p0/p1 `temposync1` for musical delay. p1 deactivated0 if independent right time.

Reverb2 ID11: p0 predelay default−4; p1 room size0 (−1..1); p2 decay.75; p3 diffusion1; p4 buildup1; p5 modulation.5; **p6 LF damping.2, p7 HF damping.2**; p8 width0dB; **p9 mix.33**. The authoritative enum is SST Reverb2.h (the adapter lists HF before LF in code but indexes by enum). For adaptable music use lower initial mix and a macro to raise it; dry boundary should be valid.

## Filters

`libs/sst/sst-filters/include/sst/filters/FilterConfiguration.h:23` IDs:

0 off;1 LP12;2 LP24;3 legacy ladder;4 HP12;5 HP24;6 BP12;7 notch12;8 comb+;9 S&H;10 vintage ladder;11 OB-Xd12LP;12 OB-Xd24LP;13 K35LP;14 K35HP;15 diode;16 cutoffwarpLP;17 cutoffwarpHP;18 cutoffwarp notch;19 cutoffwarp BP;20 OB-Xd HP;21 OB-Xd notch;22 OB-Xd BP;23 BP24;24 notch24;25 comb−;26 allpass;27 cutoffwarp allpass;28 resonancewarpLP;29 resonancewarpHP;30 resonancewarp notch;31 resonancewarpBP;32 resonancewarpallpass;33 tripole;34 CytomicSVF;35 Xpander.

Simple stable picks are1/2/4/6 with subtype0 and restrained resonance. Keytracking0..1 should be intentional. Do not route to a filter left type0; all audible macro routes need native verification.
