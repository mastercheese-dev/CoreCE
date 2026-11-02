# Releases

Mac, Windows, and Linux share one number. Each platform is built and published on its own machine, and that machine publishes only its own file. A client already installed only accepts a tag that is exactly `build-` or `testing-` plus a whole number, and only when its own file is attached. It moves only to a higher number on its own line. A local test build is not a release: it does not take the next number, and it does not move the README buttons.

## Numbers

The version is `0.3.N`. N is the number on the tag. `testing-42` is `0.3.42`. `build-58` is `0.3.58`. The number is read from the tag that file is going on. It is the same number on every machine. A machine pulls `testing` before it publishes, and it publishes from a checkout where the version is that tag number. Counting older titles, or keeping a local next version, writes a different number on the same tag.

Testing tags are `testing-N`. The first shared testing release is `testing-39`. Testing clients still running are at Linux 38, Windows 34, and Mac 33, so a testing number of 38 or lower is ignored by at least one of them. Deleted testing tags stay unused.

A main release is the testing release you decide to ship. Its tag is `build-N` with that same N, and its version is the `0.3.N` already used on that testing tag. If testing has moved through several numbers since the last main release, the main tag is the testing release you are shipping, not one higher than the old `build-` tag.

Windows clients are at 30, Mac clients at 52, and Linux clients at 58, so a main number of 58 or lower is ignored by at least one of them. `build-30`, `build-52`, and `build-58` stay as they are. `build-79`, `build-80`, `build-81`, `build-82`, and `build-84` are old tags and are not releases. They stay as they are, and a promotion does not move them.

## Files on the tag

Each tag carries all three of these files:

- `CoreCE-macos-arm64.dmg`
- `halo-windows-release.zip`
- `halo-linux-release.zip`

Before a machine uploads, it looks up the highest `testing-` tag. When that tag is missing this machine's file, the file is added to that tag. The build is stamped with that same number. The next testing tag is opened only when the highest tag already has all three files, and it is one higher. A tag missing a file is invisible to that platform.

Adding a file leaves the tag on its current commit. After the upload, the title is rebuilt from that tag's number and from every platform file then attached, including files another machine already added. Mac, Windows, and Linux stay in that order. A one platform upload keeps the other platforms in the title when their files are already there.

The notes stay with the tag. Mac writes them when it opens the tag, and those notes already cover Mac, Windows, and Linux. A later machine adds its file and rebuilds the title. It leaves the notes as Mac wrote them.

A main release is a normal release and is marked Latest. A testing release is a prerelease and is not marked Latest. Android stays where it is unless `halo-android-release.zip` is attached on purpose.

## Title

The title is the version, the platforms whose file is on the tag, and the tag. The version in the title is the same N as the tag. Mac, Windows, and Linux are named in that order, and only when that platform's file is attached. Android is named only when its zip is attached. A testing release with all three files is `0.3.42 Mac, Windows, Linux testing-42`. A Windows testing release whose tag does not yet have the other files is `0.3.45 Windows testing-45`. A Windows main release is `0.3.30 Windows build-30`. An installed client does not read the title. It reads the tag and its own file name.

## Stamp

Windows and Linux release builds set `HALO_BUILD_NUMBER` to that tag's number, `HALO_BUILD_CHANNEL` to `main` or `testing`, and `HALO_UPDATE_REPOSITORY` to `mastercheese-dev/CoreCE`. Those zip names come from a release build. A debug build looks for a different zip and is not a published update.

A Mac release passes that same number to `--build-number` and `--update-build`, and passes `main` or `testing` to `--update-channel`.

## README

Publishing a main release updates the three download buttons in `README.md` on `main`, and the sentence under them, so Mac, Windows, and Linux all download that `build-N` tag. That README change is committed and pushed to `main` with the release. Testing releases leave the buttons where they are. Do not merge `testing` into `main` to move the buttons.

## Notes

Release notes name the tag, the network protocol, and what changed. A Mac release writes them once, for Mac, Windows, and Linux together, including how each one is installed. Windows and Linux leave that text alone when they add their files. They do not use em or en dashes, and they do not put a hyphen where a dash would have been. They do not credit Cursor. They do not say that bots are used in matchmaking, or in a lobby other than LAN.
