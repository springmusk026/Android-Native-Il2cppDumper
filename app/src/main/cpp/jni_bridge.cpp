#include <jni.h>
#include <string>
#include <cstdarg>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <vector>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "core/metadata.h"
#include "core/il2cpp.h"
#include "core/section_helper.h"
#include "core/il2cpp_executor.h"
#include "core/il2cpp_decompiler.h"
#include "core/struct_generator.h"
#include "core/config.h"

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace il2cpp_dumper;

// Global state
static Metadata* g_metadata = nullptr;
static Il2CppEngine* g_il2Cpp = nullptr;
static Config g_config;
static uint64_t g_dumpAddress = 0;
// Serializes all calls touching g_metadata/g_il2Cpp so nativeCleanup() can't
// free them under a running search/dump.
static std::mutex g_mutex;

// JNI callback for logging
static JavaVM* g_jvm = nullptr;
static jobject g_callback = nullptr;
static jmethodID g_onLogMethod = nullptr;

static void logCallback(const char* msg) {
    JNIEnv* env;
    bool needsDetach = false;
    int status = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needsDetach = true;
    }
    if (env && g_callback && g_onLogMethod) {
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(g_callback, g_onLogMethod, jmsg);
        env->DeleteLocalRef(jmsg);
    }
    if (needsDetach) {
        g_jvm->DetachCurrentThread();
    }
}

// Log to both logcat and Java callback
static void uiLog(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    LOGI("%s", buf);
    logCallback(buf);
}

static bool readWholeFile(const char* path, std::vector<uint8_t>& out) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    bool ok = fseek(f, 0, SEEK_END) == 0;
    long size = ok ? ftell(f) : -1;
    ok = ok && size >= 0 && fseek(f, 0, SEEK_SET) == 0;
    if (ok) {
        out.resize(static_cast<size_t>(size));
        ok = fread(out.data(), 1, out.size(), f) == out.size();
    }
    fclose(f);
    return ok;
}

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeSetCallback(JNIEnv* env, jobject /* thiz */, jobject callback) {
    if (g_callback) {
        env->DeleteGlobalRef(g_callback);
        g_callback = nullptr;
    }
    if (callback) {
        g_callback = env->NewGlobalRef(callback);
        jclass cls = env->GetObjectClass(callback);
        g_onLogMethod = env->GetMethodID(cls, "onLog", "(Ljava/lang/String;)V");
    }
}

