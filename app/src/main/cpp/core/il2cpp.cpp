#include "il2cpp.h"
#include "elf.h"
#include "section_helper.h"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

// --- Struct read() implementations ---

void Il2CppCodeRegistration::read(BinaryStream& stream, double version) {
    if (version <= 24.1) {
        methodPointersCount = stream.readUIntPtr();
        methodPointers = stream.readUIntPtr();
    }
    if (version <= 21) {
        delegateWrappersFromNativeToManagedCount = stream.readUIntPtr();
        delegateWrappersFromNativeToManaged = stream.readUIntPtr();
    }
    if (version >= 22) {
        reversePInvokeWrapperCount = stream.readUIntPtr();
        reversePInvokeWrappers = stream.readUIntPtr();
    }
    if (version <= 22) {
        delegateWrappersFromManagedToNativeCount = stream.readUIntPtr();
        delegateWrappersFromManagedToNative = stream.readUIntPtr();
        marshalingFunctionsCount = stream.readUIntPtr();
        marshalingFunctions = stream.readUIntPtr();
    }
    if (version >= 21 && version <= 22) {
        ccwMarshalingFunctionsCount = stream.readUIntPtr();
        ccwMarshalingFunctions = stream.readUIntPtr();
    }
    genericMethodPointersCount = stream.readUIntPtr();
    genericMethodPointers = stream.readUIntPtr();
    if (version == 24.5 || version >= 27.1) {
        genericAdjustorThunks = stream.readUIntPtr();
    }
    invokerPointersCount = stream.readUIntPtr();
    invokerPointers = stream.readUIntPtr();
    if (version <= 24.5) {
        customAttributeCount = stream.readUIntPtr();
        customAttributeGenerators = stream.readUIntPtr();
    }
    if (version >= 21 && version <= 22) {
        guidCount = stream.readUIntPtr();
        guids = stream.readUIntPtr();
    }
    if (version >= 22) {
        unresolvedVirtualCallCount = stream.readUIntPtr();
        unresolvedVirtualCallPointers = stream.readUIntPtr();
    }
    if (version >= 29.1) {
        unresolvedInstanceCallPointers = stream.readUIntPtr();
        unresolvedStaticCallPointers = stream.readUIntPtr();
    }
    if (version >= 23) {
        interopDataCount = stream.readUIntPtr();
        interopData = stream.readUIntPtr();
    }
    if (version >= 24.3) {
        windowsRuntimeFactoryCount = stream.readUIntPtr();
        windowsRuntimeFactoryTable = stream.readUIntPtr();
    }
    if (version >= 24.2) {
        codeGenModulesCount = stream.readUIntPtr();
        codeGenModules = stream.readUIntPtr();
    }
}

void Il2CppMetadataRegistration::read(BinaryStream& stream, double version) {
    genericClassesCount = stream.readIntPtr();
    genericClasses = stream.readUIntPtr();
    genericInstsCount = stream.readIntPtr();
    genericInsts = stream.readUIntPtr();
    genericMethodTableCount = stream.readIntPtr();
    genericMethodTable = stream.readUIntPtr();
    typesCount = stream.readIntPtr();
    types = stream.readUIntPtr();
    methodSpecsCount = stream.readIntPtr();
    methodSpecs = stream.readUIntPtr();
    if (version <= 16) {
        methodReferencesCount = stream.readIntPtr();
        methodReferences = stream.readUIntPtr();
    }
    fieldOffsetsCount = stream.readIntPtr();
    fieldOffsets = stream.readUIntPtr();
    typeDefinitionsSizesCount = stream.readIntPtr();
    typeDefinitionsSizes = stream.readUIntPtr();
    if (version >= 19) {
        metadataUsagesCount = stream.readUIntPtr();
        metadataUsages = stream.readUIntPtr();
    }
}

void Il2CppType::read(BinaryStream& stream, double /*version*/) {
    datapoint = stream.readUIntPtr();
    bits = stream.readUInt32();
}

