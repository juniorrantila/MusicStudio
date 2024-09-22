#include <Ty/StringBuffer.h>
#include <Ty/Vector.h>

#include <dlfcn.h>
#import <Foundation/Foundation.h>

ErrorOr<Vector<StringBuffer>> find_vst_plugins() {
    auto* plugins = [[NSMutableArray<NSString*> alloc] init];
    {
        auto paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask|NSLocalDomainMask|NSSystemDomainMask, YES);
        for (NSString* path in paths) {
            auto plugins_directory = [NSString pathWithComponents:@[path, @"Audio", @"Plug-Ins", @"VST"]];
            auto* urls = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:plugins_directory error:nil];
            for (NSString* filename in urls) {
                if ([filename.pathExtension caseInsensitiveCompare:@".vst"] == NSOrderedSame)
                    continue;
                if (dlopen_preflight(filename.cString)) {
                    void* handle = dlopen(filename.cString, RTLD_LOCAL|RTLD_LAZY);
                    if (handle == nullptr)
                        continue;
                    if (dlsym(handle, "VSTPluginMain"))
                        [plugins addObject:filename];
                    dlclose(handle);
                }
            }
        }
    }

    auto result = TRY(Vector<StringBuffer>::create(plugins.count));
    for (NSString* plugin in plugins) {
        auto plugin_path = StringView::from_c_string(plugin.cString);
        TRY(result.append(StringBuffer::create_fill(plugin_path)));
    }
    return result;
}
