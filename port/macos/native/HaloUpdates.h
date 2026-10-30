#import <Foundation/Foundation.h>

/* The line drawn between the shield and the weapon. Empty means nothing to show. */
int halo_fork_update_notice(char *out, size_t capacity);
void halo_fork_update_set_notice(const char *text);

/* The Mac release number in a fork tag. build-40 is 40. testing-4 is 4. An
   older tag such as test-v0.3.0-net28-mac16 is 16. Zero when the tag is not
   one of those releases. An iOS tag is zero. A Windows or Linux build-N tag
   has no Mac disk image, so the release list skips it. */
long HaloForkUpdateBuildFromTag(NSString *tag);
/* "main" for build-N and the older mac tags. "testing" for testing-N.
   Nil for any other tag. */
NSString *HaloForkUpdateChannelFromTag(NSString *tag);
NSString *HaloForkUpdateAssetName(NSString *tag);
/* The download URL GitHub gives that tag's disk image, and no other host. */
BOOL HaloForkUpdateDownloadIsTrusted(NSString *tag, NSString *url);
/* The newest Mac disk image in a GitHub releases list. A newer iOS release
   can be the newest tag; it is not this app. Drafts and prereleases are
   skipped. Returns 0 when the list has no Mac disk image. */
long HaloForkNewestMacRelease(NSArray *releases, NSString **tag, NSString **url);
/* The release this app should install. testing selects the newest testing-N
   prerelease. Otherwise the newest main release. A testing app with testing
   off is sent back to that main release even when its number is lower.
   Returns 0 when the app should stay. */
long HaloForkUpdateChoice(NSArray *releases, BOOL testing, NSString *currentChannel, long currentBuild,
    NSString **tag, NSString **url);
/* update.txt beside debug.txt. Nil when this launch has no game data folder. */
NSURL *HaloForkUpdateHistoryURL(NSURL *dataDirectory);
/* The sentence update.txt keeps. The date is added when the line is written. */
NSString *HaloForkUpdateHistoryNote(long fromBuild, long toBuild, BOOL installed);
/* True when update.txt already records this Mac build as the one now installed. */
BOOL HaloForkUpdateHistoryRecordsBuild(NSString *text, long build);
/* Appends one line when this build is not already the installed one. */
void HaloForkUpdateRecordInstalledBuild(NSURL *dataDirectory, long build);

/* update.auto in config.toml. Missing means yes, which is the game's default. */
BOOL HaloConfigAutomaticUpdates(NSString *text);
NSString *HaloConfigSetAutomaticUpdates(NSString *text, BOOL automatic);
/* A hidden, timed, or scripted run is not asked. debug.update_answer is not
   one of those: the caller uses that answer instead of a dialog. */
BOOL HaloConfigSuppressesUpdatePrompt(NSString *text);
NSString *HaloConfigUpdateAnswer(NSString *text);

@interface HaloForkUpdater : NSObject
@property(nonatomic, readonly) long build;
- (instancetype)initWithBuild:(long)build configURL:(NSURL *)configURL bundleURL:(NSURL *)bundleURL;
- (BOOL)automatic;
- (BOOL)setAutomatic:(BOOL)automatic error:(NSError **)error;
- (BOOL)testingReleases;
/* "testing" when this app was packaged as a testing release, otherwise "main". */
- (NSString *)channel;
- (BOOL)suppressesPrompt;
- (NSString *)updateAnswer;
/* completion is on the main queue. download is nil when nothing newer was found. */
- (void)checkQuiet:(BOOL)quiet completion:(void (^)(NSError *error, NSString *tag, NSURL *download, long build))completion;
/* Downloads that disk image, checks it, and leaves a helper that replaces this
   app after this process exits. The caller then quits. */
- (BOOL)installDownload:(NSURL *)download expectedBuild:(long)build historyDirectory:(NSURL *)historyDirectory
    error:(NSError **)error progress:(void (^)(unsigned long long received, unsigned long long total))progress;
@end