void Il2CppGenericClass::read(BinaryStream& stream, double version) {
    if (version <= 24.5) {
        typeDefinitionIndex = stream.readIntPtr();
    } else {
        type = stream.readUIntPtr();
    }
    context_class_inst = stream.readUIntPtr();
    context_method_inst = stream.readUIntPtr();
    cached_class = stream.readUIntPtr();
}

void Il2CppArrayType::read(BinaryStream& stream, double /*version*/) {
    etype = stream.readUIntPtr();
    rank = stream.readByte();
    numsizes = stream.readByte();
    numlobounds = stream.readByte();
    // Padding
    stream.readByte();
    sizes = stream.readUIntPtr();
    lobounds = stream.readUIntPtr();
}

void Il2CppGenericMethodFunctionsDefinitions::read(BinaryStream& stream, double version) {
    genericMethodIndex = stream.readInt32();
    indices_methodIndex = stream.readInt32();
    indices_invokerIndex = stream.readInt32();
    if (version == 24.5 || version >= 27.1) {
        indices_adjustorThunk = stream.readInt32();
    }
}

void Il2CppCodeGenModule::read(BinaryStream& stream, double version) {
    moduleName = stream.readUIntPtr();
    methodPointerCount = stream.readIntPtr();
    methodPointers = stream.readUIntPtr();
    if (version == 24.5 || version >= 27.1) {
        adjustorThunkCount = stream.readIntPtr();
        adjustorThunks = stream.readUIntPtr();
    }
    invokerIndices = stream.readUIntPtr();
    reversePInvokeWrapperCount = stream.readUIntPtr();
    reversePInvokeWrapperIndices = stream.readUIntPtr();
    rgctxRangesCount = stream.readIntPtr();
    rgctxRanges = stream.readUIntPtr();
    rgctxsCount = stream.readIntPtr();
    rgctxs = stream.readUIntPtr();
    debuggerMetadata = stream.readUIntPtr();
    if (version >= 27 && version <= 27.2) {
        customAttributeCacheGenerator = stream.readUIntPtr();
    }
    if (version >= 27) {
        moduleInitializer = stream.readUIntPtr();
        staticConstructorTypeIndices = stream.readUIntPtr();
        metadataRegistration = stream.readUIntPtr();
        codeRegistration = stream.readUIntPtr();
    }
}

// --- Il2CppEngine ---

Il2CppEngine::Il2CppEngine(const uint8_t* data, size_t size, bool isElf64)
    : BinaryStream(data, size) {
    is32Bit = !isElf64;
}

Il2CppEngine::Il2CppEngine(std::vector<uint8_t>&& data, bool isElf64)
    : BinaryStream(std::move(data)) {
    is32Bit = !isElf64;
}

void Il2CppEngine::setProperties(double ver, int64_t muCount) {
    version = ver;
    metadataUsagesCount = muCount;
}