JNIEXPORT jboolean JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeInit(
    JNIEnv* env, jobject /* thiz */,
    jstring metadataPath, jstring il2cppPath) {

    std::lock_guard<std::mutex> lock(g_mutex);
    il2cpp_dumper::g_cancelled.store(false);

    // Clean up previous state
    delete g_metadata;
    delete g_il2Cpp;
    g_metadata = nullptr;
    g_il2Cpp = nullptr;

    const char* metaPath = env->GetStringUTFChars(metadataPath, nullptr);
    const char* il2Path = env->GetStringUTFChars(il2cppPath, nullptr);
    LOGI("Initializing: metadata=%s, il2cpp=%s", metaPath, il2Path);
    std::vector<uint8_t> metaBytes, il2Bytes;
    bool readOk = readWholeFile(metaPath, metaBytes) && readWholeFile(il2Path, il2Bytes);
    env->ReleaseStringUTFChars(metadataPath, metaPath);
    env->ReleaseStringUTFChars(il2cppPath, il2Path);
    if (!readOk) {
        uiLog("ERROR: Failed to read input files");
        return JNI_FALSE;
    }
    g_dumpAddress = 0;
    g_config = Config{};

    try {
        // Initialize metadata
        g_metadata = new Metadata(std::move(metaBytes));
        uiLog("Metadata version: %.1f", g_metadata->version);
        uiLog("Images: %zu | Types: %zu | Methods: %zu",
              g_metadata->imageDefs.size(), g_metadata->typeDefs.size(), g_metadata->methodDefs.size());
        uiLog("Fields: %zu | Properties: %zu | Events: %zu",
              g_metadata->fieldDefs.size(), g_metadata->propertyDefs.size(), g_metadata->eventDefs.size());

        // Determine il2cpp format
        if (il2Bytes.size() < 4) {
            uiLog("ERROR: Il2Cpp binary too small (%zu bytes)", il2Bytes.size());
            return JNI_FALSE;
        }

        uint32_t magic;
        std::memcpy(&magic, il2Bytes.data(), 4);

        if (magic == 0x464c457f) { // ELF
            // Check ELF class
            bool isElf64 = (il2Bytes[4] == 2);
            auto il2Size = il2Bytes.size();
            g_il2Cpp = new ElfIl2Cpp(std::move(il2Bytes), isElf64);
            uiLog("ELF loaded: %s (%zu bytes)", isElf64 ? "64-bit" : "32-bit", il2Size);
            if (g_il2Cpp->checkProtection()) {
                uiLog("WARNING: Binary may be protected or obfuscated; results may be incomplete.");
            }
        } else {
            uiLog("ERROR: Unsupported binary format (magic: 0x%08x)", magic);
            return JNI_FALSE;
        }

        g_il2Cpp->setProperties(g_metadata->version, g_metadata->metadataUsagesCount);

        return JNI_TRUE;
    } catch (const std::exception& e) {
        uiLog("ERROR: Init failed - %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT void JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeSetConfig(JNIEnv* env, jobject /* thiz */, jstring configJson) {
    std::lock_guard<std::mutex> lock(g_mutex);
    const char* cfgJson = env->GetStringUTFChars(configJson, nullptr);
    g_config = Config::fromJson(cfgJson);
    env->ReleaseStringUTFChars(configJson, cfgJson);
}

JNIEXPORT jboolean JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeSearch(JNIEnv* env, jobject /* thiz */) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_metadata || !g_il2Cpp) return JNI_FALSE;

    try {
        // Mirrors upstream Program.Init(): the forced version only applies to
        // the binary; metadata keeps the version it was parsed with.
        auto version = g_config.forceIl2CppVersion ? g_config.forceVersion : g_metadata->version;
        g_il2Cpp->setProperties(version, g_metadata->metadataUsagesCount);
        uiLog("Il2Cpp version: %.1f%s", version, g_config.forceIl2CppVersion ? " (forced)" : "");

        if (g_config.forceDump || g_il2Cpp->checkDump()) {
            uiLog("Detected this may be a dump file.");
            if (g_dumpAddress != 0) {
                g_il2Cpp->applyDumpAddress(g_dumpAddress, !g_config.noRedirectedPointer);
                uiLog("Dump address: 0x%llx%s", (unsigned long long)g_dumpAddress,
                      g_config.noRedirectedPointer ? " (pointers not redirected)" : "");
            } else {
                uiLog("No dump address given, continuing as a regular binary");
            }
        }

        int methodCount = 0;
        for (auto& md : g_metadata->methodDefs) {
            if (md.methodIndex >= 0) methodCount++;
        }
        int typeDefCount = static_cast<int>(g_metadata->typeDefs.size());
        int imageCount = static_cast<int>(g_metadata->imageDefs.size());
        uiLog("Searching... (methods=%d, types=%d, images=%d)", methodCount, typeDefCount, imageCount);

        // Each strategy initializes the engine itself on success; a failure in
        // one (e.g. a bad candidate) must not prevent trying the next.
        auto attempt = [](const char* name, auto&& fn) {
            try {
                if (fn()) {
                    uiLog("%s succeeded", name);
                    return true;
                }
                uiLog("%s: not found", name);
            } catch (const std::exception& e) {
                uiLog("%s failed: %s", name, e.what());
            }
            return false;
        };
        bool found = attempt("PlusSearch", [&] { return g_il2Cpp->plusSearch(methodCount, typeDefCount, imageCount); });
        if (!found) found = attempt("Pattern search", [&] { return g_il2Cpp->search(); });
        if (!found) found = attempt("Symbol search", [&] { return g_il2Cpp->symbolSearch(); });

        if (!found) {
            uiLog("ERROR: All search methods failed. Cannot locate registration structures.");
            return JNI_FALSE;
        }

        uiLog("Il2Cpp version: %.1f | Types loaded: %zu | GenericInsts: %zu",
              g_il2Cpp->version, g_il2Cpp->types.size(), g_il2Cpp->genericInsts.size());
        if (!g_il2Cpp->codeGenModules.empty()) {
            uiLog("CodeGenModules: %zu", g_il2Cpp->codeGenModules.size());
        }
        if (!g_il2Cpp->methodPointers.empty()) {
            uiLog("Method pointers: %zu", g_il2Cpp->methodPointers.size());
        }

        // Dumped v27+ binaries reference metadata by absolute address.
        if (g_il2Cpp->version >= 27 && g_il2Cpp->isDumped && !g_metadata->typeDefs.empty()) {
            auto& il2CppType = g_il2Cpp->types.at(g_metadata->typeDefs[0].byvalTypeIndex);
            g_metadata->imageBase = il2CppType.typeHandle() - g_metadata->header.typeDefinitionsOffset;
            uiLog("Metadata ImageBase: 0x%llx", (unsigned long long)g_metadata->imageBase);
        }

        return JNI_TRUE;
    } catch (const std::exception& e) {
        uiLog("ERROR: Search failed - %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeDump(
    JNIEnv* env, jobject /* thiz */,
    jstring outputDir, jstring configJson) {

    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_metadata || !g_il2Cpp) return JNI_FALSE;

    const char* outDir = env->GetStringUTFChars(outputDir, nullptr);
    const char* cfgJson = env->GetStringUTFChars(configJson, nullptr);

    std::string outDirStr(outDir);
    Config config = Config::fromJson(cfgJson);

    env->ReleaseStringUTFChars(outputDir, outDir);
    env->ReleaseStringUTFChars(configJson, cfgJson);

    try {
        Il2CppExecutor executor(*g_metadata, *g_il2Cpp);

        if (config.dumpMethod || config.dumpField || config.dumpProperty) {
            uiLog("Decompiling %zu types to dump.cs...", g_metadata->typeDefs.size());
            Il2CppDecompiler decompiler(executor);
            decompiler.decompile(config, outDirStr);
            if (g_cancelled.load()) {
                uiLog("Dump cancelled");
                return JNI_FALSE;
            }
            uiLog("dump.cs generated");
        }

        if (config.generateStruct) {
            uiLog("Generating script.json + il2cpp.h...");
            StructGenerator gen(executor);
            gen.writeScript(outDirStr);
            uiLog("Struct generation complete");
        }

        uiLog("Output: %s", outDirStr.c_str());
        return JNI_TRUE;
    } catch (const std::exception& e) {
        uiLog("ERROR: Dump failed - %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeGetVersion(JNIEnv* env, jobject /* thiz */) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_metadata) return env->NewStringUTF("Unknown");
    char buf[64];
    snprintf(buf, sizeof(buf), "Metadata: %.1f", g_metadata->version);
    return env->NewStringUTF(buf);
}

// The binary's version after search (may be refined, e.g. 24.4 -> 24.5, 29 -> 29.1).
JNIEXPORT jstring JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeGetIl2CppVersion(JNIEnv* env, jobject /* thiz */) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_il2Cpp) return env->NewStringUTF("Unknown");
    char buf[64];
    snprintf(buf, sizeof(buf), "%.1f", g_il2Cpp->version);
    return env->NewStringUTF(buf);
}

JNIEXPORT jboolean JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeIsDumpFile(JNIEnv* /* env */, jobject /* thiz */) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_il2Cpp) return JNI_FALSE;
    return g_il2Cpp->checkDump() ? JNI_TRUE : JNI_FALSE;
}

// Stored and applied in nativeSearch, after the config (NoRedirectedPointer) is known.
JNIEXPORT void JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeSetDumpAddress(
    JNIEnv* /* env */, jobject /* thiz */, jlong address) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_dumpAddress = static_cast<uint64_t>(address);
    LOGI("Set dump address: 0x%llx", (unsigned long long)address);
}

JNIEXPORT void JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeCleanup(JNIEnv* /* env */, jobject /* thiz */) {
    il2cpp_dumper::g_cancelled.store(true); // stop a running dump so the lock frees quickly
    std::lock_guard<std::mutex> lock(g_mutex);
    delete g_metadata;
    delete g_il2Cpp;
    g_metadata = nullptr;
    g_il2Cpp = nullptr;
}

JNIEXPORT void JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeCancel(JNIEnv* /* env */, jobject /* thiz */) {
    il2cpp_dumper::g_cancelled.store(true);
}

} // extern "C"
