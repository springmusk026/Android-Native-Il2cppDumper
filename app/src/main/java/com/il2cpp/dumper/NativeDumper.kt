package com.il2cpp.dumper

object NativeDumper {

    init {
        System.loadLibrary("dumper")
    }

    interface LogCallback {
        fun onLog(message: String)
    }

    external fun nativeSetCallback(callback: LogCallback?)
    external fun nativeInit(metadataPath: String, il2cppPath: String): Boolean
    external fun nativeSetConfig(configJson: String)
    external fun nativeSearch(): Boolean
    external fun nativeDump(outputDir: String, configJson: String): Boolean
    external fun nativeGetVersion(): String
    external fun nativeGetIl2CppVersion(): String
    external fun nativeIsDumpFile(): Boolean
    external fun nativeSetDumpAddress(address: Long)
    external fun nativeCancel()
    external fun nativeCleanup()
}