bool Il2CppEngine::autoPlusInit(uint64_t codeRegistration, uint64_t metadataRegistration) {
    if (codeRegistration != 0) {
        uint64_t limit = 0x50000;
        if (version >= 24.2) {
            pCodeRegistration = readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));
            if (version == 31) {
                if (pCodeRegistration.genericMethodPointersCount > limit) {
                    codeRegistration -= getPointerSize() * 2;
                } else {
                    version = 29;
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
            if (version == 29) {
                if (pCodeRegistration.genericMethodPointersCount > limit) {
                    version = 29.1;
                    codeRegistration -= getPointerSize() * 2;
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
            if (version == 27) {
                if (pCodeRegistration.reversePInvokeWrapperCount > limit) {
                    version = 27.1;
                    codeRegistration -= getPointerSize();
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
            if (version == 24.4) {
                codeRegistration -= getPointerSize() * 2;
                if (pCodeRegistration.reversePInvokeWrapperCount > limit) {
                    version = 24.5;
                    codeRegistration -= getPointerSize();
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
            if (version == 24.2) {
                if (pCodeRegistration.interopDataCount == 0) {
                    version = 24.3;
                    codeRegistration -= getPointerSize() * 2;
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
        }
    }

    LOGI("CodeRegistration : %llx", (unsigned long long)codeRegistration);
    LOGI("MetadataRegistration : %llx", (unsigned long long)metadataRegistration);

    if (codeRegistration != 0 && metadataRegistration != 0) {
        init(codeRegistration, metadataRegistration);
        return true;
    }
    return false;
}

void Il2CppEngine::init(uint64_t codeRegistration, uint64_t metadataRegistration) {
    // A previous search strategy may have failed mid-init; start clean.
    methodPointers.clear();
    genericMethodPointers.clear();
    invokerPointers.clear();
    customAttributeGenerators.clear();
    reversePInvokeWrappers.clear();
    unresolvedVirtualCallPointers.clear();
    types.clear();
    metadataUsages.clear();
    genericInsts.clear();
    genericInstPointers.clear();
    methodSpecs.clear();
    methodDefinitionMethodSpecs.clear();
    methodSpecGenericMethodPointers.clear();
    codeGenModules.clear();
    codeGenModuleMethodPointers.clear();
    rgctxsDictionary.clear();
    fieldOffsets.clear();
    typeDic.clear();

    pCodeRegistration =readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));

    uint64_t limit = 0x50000;
    if (version == 27 && pCodeRegistration.invokerPointersCount > limit) {
        version = 27.1;
        LOGI("Change il2cpp version to: %.1f", version);
        pCodeRegistration = readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));
    }

    if (version == 27.1) {
        // 27.2 stores a pointer in the RGCTX data slot instead of an index. If every
        // entry (read with the 27.1 layout) looks like a pointer, upgrade.
        auto pCodeGenModules = readPointerArray(
            mapVATR(pCodeRegistration.codeGenModules), pCodeRegistration.codeGenModulesCount);
        for (auto pCodeGenModule : pCodeGenModules) {
            auto codeGenModule = readClass<Il2CppCodeGenModule>(mapVATR(pCodeGenModule));
            if (codeGenModule.rgctxsCount > 0) {
                auto rgctxs = readClassArray<Il2CppRGCTXDefinition>(
                    mapVATR(codeGenModule.rgctxs), codeGenModule.rgctxsCount);
                bool allPointers = std::all_of(rgctxs.begin(), rgctxs.end(), [limit](const Il2CppRGCTXDefinition& x) {
                    return x.data.rgctxDataDummy > static_cast<int64_t>(limit);
                });
                if (allPointers) {
                    version = 27.2;
                    LOGI("Change il2cpp version to: %.1f", version);
                }
                break;
            }
        }
    }

    if (version == 24.4 && pCodeRegistration.invokerPointersCount > limit) {
        version = 24.5;
        LOGI("Change il2cpp version to: %.1f", version);
        pCodeRegistration = readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));
    }

    if (version == 24.2 && pCodeRegistration.codeGenModules == 0) {
        version = 24.3;
        LOGI("Change il2cpp version to: %.1f", version);
        pCodeRegistration = readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));
    }

    pMetadataRegistration = readClass<Il2CppMetadataRegistration>(mapVATR(metadataRegistration));

    genericMethodPointers = readPointerArray(
        mapVATR(pCodeRegistration.genericMethodPointers), pCodeRegistration.genericMethodPointersCount);
    invokerPointers = readPointerArray(
        mapVATR(pCodeRegistration.invokerPointers), pCodeRegistration.invokerPointersCount);

    if (version < 27) {
        customAttributeGenerators = readPointerArray(
            mapVATR(pCodeRegistration.customAttributeGenerators), pCodeRegistration.customAttributeCount);
    }

    if (version > 16 && version < 27) {
        metadataUsages = readPointerArray(
            mapVATR(pMetadataRegistration.metadataUsages), metadataUsagesCount);
    }

    if (version >= 22) {
        if (pCodeRegistration.reversePInvokeWrapperCount != 0)
            reversePInvokeWrappers = readPointerArray(
                mapVATR(pCodeRegistration.reversePInvokeWrappers), pCodeRegistration.reversePInvokeWrapperCount);
        if (pCodeRegistration.unresolvedVirtualCallCount != 0)
            unresolvedVirtualCallPointers = readPointerArray(
                mapVATR(pCodeRegistration.unresolvedVirtualCallPointers), pCodeRegistration.unresolvedVirtualCallCount);
    }

    genericInstPointers = readPointerArray(
        mapVATR(pMetadataRegistration.genericInsts), pMetadataRegistration.genericInstsCount);
    genericInsts.resize(genericInstPointers.size());
    for (size_t i = 0; i < genericInstPointers.size(); i++) {
        genericInsts[i] = readClass<Il2CppGenericInst>(mapVATR(genericInstPointers[i]));
    }

    fieldOffsetsArePointers = version > 21;
    if (version == 21) {
        auto fieldTest = readPrimitiveArray<uint32_t>(mapVATR(pMetadataRegistration.fieldOffsets), 6);
        fieldOffsetsArePointers = fieldTest[0] == 0 && fieldTest[1] == 0 && fieldTest[2] == 0 &&
                                   fieldTest[3] == 0 && fieldTest[4] == 0 && fieldTest[5] > 0;
    }
    if (fieldOffsetsArePointers) {
        fieldOffsets = readPointerArray(
            mapVATR(pMetadataRegistration.fieldOffsets), pMetadataRegistration.fieldOffsetsCount);
    } else {
        auto offsets32 = readPrimitiveArray<uint32_t>(
            mapVATR(pMetadataRegistration.fieldOffsets), pMetadataRegistration.fieldOffsetsCount);
        fieldOffsets.assign(offsets32.begin(), offsets32.end());
    }

    auto pTypes = readPointerArray(
        mapVATR(pMetadataRegistration.types), pMetadataRegistration.typesCount);
    types.resize(pTypes.size());
    typeDic.clear();
    typeDic.reserve(pTypes.size());
    for (size_t i = 0; i < pTypes.size(); i++) {
        types[i] = readClass<Il2CppType>(mapVATR(pTypes[i]));
        types[i].init(version);
        typeDic[pTypes[i]] = i;
    }

    if (version >= 24.2) {
        auto pCodeGenModules = readPointerArray(
            mapVATR(pCodeRegistration.codeGenModules), pCodeRegistration.codeGenModulesCount);
        for (auto pCodeGenModule : pCodeGenModules) {
            auto codeGenModule = readClass<Il2CppCodeGenModule>(mapVATR(pCodeGenModule));
            auto moduleName = readStringToNull(mapVATR(codeGenModule.moduleName));
            codeGenModules[moduleName] = codeGenModule;
            try {
                codeGenModuleMethodPointers[moduleName] = readPointerArray(
                    mapVATR(codeGenModule.methodPointers), codeGenModule.methodPointerCount);
            } catch (const std::exception&) {
                // Matches upstream: unmapped/invalid table -> all-zero pointers.
                codeGenModuleMethodPointers[moduleName].assign(
                    static_cast<size_t>(std::max<int64_t>(0, std::min<int64_t>(codeGenModule.methodPointerCount, 0x1000000))), 0);
            }
            std::unordered_map<uint32_t, std::vector<Il2CppRGCTXDefinition>> rgctxDefDic;
            if (codeGenModule.rgctxsCount > 0) {
                auto rgctxs = readClassArray<Il2CppRGCTXDefinition>(
                    mapVATR(codeGenModule.rgctxs), codeGenModule.rgctxsCount);
                auto rgctxRanges = readClassArray<Il2CppTokenRangePair>(
                    mapVATR(codeGenModule.rgctxRanges), codeGenModule.rgctxRangesCount);
                for (auto& range : rgctxRanges) {
                    auto start = static_cast<int64_t>(range.range.start);
                    auto length = static_cast<int64_t>(range.range.length);
                    if (start < 0 || length < 0 || start + length > static_cast<int64_t>(rgctxs.size())) continue;
                    rgctxDefDic[range.token] = std::vector<Il2CppRGCTXDefinition>(
                        rgctxs.begin() + start, rgctxs.begin() + start + length);
                }
            }
            rgctxsDictionary[moduleName] = std::move(rgctxDefDic);
        }
    } else {
        methodPointers = readPointerArray(
            mapVATR(pCodeRegistration.methodPointers), pCodeRegistration.methodPointersCount);
    }

    auto genericMethodTable = readClassArray<Il2CppGenericMethodFunctionsDefinitions>(
        mapVATR(pMetadataRegistration.genericMethodTable), pMetadataRegistration.genericMethodTableCount);
    methodSpecs = readClassArray<Il2CppMethodSpec>(
        mapVATR(pMetadataRegistration.methodSpecs), pMetadataRegistration.methodSpecsCount);

    for (auto& table : genericMethodTable) {
        if (table.genericMethodIndex < 0 || static_cast<size_t>(table.genericMethodIndex) >= methodSpecs.size()) continue;
        auto& methodSpec = methodSpecs[table.genericMethodIndex];
        methodDefinitionMethodSpecs[methodSpec.methodDefinitionIndex].push_back(methodSpec);
        uint64_t pointer = 0;
        if (table.indices_methodIndex >= 0 && static_cast<size_t>(table.indices_methodIndex) < genericMethodPointers.size())
            pointer = genericMethodPointers[table.indices_methodIndex];
        methodSpecGenericMethodPointers.emplace(methodSpec, pointer);
    }

    LOGI("Il2Cpp initialized: version=%.1f, types=%zu", version, types.size());
}

