# Wave 5 / assignment 30 — Live captions and Clear paper

## Evidence

Compared original references 10 and 08 against the committed 1586 × 992 native
gallery from source `a362667839b8e1d742cf3af94bb8710fa3c6d1fd`. These captures
precede this patch and the intervening wave 3–5 work.

At a minimum-channel threshold of 130, Live Macro 1's first-line ink spans
x519–553/y318–325 in the reference and x527–567/y322–329 natively. Its actual
name, Drive, spans x532–555/y337–344 versus x542–567/y338–345. On the other side,
Macro 5 starts at x1032 in the reference versus x1021 natively. Across all eight
macros, the first line is approximately four pixels low and the captions sit
11–15 pixels too close to their correctly positioned dials.

The reference Scene A lettering spans x92–146/y135–146; the native uppercase
caption spans x96–141/y140–147. The reference uses bold mixed case for both
scene selectors.

Clear blank-patch RGB medians confirm a shared neutral-field deficit:

| Field | Reference | Native |
| --- | --- | --- |
| Inspector upper | 252,252,253 | 249,249,249 |
| Inspector middle | 251,251,251 | 248,248,248 |
| Inspector lower | 248,248,249 | 245,246,247 |
| Oscillator 1 lower | 248,248,249 | 245,246,247 |
| Filter 2 lower | 248,248,249 | 245,246,247 |
| FX bottom gutter | 248,248,249 | 245,246,247 |

## Changes

Only three scoped `Workspace.cpp` regions changed:

- Live macro side captions move outward 12 pixels. Their upper line moves up
  four pixels with 0.90 horizontal font scale; actual assignment names move up
  one pixel and use the brighter text ink. Fitted text stays within each existing
  component, including longer custom names. Dial paint and native values remain
  unchanged.
- The two Live scene cards use the existing configurable `HeaderButton`, bold
  16.5 px nominal type, and `Scene A`/`Scene B`. IDs, bounds, toggle state,
  focus, accessible names, owner and callbacks are preserved. No patch names
  are invented; the separate header A/B controls are untouched.
- Clear's neutral panel gradient changes to `#FDFDFD` → `#F8F8F9`. Accent,
  display, border and text colours are untouched.

## Verification and remaining gaps

`git diff --check` passes. Focused source comparisons confirm unchanged shared
font/button painters, binding and input handlers, macro face painter, MacroXY,
and header layout; scene setup differs only in class/caption/presentation.
No build, tests, commit or push was performed.

The coordinator owns native capture QA. Glyph rasterization and individual
macro-row baseline differences of approximately one pixel still need that
capture. Clear's reference has local paper variation that a shared gradient
does not reproduce; these changes do not establish pixel identity.
