# Agent 57: native CI audit

**Status: awaiting publication and an actual current-source workflow run.**
This record does not declare a CI pass. The local full-bank measurements are
separate evidence; the previous repository installer run predates Scoring 1000.

## Prepared verification scope

I inspected the new native workflow and recorded the exact final preset hashes
for its ten selected examples, one from each category. The catalog is:

`67bd4a5dc3fbf6f3794679b24b500f1ef37af83b7a47a863d10cb964a57314a2`

The push-triggered workflow is configured to perform:

| Check | Expected scope, pending observed results |
|---|---:|
| Complete source regeneration and definition validation | 1,000 presets |
| Staged factory hashes and native discovery | 1,000 paths |
| Primary native audio | 10 presets, 230 scenarios |
| Five technical gates | 50 gates |
| Eight macros | 80 responses |
| Long-held and complete-cycle supplement | 20 scenarios |
| Native semantic save/load | 10 presets |
| Original/saved audio comparison | 10 pairs |

A separate read-only artifact checker is prepared in the task workspace. It
reads ZIP members without extracting or executing them, verifies CRCs and input
bindings, compares native and Python measurements, checks every selected report
against the final presets and fixture definitions, and binds the actual run's
engine identifier to its published source commit. Its syntax check passed.
It has **not yet inspected an actual CI artifact**.

The fresh CI executable will have its own hash. It will not be mislabeled as the
locally compiled renderer used for the complete 1,000-preset audio measurements.
The selected CI audio scope is ten presets; complete source and factory discovery
coverage do not turn that smoke run into a second full-bank audio run.

## Next evidence required

After publication, inspect the actual run, step conclusions, configure/build
logs and uploaded artifact. Record its exact source SHA, run/job/artifact IDs,
renderer hash, input hashes and observed results here. Report any failure or
warning before claiming success.

The [machine record](w10-a57-native-ci-evidence.json) explicitly retains pending
status. Human musical audition remains pending; no composer endorsement is
claimed.
