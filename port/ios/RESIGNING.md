# Install the unsigned IPA with your own Apple account

Use an Apple Silicon Mac with Xcode 26 or later and Python 3. This runtime requires
iOS/iPadOS 26 or later and a profile granting Extended Virtual Addressing. A paid
development team was required to provision that capability. Build the unsigned
device IPA from this repository with [the iOS build instructions](README.md)
and `tools/ios_package.py`. This repository does not publish an IPA. A simulator
app cannot run on an iPhone.

The script signs locally using a certificate/private key already in your
Keychain and a provisioning profile issued for your account. It does not
sign in to Apple, create certificates, register devices, or bypass provisioning.
You do not need to send your device ID to this project's maintainer when
you use your own account.

## Set up signing once

1. Add your Apple account in **Xcode → Settings → Accounts**. Connect your
   iPhone, trust the Mac, and enable Developer Mode if requested.
2. If you do not already have a suitable iOS development profile, create a
   temporary iOS App project in Xcode. Select your team, enable automatic
   signing, add the **Extended Virtual Addressing** capability, and set a unique bundle identifier such as `com.yourname.haloce`.
   Build/run that temporary app on your phone so Xcode creates the identity
   and a profile covering the device. Then delete the temporary app from
   the phone before installing Halo with the same identifier.
3. Locate the temporary app's build product using **Products → Show in
   Finder**, then **Show Package Contents**. Copy its
   `embedded.mobileprovision` somewhere convenient. Alternatively, use an
   existing development or Ad Hoc `.mobileprovision` downloaded from Apple.
4. List available identities:

   ```sh
   security find-identity -v -p codesigning
   ```

   Use the matching **Apple Development** identity (or **Apple Distribution**
   identity for an Ad Hoc profile). Its private key must be in your Keychain,
   and its certificate must be included in the profile. An App Store profile
   cannot install this build directly on a phone.

## Sign and install

From the root of this repository, run:

```sh
./tools/ios_resign.sh ~/Downloads/OpenCE-unsigned.ipa \
  --identity 'Apple Development: Your Name (IDENTITY_ID)' \
  --profile ~/Downloads/embedded.mobileprovision \
  --bundle-id com.yourname.haloce \
  --device YOUR_DEVICE_UDID \
  --output ~/Downloads/Halo-signed.ipa
```

The exact identity name or its SHA-1 fingerprint is accepted. `--device`
is optional but recommended: it verifies that the supplied UDID is covered.
Use `xcrun devicectl list devices` or Xcode's Devices and Simulators window
to identify your phone. The output must not already exist.

The script checks profile expiry, platform, bundle ID, device and signing
certificate; updates the bundle ID; signs ANGLE/SDL frameworks and the app
inside-out; generates DER entitlements; verifies signatures; and writes a
new IPA plus a SHA-256 checksum. It never changes the downloaded IPA.
The shell entry point uses the adjacent `ios_resign.py`, so keep both files.

Install `Halo-signed.ipa` with Apple Configurator, or extract and install
the app with Xcode's command-line device tools:

```sh
mkdir -p ~/Downloads/Halo-signed-extracted
ditto -x -k ~/Downloads/Halo-signed.ipa ~/Downloads/Halo-signed-extracted
xcrun devicectl device install app --device YOUR_DEVICE_UDID \
  ~/Downloads/Halo-signed-extracted/Payload/Halo.app
xcrun devicectl device process launch --device YOUR_DEVICE_UDID com.yourname.haloce
```

On first launch, choose your own supported original Xbox Halo ISO/XISO.
No game image or maps are included in the IPA. Keep the same account and
bundle ID for updates to retain imported data. Renew the provisioning
profile and re-sign before it expires; a free Personal Team's validity and
device limits differ from paid developer membership. Back up saves before
uninstalling. Do not upload your signed IPA/profile or private key to this
repository; they contain your signing/device information.

## AltStore / similar sideloaders

Do not assume that a free Apple ID plus AltStore Classic will run this build.
The runtime requires `com.apple.developer.kernel.extended-virtual-addressing`;
the provisioning profile must authorize it and the re-signer must retain it.
Our tested setup uses a paid Apple Developer team. Personal Team provisioning
and third-party sideloaders have not been validated for this port.

The unsigned IPA has no embedded profile or code signature. The kit includes
`Required-entitlements.plist` for tools that accept an entitlement file, but
adding that file does not grant the capability: the profile must contain it.
If a sideloader cannot provision or preserve it, use the script above with an
appropriate profile. Installation alone is not proof of a working runtime.
This is a manually re-signable test artifact, not an AltStore PAL listing.

## LiveContainer and memory reservation errors

A reported iPhone 17 Pro Max / iOS 26.0 LiveContainer launch failed with
`Guest arena reservation: (os/kern) invalid address` after ISO import. This
happens before the game image starts, not while parsing the ISO. Re-importing
or reinstalling the same image does not change process address-space limits.

[LiveContainer documents](https://github.com/LiveContainer/LiveContainer#limitations)
that guest entitlements are not applied to its host process. Granting Extended
Virtual Addressing to the Halo guest therefore does not prove the hosting
process has it. The file-picker workaround does not grant memory capabilities.
The error is consistent with a missing process capability or an incompatible
address-space layout; the log alone does not distinguish them.

Try the standalone installation described above with a profile authorizing
Extended Virtual Addressing. LiveContainer remains unverified even if its host
is entitled: the required address range must also be available. Do not change
the compiled runtime's fixed base address as a signing workaround.

The startup preflight now checks memory before importing an ISO. Failures show
a scrollable native screen with **Copy details** and **Close**, and log the
requested address/size and kernel result in `Documents/OpenCE/save/halo.log` (or the
existing legacy save directory). Existing imports and saves are preserved.

## Build it yourself

Follow [the build instructions](README.md#build) in this repository, supplying
your own team and unique bundle ID. Source builds need the same Extended
Virtual Addressing capability. Compiling locally does not remove that requirement.

The public IPA is checked by re-signing it locally and verifying the nested
framework and app signatures and required entitlement. This does not establish
compatibility with every device or signing service.

Apple references: [signing setup](https://developer.apple.com/documentation/xcode/running-your-app-on-simulated-or-physical-devices),
[provisioning profiles](https://developer.apple.com/documentation/technotes/tn3125-inside-code-signing-provisioning-profiles),
and [nested signatures and DER entitlements](https://developer.apple.com/documentation/xcode/using-the-latest-code-signature-format).