Il2CppType* Il2CppEngine::getIl2CppType(uint64_t pointer) {
    auto it = typeDic.find(pointer);
    if (it != typeDic.end()) return &types[it->second];
    return nullptr;
}

uint64_t Il2CppEngine::getMethodPointer(const std::string& imageName, const Il2CppMethodDefinition& methodDef) {
    if (version >= 24.2) {
        auto it = codeGenModuleMethodPointers.find(imageName);
        if (it == codeGenModuleMethodPointers.end()) return 0;
        auto methodToken = methodDef.token;
        auto methodPointerIndex = methodToken & 0x00FFFFFFu;
        if (methodPointerIndex == 0 || methodPointerIndex > it->second.size()) return 0;
        return it->second[methodPointerIndex - 1];
    } else {
        auto methodIndex = methodDef.methodIndex;
        if (methodIndex >= 0 && static_cast<size_t>(methodIndex) < methodPointers.size()) {
            return methodPointers[methodIndex];
        }
    }
    return 0;
}

int Il2CppEngine::getFieldOffsetFromIndex(int typeIndex, int fieldIndexInType, int fieldIndex,
                                            bool isValueType, bool isStatic) {
    try {
        int offset = -1;
        if (fieldOffsetsArePointers) {
            if (typeIndex < 0 || static_cast<size_t>(typeIndex) >= fieldOffsets.size()) return -1;
            auto ptr = fieldOffsets[typeIndex];
            if (ptr > 0) {
                setPosition(mapVATR(ptr) + 4ull * fieldIndexInType);
                offset = readInt32();
            }
        } else {
            if (fieldIndex < 0 || static_cast<size_t>(fieldIndex) >= fieldOffsets.size()) return -1;
            offset = static_cast<int>(fieldOffsets[fieldIndex]);
        }
        if (offset > 0) {
            if (isValueType && !isStatic) {
                offset -= is32Bit ? 8 : 16;
            }
        }
        return offset;
    } catch (...) {
        return -1;
    }
}

