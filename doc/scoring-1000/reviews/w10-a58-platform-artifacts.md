# Wave 10 / Agent 58 — current platform artifacts

## Current status

This report is awaiting the feature's published GitHub Actions installer run.
It does not treat the successful baseline run as a build of Scoring 1000.
The frozen bank catalog is `67bd4a5dc3fbf6f3794679b24b500f1ef37af83b7a47a863d10cb964a57314a2`.

The independent source audit is in [agent 52's packaging review](w09-a52-installer-pipeline.md).
This assignment checks the actual seven platform build conclusions and uploaded
files from the feature commit. Results will be bound to the exact Actions run,
source SHA, artifact ID, artifact archive hash, nested payload hash and preset
catalog hash in [the machine evidence](w10-a58-platform-artifacts-evidence.json).

## Payload checks

The [archive inspector](../../../scripts/scoring/inspect_archive.py) compares all
1,000 expected FXP paths and SHA-256 values in data-bearing ZIP/tar payloads.
It reads entries without extracting archive paths or executing the installer.
The principal checks will cover a Windows portable installation and Linux full
distribution, with other supported payloads checked when available.
Plugin-only absence is recorded separately when inspected.

A successful build or upload alone does not prove that a particular archive
contains the right bank. Conversely, inspecting a resource payload does not
establish successful installation, platform code signing, DAW-host behavior,
human musical audition or composer endorsement. Those scopes remain explicit.
