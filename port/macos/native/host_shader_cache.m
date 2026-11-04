/* The Mac host's shader files. The ANGLE cache is the one iOS keeps in
   port/apple/host_shader_cache.m. This build deletes CoreCE's folder if a
   previous host left it, and the Metal warmup beside the saves. */
#import <Foundation/Foundation.h>

void halo_update_discard_shader_caches(const char *save_root)
{
    NSString *root = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).firstObject
        stringByAppendingPathComponent:@"CoreCE/ShaderCache-v1"];

    [NSFileManager.defaultManager removeItemAtPath:root error:nil];
    if (!save_root || !save_root[0])
        return;
    [NSFileManager.defaultManager removeItemAtPath:
        [[NSString stringWithUTF8String:save_root] stringByAppendingPathComponent:@"Cache/MetalWarmup-v1.bin"]
        error:nil];
}
