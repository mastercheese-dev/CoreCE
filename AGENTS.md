# Releases

Mac, Windows, and Linux share one number on the main line and a separate number on the testing line. A client already installed only accepts a tag that is exactly `build-` or `testing-` plus a whole number, and only when its own file is attached. A local test build is not a release: it does not take the next number, and it does not move the README buttons.

## Numbers

Main tags are `build-N`. The first shared main release is `build-59`. Windows clients are at 30, Mac clients at 52, and Linux clients at 58, so a main number of 58 or lower is ignored by at least one of them. After `build-59`, the next main tag is one higher than the highest `build-` tag on this line.

Testing tags are `testing-N`. The first shared testing release is `testing-39`. Testing clients still running are at Linux 38, Windows 34, and Mac 33, so a testing number of 38 or lower is ignored by at least one of them. After `testing-39`, the next testing tag is one higher than the highest `testing-` tag on this line.

Main and testing do not share a count. A number is used once on its own line. Deleted testing tags stay unused. `build-30`, `build-52`, and `build-58` stay as they are.

## Files on the tag

Each tag carries all three of these files:

- `CoreCE-macos-arm64.dmg`
- `halo-windows-release.zip`
- `halo-linux-release.zip`

The next number on that line waits until all three files are on the current tag. A late file is added to that same tag. A tag missing a file is invisible to that platform.

A main release is a normal release and is marked Latest. A testing release is a prerelease and is not marked Latest. Android stays where it is unless `halo-android-release.zip` is attached on purpose.

## Stamp

Windows and Linux release builds set `HALO_BUILD_NUMBER` to that tag's number, `HALO_BUILD_CHANNEL` to `main` or `testing`, and `HALO_UPDATE_REPOSITORY` to `mastercheese-dev/CoreCE`. Those zip names come from a release build. A debug build looks for a different zip and is not a published update.

A Mac release passes that same number to `--build-number` and `--update-build`, and passes `main` or `testing` to `--update-channel`.

## README

Publishing a main release updates the three download buttons in `README.md` on `main`, and the sentence under them, so Mac, Windows, and Linux all download that `build-N` tag. That README change is committed and pushed to `main` with the release. Testing releases leave the buttons where they are. Do not merge `testing` into `main` to move the buttons.

## Notes

Release notes name the tag, the network protocol, and what changed. They do not use em or en dashes, and they do not put a hyphen where a dash would have been. They do not credit Cursor. They do not say that bots are used in matchmaking, or in a lobby other than LAN.