int32_t Il2CppEngine::getRGCTXDataIndex(const Il2CppRGCTXDefinition& def) {
    if (version >= 27.2) {
        // _data points at an Il2CppRGCTXDefinitionData (a single int32 index).
        setPosition(mapVATR(def._data));
        return readInt32();
    }
    return def.data.rgctxDataDummy;
}

// --- ElfIl2Cpp ---

ElfIl2Cpp::ElfIl2Cpp(const uint8_t* data, size_t size, bool isElf64)
    : Il2CppEngine(data, size, isElf64) {
    initElf();
}

ElfIl2Cpp::ElfIl2Cpp(std::vector<uint8_t>&& data, bool isElf64)
    : Il2CppEngine(std::move(data), isElf64) {
    initElf();
}

ElfIl2Cpp::~ElfIl2Cpp() {
    delete elfParser;
}

void ElfIl2Cpp::initElf() {
    if (getLength() < 5) throw std::runtime_error("ELF file too small");
    setPosition(4);
    is32Bit = readByte() != 2; // EI_CLASS: 2 = ELFCLASS64

    // The ELF parser shares this stream's buffer, so relocations it applies are
    // visible to the Il2Cpp reader.
    elfParser = new Elf(getDataMutable(), getLength());
    elfParser->version = version;
}

