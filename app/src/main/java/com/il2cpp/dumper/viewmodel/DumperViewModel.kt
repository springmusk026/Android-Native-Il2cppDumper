package com.il2cpp.dumper.viewmodel

import android.app.Application
import android.net.Uri
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.il2cpp.dumper.NativeDumper
import com.il2cpp.dumper.data.db.DumpJobEntity
import com.il2cpp.dumper.data.db.JobStatus
import com.il2cpp.dumper.data.repository.DumpJobRepository
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File

data class DumpConfig(
    val dumpMethod: Boolean = true,
    val dumpField: Boolean = true,
    val dumpProperty: Boolean = false,
    val dumpAttribute: Boolean = false,
    val dumpFieldOffset: Boolean = true,
    val dumpMethodOffset: Boolean = true,
    val dumpTypeDefIndex: Boolean = true,
    val generateStruct: Boolean = true,
    val forceIl2CppVersion: Boolean = false,
    val forceVersion: Double = 24.3,
    val forceDump: Boolean = false,
    val noRedirectedPointer: Boolean = false
) {
    fun toJson(): String {
        val json = JSONObject()
        json.put("DumpMethod", dumpMethod)
        json.put("DumpField", dumpField)
        json.put("DumpProperty", dumpProperty)
        json.put("DumpAttribute", dumpAttribute)
        json.put("DumpFieldOffset", dumpFieldOffset)
        json.put("DumpMethodOffset", dumpMethodOffset)
        json.put("DumpTypeDefIndex", dumpTypeDefIndex)
        json.put("GenerateStruct", generateStruct)
        json.put("ForceIl2CppVersion", forceIl2CppVersion)
        json.put("ForceVersion", forceVersion)
        json.put("ForceDump", forceDump)
        json.put("NoRedirectedPointer", noRedirectedPointer)
        json.put("GenerateDummyDll", false)
        return json.toString()
    }
}

sealed class DumpState {
    data object Idle : DumpState()
    data object Initializing : DumpState()
    data object Searching : DumpState()
    data object Dumping : DumpState()
    data class Success(val outputDir: String, val jobId: Long, val durationMs: Long) : DumpState()
    data class Error(val message: String) : DumpState()
}

