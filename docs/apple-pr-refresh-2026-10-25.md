# Apple PR refresh, October 25

## Working state

The public PR is mastercheese-dev/CoreCE#1 (formerly innerhat-dev/OpenCE).
Its old head ff18344c is retained in backup/ios-installer-before-review-20261025.
The public branch is being rebuilt on c88fcf2d, rather than replaying implementations
already present in CoreCE under different commits. The private release branch
apple/sandbox-sync remains unchanged at 8cc4a6c7.

The remaining changes currently carried are the dynamic iOS guest arena, which
removes the Extended Virtual Addressing requirement, the player-one touch routing
correction, and a visible settings entry so a player does not have to discover
the shake gesture. The upstream names, icons and bundle defaults are preserved.
Both controller-routing probes passed. The simulator build and signed-runtime
probe at three arena addresses passed; no ready-for-review claim is made yet.

## Compatibility gate

CoreCE uses network 34 and corece/1/lobby topics with corece-lobby-1 signatures.
OpenCE b09a3ea7 uses network 38 and hceu/3/lobby topics with hceu-lobby-1
signatures. The prior public branch used network 29. Merely changing the browser
topics or version constant is not a valid interoperability fix. Whether the
public contribution stays against CoreCE or moves to a mainline-compatible port
is pending the user's direction. A mainline-compatible downstream build remains
part of the work; no compatible release has been produced by this refresh.

## VISR review

Reviewed https://github.com/steverice/visr at
7f08179473a5d096d718bb15881f520bf3fd6634. Its README describes native Metal,
AppKit, touch controls and Apple device support. docs/benchmarks.md explicitly
says comparisons are still in progress; no measured performance advantage is
established by that page.

Concrete useful candidates inspected:

- tools/test_shader_warming_identity.py exercises separation of map warmup
  aliases from upload identity, including unknown fingerprints, regional stock
  maps, renamed maps and path traversal. This is useful if we adopt portable
  warmup packs, but our Metal cache currently uses shader-source identity; do
  not insert VISR map aliases into that cache without a corresponding feature.
- port/ios/tests/metal_border_probe.m tests real sampler construction/cache/bind
  bodies with stand-ins only for Metal object boundaries. This is a useful
  approach for verifying sampler fidelity and failed allocations without
  claiming the probe proves GPU output. Compare with our existing Metal sampler
  tests before adding duplicate coverage.
- Automatic shader uploads are a separately enabled feature in VISR. They are
  not being enabled or copied here; local user-exported diagnostics remain the
  current behavior.

VISR is Apache-2.0 with component notices. Its contribution rules reject GPL/AGPL
imports into VISR. That policy matters for contributions back to VISR; it does
not by itself prohibit using appropriately attributed Apache code here. No VISR
implementation has been copied in this refresh.

## Completion gates still open

- Finish reusable-delta audit against the old PR and private branch, including
  private ISO retention, packaging, documentation and optional save sync.
- Address reviewer findings, distinguishing defects from feature requests and
  preserving the user's rule that touch controls cannot be hidden without an
  alternate input method.
- Decide and implement the mainline compatibility path; verify discovery and a
  real join rather than just constants or a populated listing.
- Build and test final Mac/iOS artifacts; test settings in a leased simulator.
- Rewrite PR description around final changes, attribution and current evidence;
  push the reviewed branch, inspect checks and mark ready only after validation.

## Validation follow-up

The refreshed simulator app launched in landscape to the ISO importer on a
leased simulator; screenshot /tmp/halo-pr-refresh-startup.png. The lease was
released. This does not verify the in-game settings button or multiplayer.
Mac private-ISO retention, rollback, persistent preferences and legacy migration
probes passed. CoreCE storage naming is preserved in the public change.
Removed loading-film packaging; an incremental-build check found Xcode retained
the previous resource, so ios_build.py now removes it before build/signing.
The rebuilt simulator package no longer contains the film (33 MB versus 56 MB).
Device build is running in /tmp/halo-pr-refresh-device.log. No personal device
has been installed on or interacted with. The branch has not yet been pushed.

## Public refresh build results

At 8f4875b7, simulator and device builds passed. The native Metal Mac build
passed, its installed CoreCE.app signature verified, and BuildInfo records the
same source. Device metadata marks the dynamic arena and excludes the loading
film. Controller/installer/DMG pytest run: 11 passed and 3 subtests passed.

The branch is pushed and PR #1 now describes this smaller delta, attribution,
compatibility limits and historical-versus-current validation. GitHub reports
MERGEABLE, with no status checks reported. It remains draft.

An attempted real-map simulator test has not reached the main menu: the service
rejects install_app_data, and the attempted seed helper did not make the maps
available to the game. The screenshot still shows the importer. Do not count
this as settings or gameplay validation. The lease was released and temporary
extracted data removed; the original ISO and user containers were unchanged.

## In-game rendering and settings verified

The test setup is now resolved: the importer requires all eleven stock campaign
and UI maps, and reinstalling the simulator app changes its container path.
After supplying complete user-owned maps to the current container, the settings
button action opened Graphics & Diagnostics in landscape.

The ordinary game view exposed a separate real regression: host_gl_bridge.c
lacked the selected-renderer dispatch header, sending texture-allocation and
blit calls to a different GL driver from the active context. Restoring the
header rendered the main menu successfully. The new production-bridge probe
checks selected-driver resolution, texture arguments and framebuffer-blit stack
arguments; a negative control removing the header fails to link as expected.
The final runtime log loads 1,277 menu widgets and has no incomplete-framebuffer
report. Screenshots remain local: /tmp/halo-pr-settings-final.png and
/tmp/halo-pr-render-fixed.png. They are not bundled game content or evidence of
a full campaign/multiplayer match.

Targeted suite: 12 tests and 3 subtests passed. The device rebuild after the GL
fix is in progress. The leased simulator has been released and extracted test
maps removed. No personal devices or existing saves were changed.

## Review readiness

The final iOS device rebuild after the renderer fix passed. This PR's remaining
Apple delta is ready for code review: dynamic memory, retained Mac ISO import,
controller routing/settings access, GL bridge dispatch, packaging and accurate
documentation. Simulator main-menu and settings checks are complete. Physical
full-match, free-account signing and performance measurements are not claimed.
Private optional cloud-sync/sandbox distribution work remains on the separate
apple/sandbox-sync branch; it is not silently included in this upstream PR.

The public PR intentionally retains CoreCE's protocol and discovery namespace.
Restoring the user's mainline server compatibility remains unresolved and is not
implied by marking this CoreCE-targeted contribution ready for review.