void ElfIl2Cpp::syncParser() {
    elfParser->version = version;
    elfParser->isDumped = isDumped;
    elfParser->imageBase = imageBase;
}

uint64_t ElfIl2Cpp::mapVATR(uint64_t addr) {
    return elfParser->mapVATR(addr);
}

uint64_t ElfIl2Cpp::mapRTVA(uint64_t addr) {
    return elfParser->mapRTVA(addr);
}

bool ElfIl2Cpp::search() {
    syncParser();
    uint64_t codeRegistration = 0, metadataRegistration = 0;
    if (!elfParser->search(codeRegistration, metadataRegistration)) return false;
    init(codeRegistration, metadataRegistration);
    return true;
}

bool ElfIl2Cpp::plusSearch(int methodCount, int typeDefinitionsCount, int imageCount) {
    std::unique_ptr<SectionHelper> helper(getSectionHelper(methodCount, typeDefinitionsCount, imageCount));
    auto codeRegistration = helper->findCodeRegistration();
    auto metadataRegistration = helper->findMetadataRegistration();
    return autoPlusInit(codeRegistration, metadataRegistration);
}

bool ElfIl2Cpp::symbolSearch() {
    syncParser();
    uint64_t codeRegistration = 0, metadataRegistration = 0;
    if (!elfParser->symbolSearch(codeRegistration, metadataRegistration)) return false;
    init(codeRegistration, metadataRegistration);
    return true;
}

SectionHelper* ElfIl2Cpp::getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount) {
    syncParser();
    return elfParser->getSectionHelper(methodCount, typeDefinitionsCount, imageCount, metadataUsagesCount);
}

SectionHelper* ElfIl2Cpp::getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount, int64_t muCount) {
    syncParser();
    return elfParser->getSectionHelper(methodCount, typeDefinitionsCount, imageCount, muCount);
}

bool ElfIl2Cpp::checkDump() {
    return elfParser->checkDump();
}

uint64_t ElfIl2Cpp::getRVA(uint64_t pointer) {
    syncParser();
    return elfParser->getRVA(pointer);
}

bool ElfIl2Cpp::checkProtection() {
    return elfParser->isProtected;
}

void ElfIl2Cpp::applyDumpAddress(uint64_t address, bool redirect) {
    Il2CppEngine::applyDumpAddress(address, redirect);
    syncParser();
    if (redirect) {
        elfParser->reload();
    }
}

} // namespace il2cpp_dumper