class DumperViewModel(
    application: Application,
    private val repository: DumpJobRepository
) : AndroidViewModel(application) {

    private val _state = MutableStateFlow<DumpState>(DumpState.Idle)
    val state: StateFlow<DumpState> = _state.asStateFlow()

    private val _logs = MutableStateFlow<List<String>>(emptyList())
    val logs: StateFlow<List<String>> = _logs.asStateFlow()

    private val _config = MutableStateFlow(DumpConfig())
    val config: StateFlow<DumpConfig> = _config.asStateFlow()

    private val _metadataUri = MutableStateFlow<Uri?>(null)
    val metadataUri: StateFlow<Uri?> = _metadataUri.asStateFlow()

    private val _il2cppUri = MutableStateFlow<Uri?>(null)
    val il2cppUri: StateFlow<Uri?> = _il2cppUri.asStateFlow()

    /** Display name and size of a picked file, resolved from the content provider. */
    data class PickedFile(val name: String, val size: Long)

    private val _metadataFile = MutableStateFlow<PickedFile?>(null)
    val metadataFile: StateFlow<PickedFile?> = _metadataFile.asStateFlow()

    private val _il2cppFile = MutableStateFlow<PickedFile?>(null)
    val il2cppFile: StateFlow<PickedFile?> = _il2cppFile.asStateFlow()

    /** Wall-clock start of the running dump, for the elapsed-time readout. */
    private val _startedAt = MutableStateFlow(0L)
    val startedAt: StateFlow<Long> = _startedAt.asStateFlow()

    private val _isDumpFile = MutableStateFlow(false)
    val isDumpFile: StateFlow<Boolean> = _isDumpFile.asStateFlow()

    private val _dumpAddress = MutableStateFlow<Long?>(null)
    val dumpAddress: StateFlow<Long?> = _dumpAddress.asStateFlow()

    private var currentJobId: Long = 0

    private val logCallback = object : NativeDumper.LogCallback {
        override fun onLog(message: String) {
            _logs.update { it + message } // called from the native worker thread
        }
    }

    init {
        NativeDumper.nativeSetCallback(logCallback)
    }

    fun setMetadataUri(uri: Uri) {
        _metadataUri.value = uri
        _metadataFile.value = queryFile(uri)
    }

    fun setIl2cppUri(uri: Uri) {
        _il2cppUri.value = uri
        _il2cppFile.value = queryFile(uri)
    }

    private fun queryFile(uri: Uri): PickedFile {
        val context = getApplication<Application>()
        var name = uri.lastPathSegment ?: "file"
        var size = -1L
        context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val nameIndex = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
                val sizeIndex = cursor.getColumnIndex(android.provider.OpenableColumns.SIZE)
                if (nameIndex >= 0) name = cursor.getString(nameIndex) ?: name
                if (sizeIndex >= 0 && !cursor.isNull(sizeIndex)) size = cursor.getLong(sizeIndex)
            }
        }
        return PickedFile(name, size)
    }

    fun updateConfig(config: DumpConfig) {
        _config.value = config
    }

    fun setDumpAddress(address: Long) {
        _dumpAddress.value = address
    }

    fun startDump() {
        val metaUri = _metadataUri.value
        val il2Uri = _il2cppUri.value
        if (metaUri == null || il2Uri == null) {
            _state.value = DumpState.Error("Please select both files")
            return
        }

        viewModelScope.launch {
            _logs.value = emptyList()
            _state.value = DumpState.Initializing

            val startTime = System.currentTimeMillis()
            _startedAt.value = startTime
            val context = getApplication<Application>()

            // Get file info for the job
            val metaFileName = getFileName(metaUri)
            val il2FileName = getFileName(il2Uri)

            // Create job entity
            val job = DumpJobEntity(
                timestamp = startTime,
                metadataFileName = metaFileName,
                il2cppFileName = il2FileName,
                configJson = _config.value.toJson(),
                status = JobStatus.RUNNING
            )
            currentJobId = repository.insertJob(job)

            try {
                // Copy files to internal storage
                addLog("Loading files...")
                val metaFile = copyUriToFile(metaUri, "global-metadata.dat")
                val il2File = copyUriToFile(il2Uri, "libil2cpp.so")
                addLog("├─ $metaFileName (${formatSize(metaFile.length())})")
                addLog("└─ $il2FileName (${formatSize(il2File.length())})")

                // Compute hashes
                val metaHash = DumpJobRepository.computeFileHash(metaFile)
                val il2Hash = DumpJobRepository.computeFileHash(il2File)

                addLog("Initializing native engine...")
                val initResult = withContext(Dispatchers.IO) {
                    NativeDumper.nativeInit(metaFile.absolutePath, il2File.absolutePath)
                }
                if (!initResult) {
                    failJob("Failed to initialize", startTime, metaFileName, il2FileName, metaHash, il2Hash, metaFile.length(), il2File.length())
                    return@launch
                }

                val versionStr = NativeDumper.nativeGetVersion()

                val isDump = withContext(Dispatchers.IO) { NativeDumper.nativeIsDumpFile() }
                _isDumpFile.value = isDump
                if (isDump || _config.value.forceDump) {
                    val addr = _dumpAddress.value
                    if (addr != null && addr != 0L) {
                        withContext(Dispatchers.IO) { NativeDumper.nativeSetDumpAddress(addr) }
                        addLog("Using dump address: 0x${addr.toString(16)}")
                    }
                }

                _state.value = DumpState.Searching
                withContext(Dispatchers.IO) {
                    NativeDumper.nativeSetConfig(_config.value.toJson())
                }
                val searchResult = withContext(Dispatchers.IO) {
                    NativeDumper.nativeSearch()
                }
                if (!searchResult) {
                    failJob("Failed to find registration structures", startTime, metaFileName, il2FileName, metaHash, il2Hash, metaFile.length(), il2File.length())
                    return@launch
                }
                val il2cppVersionStr = NativeDumper.nativeGetIl2CppVersion()

                _state.value = DumpState.Dumping

                // Create job-specific output directory
                val jobOutputDir = File(context.filesDir, "jobs/$currentJobId")
                jobOutputDir.mkdirs()

                addLog("Dumping...")
                val dumpResult = withContext(Dispatchers.IO) {
                    NativeDumper.nativeDump(jobOutputDir.absolutePath, _config.value.toJson())
                }

                val duration = System.currentTimeMillis() - startTime

                if (dumpResult) {
                    val outputFiles = jobOutputDir.listFiles()?.map { it.name } ?: emptyList()
                    val entity = DumpJobEntity(
                        id = currentJobId,
                        timestamp = startTime,
                        metadataFileName = metaFileName,
                        il2cppFileName = il2FileName,
                        metadataHash = metaHash,
                        il2cppHash = il2Hash,
                        metadataSize = metaFile.length(),
                        il2cppSize = il2File.length(),
                        configJson = _config.value.toJson(),
                        status = JobStatus.SUCCESS,
                        outputDirPath = jobOutputDir.absolutePath,
                        logs = _logs.value,
                        durationMs = duration,
                        metadataVersion = parseVersion(versionStr),
                        il2cppVersion = il2cppVersionStr,
                        outputFileNames = outputFiles
                    )
                    repository.updateJob(entity)
                    _state.value = DumpState.Success(jobOutputDir.absolutePath, currentJobId, duration)
                    addLog("Done! ${outputFiles.size} file(s) in ${duration}ms")
                } else {
                    failJob("Dump failed", startTime, metaFileName, il2FileName, metaHash, il2Hash, metaFile.length(), il2File.length(), duration)
                }
            } catch (e: Exception) {
                val duration = System.currentTimeMillis() - startTime
                failJob(e.message ?: "Unknown error", startTime, metaFileName, il2FileName, null, null, 0, 0, duration)
            } finally {
                // Clean up temp files
                File(context.cacheDir, "global-metadata.dat").delete()
                File(context.cacheDir, "libil2cpp.so").delete()
            }
        }
    }

    private suspend fun failJob(
        error: String,
        startTime: Long,
        metaFileName: String,
        il2FileName: String,
        metaHash: String?,
        il2Hash: String?,
        metaSize: Long,
        il2Size: Long,
        duration: Long = System.currentTimeMillis() - startTime
    ) {
        _state.value = DumpState.Error(error)
        addLog("Error: $error")
        val entity = DumpJobEntity(
            id = currentJobId,
            timestamp = startTime,
            metadataFileName = metaFileName,
            il2cppFileName = il2FileName,
            metadataHash = metaHash,
            il2cppHash = il2Hash,
            metadataSize = metaSize,
            il2cppSize = il2Size,
            configJson = _config.value.toJson(),
            status = JobStatus.FAILED,
            logs = _logs.value,
            durationMs = duration,
            errorMessage = error
        )
        repository.updateJob(entity)
    }

    private fun addLog(message: String) {
        _logs.update { it + message }
    }

    private suspend fun copyUriToFile(uri: Uri, fileName: String): File {
        return withContext(Dispatchers.IO) {
            val context = getApplication<Application>()
            val file = File(context.cacheDir, fileName)
            context.contentResolver.openInputStream(uri)?.use { input ->
                file.outputStream().use { output ->
                    input.copyTo(output)
                }
            }
            file
        }
    }

    private fun getFileName(uri: Uri): String {
        val context = getApplication<Application>()
        var name = "unknown"
        context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            val nameIndex = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
            if (cursor.moveToFirst() && nameIndex >= 0) {
                name = cursor.getString(nameIndex) ?: "unknown"
            }
        }
        return name
    }

    private fun parseVersion(versionStr: String): Double? {
        val match = Regex("""[\d.]+""").find(versionStr)
        return match?.value?.toDoubleOrNull()
    }

    private fun formatSize(bytes: Long): String = when {
        bytes >= 1_048_576 -> "%.1f MB".format(bytes / 1_048_576.0)
        bytes >= 1024 -> "%.1f KB".format(bytes / 1024.0)
        else -> "$bytes B"
    }

    fun reset() {
        _state.value = DumpState.Idle
        _logs.value = emptyList()
        _isDumpFile.value = false
        _dumpAddress.value = null
        NativeDumper.nativeCleanup()
    }

    override fun onCleared() {
        super.onCleared()
        NativeDumper.nativeSetCallback(null)
        NativeDumper.nativeCleanup()
    }
}
