# iPhone development build

Native ARM64 iOS/iPadOS 26+ build using innerhat's signed ILP32 runtime and
ANGLE Metal renderer. Both iPhone and iPad are supported by the app bundle.
No game data is packaged: first launch presents **Choose Halo ISO** in Files.
Import validates supported original Xbox PAL/NTSC-US maps, verifies a retained
private image by SHA-256 readback, and atomically publishes the new map cache.
Subsequent launches reuse it. Existing imports/saves from our earlier branch
and existing innerhat `Documents/GameData/maps` installations remain readable
when the same bundle ID and account are used. Different IDs have separate data.

## Historical test IPA

[Download the unsigned IPA and re-signing kit](https://github.com/AttilaTheFun/halo-ce-universal/releases/tag/apple-ios-pr1-20261013) from the earlier PR revision. That download is not a build of the refreshed branch.
Follow [the re-signing guide](RESIGNING.md). Source builds starting at build 17
no longer require Extended Virtual Addressing. Free-account AltStore and
LiveContainer installation remain unverified; older downloads still require it.
You supply your own game image on first launch. No device ID needs to be sent
to the maintainer when you use your own signing account.

## Build

First complete [Apple build setup](../../docs/apple-build.md), including full
Xcode 26+, CMake, the public dependencies, your local game maps and XDK headers.
You do not need to build the Mac app or the simulator first.
`DEVELOPER_DIR` defaults to `/Applications/Xcode.app/Contents/Developer`;
the build does not change the system's selected developer directory.

Sign into Xcode → Settings → Accounts with your own Apple account. Connect and
unlock the iPhone, trust the Mac, and enable Developer Mode in Settings →
Privacy & Security (restart and confirm when prompted).

The current source does not request Extended Virtual Addressing or Increased
Memory Limit. The guest arena is selected at runtime; no JIT entitlement is
required. Your signing profile must still cover the chosen app identifier and
the device. A paid development identity was used for our tests; free-account
sideloading is not independently verified on this refreshed branch.

Find your own Team ID in your Apple developer membership, and choose a unique
reverse-DNS app identifier. Replace both example values below:

```sh
python3 tools/ios_build.py --platform device --sign \
  --team YOUR_TEAM_ID --bundle-id com.yourname.halo
```

This compiles the game, builds SDL and the native host, and asks Xcode to
provision/sign with your local account. Game data is imported in the app. No signing identity is
provided by this repository. Outputs:

The runtime reserves an OS-selected aligned virtual arena without Extended
Virtual Addressing or Increased Memory Limit. No JIT entitlement is used. A development
or Ad Hoc profile must cover the installing device. See the [IPA re-signing
guide](RESIGNING.md) for a locally held identity/profile workflow.

Omit `--sign` and `--team` for an unsigned compile check. To use the Xcode UI,
open the generated project, choose your team under Signing & Capabilities,
select your connected iPhone, and run **Halo**. Use Release configuration for
performance testing. Rerunning the build script regenerates the project from
source, so pass your team and bundle ID on subsequent signed command-line builds.

`--host-only` reuses the compiled game image for signing/UI-only changes. Omit it
when game sources, compiler code or generated bridges change. `--platform both`
builds separate simulator and device products from the same source.

## Install on a phone

With `DEVELOPER_DIR` set as in the setup guide:

```sh
xcrun devicectl list devices
xcrun devicectl device install app \
  --device DEVICE_ID build/ios/device/Release-iphoneos/Halo.app
xcrun devicectl device process launch \
  --device DEVICE_ID com.yourname.halo
```

Replace `DEVICE_ID` with your phone's identifier and use the bundle ID passed
at build time. Alternatively, use Xcode's Run button. The default bundle ID is
`local.halo.ce-universal.ios`; choose your own for device provisioning to avoid
registration conflicts.

The app does not include game data. On first launch it asks for your own Xbox
disc image (`.iso` or `.xiso`) and keeps the extracted maps in the app's
Documents folder. An older `Documents/GameData/maps` folder is still used when
it is already there. Saves, `config.toml` and `halo.log` are in `Documents/save`
inside the app container. Do not publish the app bundle or local signing logs.

For multiplayer on a physical phone, see [Mac and iPhone System Link](../../docs/apple-multiplayer.md).

## Simulator

No device development team is needed:

```sh
python3 tools/ios_build.py --platform simulator
xcrun simctl list devices available
```

Choose an installed iPhone simulator's UUID, then:

```sh
xcrun simctl boot SIMULATOR_ID   # skip if already booted
open -a Simulator
xcrun simctl bootstatus SIMULATOR_ID -b
xcrun simctl install SIMULATOR_ID build/ios/simulator/Release-iphonesimulator/Halo.app
SIMCTL_CHILD_HALO_TOUCH_CONTROLS=1 xcrun simctl launch \
  SIMULATOR_ID local.halo.ce-universal.ios
```

Use your selected `--bundle-id` in the launch command if you overrode the default.
Simulator performance does not predict phone performance.

## Controls

Touch input uses two sticks plus A/B/X/Y, triggers, crouch, zoom, flashlight,
grenade type, Back and Pause. These feed the existing Xbox controller mapping.
The layout is an initial playable implementation; sensitivity and ergonomics
still need testing on the phone. Physical controllers use SDL's GameController
backend, and touch controls hide when a system controller is present. Physical
gamepad gameplay has not yet been validated on the iPhone.

For simulator touch testing, launch with `SIMCTL_CHILD_HALO_TOUCH_CONTROLS=1`
before `xcrun simctl launch`; the simulator may advertise a system gamepad.
`HALO_TOUCH_CONTROLS=0` hides the overlay. The Mac window uses neither setting.

## Frame rate

The app opts into ProMotion and SDL requests the display's maximum refresh
rate: 120 Hz on supported iPhones, 60 Hz on other displays. ANGLE/Metal vsync
paces presentation. The engine interpolates rendered frames between its
unchanged 30 Hz simulation ticks, so higher frame rates do not speed up gameplay.
Keep `display.interpolation` and `display.vsync` enabled in `config.toml`.

For a 60 Hz comparison or lower-power session, set `HALO_IOS_REFRESH_RATE=60`
in the Xcode scheme's environment or pass it through `devicectl`:

```sh
xcrun devicectl device process launch --terminate-existing --device DEVICE_ID \
  --environment-variables '{"HALO_IOS_REFRESH_RATE":"60"}' com.yourname.halo
```

Omit the variable (or set it to `120`) to request the display's maximum again.
This is a refresh-rate preference, not a guarantee of sustained 120 FPS: iOS can
reduce the rate for Low Power Mode, the Accessibility → Motion → Limit Frame
Rate setting, or thermal conditions. See Apple's
[ProMotion guidance](https://developer.apple.com/documentation/quartzcore/optimizing-iphone-and-ipad-apps-to-support-promotion-displays).

For device measurements, set `HALO_PERF_LOG=1` at launch. This writes per-frame
timings to `Library/Application Support/Halo/frames.csv` in the app container;
each profiled launch overwrites that file. `frame_ms` measures the interval
between completed buffer swaps (about 8.33 ms at 120 FPS or 16.67 ms at 60 FPS),
not GPU execution time or proof that every frame reached the screen. Use
Instruments' Game Performance/Metal timeline to inspect GPU and presentation
behavior. Profiling is off on ordinary launches.

## Runtime design

The game retains 32-bit pointers and structure layouts. Its ARM instructions
are embedded into the app's signed Mach-O `__TEXT,__halo` section at build time.
The ELF is an intermediate build artifact, not runtime-loaded executable code.
No JIT, writable executable mapping or runtime instruction patching is used.

The LLVM adapter translates original 32-bit game addresses into an OS-selected,
4-GiB-aligned non-executable arena on iOS, including the simulator. The reserved
span is 2.25 GiB of virtual space; physical memory is committed as used. Compiler
pointer translation, import stubs and graphics bridges share its runtime base.
Executable code stays in the signed image, and indirect calls use its immutable
code-image delta. See [the dynamic arena design](../../docs/apple-ios-dynamic-arena.md).

## Verification

```sh
python3 tools/test_ios_runtime.py
python3 tools/ios_mac_runtime_check.py
python3 tools/macos_benchmark.py \
  --host build/ios/mac-runtime-check/halo --guest build/ios/halo_guest.elf \
  --output build/ios/mac-runtime-check/campaign --effects --seconds 45
```

The standalone signed-image test covers globals, indirect calls, stack pointers,
unaligned accesses, atomics, varargs and Xbox memory addresses. The Mac diagnostic
host exercises the complete iPhone engine layout independently of UIKit and
device signing. It is not an iPhone performance benchmark.

On September 27, 2026, the 45-second Mac diagnostic campaign with repeated
explosions exited successfully: median frame time 16.66 ms, p95 17.11 ms,
maximum measured footprint 465 MB. Mac allocator and audio regression probes
also passed. The iPhone simulator rendered the menu at 2868×1320 with Metal and
displayed touch controls. A development build on iPhone 17 Pro Max with
iOS 26.6.2 confirmed the signed engine, 2868×1320 Metal drawable and A19 Pro GPU.
Campaign gameplay, movement/aiming, touch controls and audio were tested on the
phone.

ProMotion validation on the same phone measured the following buffer-swap
intervals with optional frame logging enabled:

| Session | Duration | Average FPS | Median frame | p95 frame |
| --- | ---: | ---: | ---: | ---: |
| 60 Hz override, main menu | 15 s | 59.99 | 16.66 ms | 16.82 ms |
| 120 Hz target, campaign play | 179 s | 119.42 | 8.33 ms | 8.61 ms |

The campaign capture included 21,368 frames, a 9.65 ms p99, an isolated
114.97 ms maximum frame, and a 455 MiB peak physical footprint. A separate
1.70-second sequence of 205 Metal drawable presentation callbacks in Instruments
averaged 119.97 FPS; the trace reported a nominal thermal state. Movement,
aiming, firing and audio were confirmed on the device. The menu and campaign
rows cover different workloads; these short checks do not establish a sustained
120 FPS minimum across every level or long thermal sessions.

Bink movies remain unsupported, as in the Mac port. Multiplayer, sustained
thermal behavior, background/resume behavior and the full campaign require
separate device testing before treating this as a finished iPhone release.

## Graphics and controller settings

Tap the gear button at the top right of the game view, or shake the phone, to
open Graphics & Diagnostics. The gear remains available with a paired controller.
For split screen, enable **Touch as a separate player**, relaunch, then use
**Add Player** and select the additional profile with the physical controller.
By default, touch and the primary hardware controller share player one, and
pairing hardware hides the touch gamepad. Disconnecting it restores touch.

## Server compatibility

This CoreCE-based branch uses protocol 34 and CoreCE's discovery namespace.
It cannot discover or join mainline OpenCE protocol-38 servers. Build success or
a populated CoreCE browser must not be presented as proof of mainline cross-play.
