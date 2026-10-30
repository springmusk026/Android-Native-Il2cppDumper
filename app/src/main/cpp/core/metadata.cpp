#include "metadata.h"
#include <algorithm>
#include <stdexcept>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

// --- Struct read() implementations ---

void Il2CppGlobalMetadataHeader::read(BinaryStream& stream, double version) {
    sanity = stream.readUInt32();
    this->version = stream.readInt32();
    stringLiteralOffset = stream.readUInt32();
    stringLiteralSize = stream.readInt32();
    stringLiteralDataOffset = stream.readUInt32();
    stringLiteralDataSize = stream.readInt32();
    stringOffset = stream.readUInt32();
    stringSize = stream.readInt32();
    eventsOffset = stream.readUInt32();
    eventsSize = stream.readInt32();
    propertiesOffset = stream.readUInt32();
    propertiesSize = stream.readInt32();
    methodsOffset = stream.readUInt32();
    methodsSize = stream.readInt32();
    parameterDefaultValuesOffset = stream.readUInt32();
    parameterDefaultValuesSize = stream.readInt32();
    fieldDefaultValuesOffset = stream.readUInt32();
    fieldDefaultValuesSize = stream.readInt32();
    fieldAndParameterDefaultValueDataOffset = stream.readUInt32();
    fieldAndParameterDefaultValueDataSize = stream.readInt32();
    fieldMarshaledSizesOffset = stream.readInt32();
    fieldMarshaledSizesSize = stream.readInt32();
    parametersOffset = stream.readUInt32();
    parametersSize = stream.readInt32();
    fieldsOffset = stream.readUInt32();
    fieldsSize = stream.readInt32();
    genericParametersOffset = stream.readUInt32();
    genericParametersSize = stream.readInt32();
    genericParameterConstraintsOffset = stream.readUInt32();
    genericParameterConstraintsSize = stream.readInt32();
    genericContainersOffset = stream.readUInt32();
    genericContainersSize = stream.readInt32();
    nestedTypesOffset = stream.readUInt32();
    nestedTypesSize = stream.readInt32();
    interfacesOffset = stream.readUInt32();
    interfacesSize = stream.readInt32();
    vtableMethodsOffset = stream.readUInt32();
    vtableMethodsSize = stream.readInt32();
    interfaceOffsetsOffset = stream.readInt32();
    interfaceOffsetsSize = stream.readInt32();
    typeDefinitionsOffset = stream.readUInt32();
    typeDefinitionsSize = stream.readInt32();

    if (version <= 24.1) {
        rgctxEntriesOffset = stream.readUInt32();
        rgctxEntriesCount = stream.readInt32();
    }

    imagesOffset = stream.readUInt32();
    imagesSize = stream.readInt32();
    assembliesOffset = stream.readUInt32();
    assembliesSize = stream.readInt32();

    if (version >= 19 && version <= 24.5) {
        metadataUsageListsOffset = stream.readUInt32();
        metadataUsageListsCount = stream.readInt32();
        metadataUsagePairsOffset = stream.readUInt32();
        metadataUsagePairsCount = stream.readInt32();
    }

    if (version >= 19) {
        fieldRefsOffset = stream.readUInt32();
        fieldRefsSize = stream.readInt32();
    }

    if (version >= 20) {
        referencedAssembliesOffset = stream.readInt32();
        referencedAssembliesSize = stream.readInt32();
    }

    if (version >= 21 && version <= 27.2) {
        attributesInfoOffset = stream.readUInt32();
        attributesInfoCount = stream.readInt32();
        attributeTypesOffset = stream.readUInt32();
        attributeTypesCount = stream.readInt32();
    }

    if (version >= 29) {
        attributeDataOffset = stream.readUInt32();
        attributeDataSize = stream.readInt32();
        attributeDataRangeOffset = stream.readUInt32();
        attributeDataRangeSize = stream.readInt32();
    }

    if (version >= 22) {
        unresolvedVirtualCallParameterTypesOffset = stream.readInt32();
        unresolvedVirtualCallParameterTypesSize = stream.readInt32();
        unresolvedVirtualCallParameterRangesOffset = stream.readInt32();
        unresolvedVirtualCallParameterRangesSize = stream.readInt32();
    }

    if (version >= 23) {
        windowsRuntimeTypeNamesOffset = stream.readInt32();
        windowsRuntimeTypeNamesSize = stream.readInt32();
    }

    if (version >= 27) {
        windowsRuntimeStringsOffset = stream.readInt32();
        windowsRuntimeStringsSize = stream.readInt32();
    }

    if (version >= 24) {
        exportedTypeDefinitionsOffset = stream.readInt32();
        exportedTypeDefinitionsSize = stream.readInt32();
    }
}

void Il2CppImageDefinition::read(BinaryStream& stream, double version) {
    nameIndex = stream.readUInt32();
    assemblyIndex = stream.readInt32();
    typeStart = stream.readInt32();
    typeCount = stream.readUInt32();
    if (version >= 24) {
        exportedTypeStart = stream.readInt32();
        exportedTypeCount = stream.readUInt32();
    }
    entryPointIndex = stream.readInt32();
    if (version >= 19) {
        token = stream.readUInt32();
    }
    if (version >= 24.1) {
        customAttributeStart = stream.readInt32();
        customAttributeCount = stream.readUInt32();
    }
}

void Il2CppAssemblyDefinition::read(BinaryStream& stream, double version) {
    imageIndex = stream.readInt32();
    if (version >= 24.1) {
        token = stream.readUInt32();
    } else {
        customAttributeIndex = stream.readInt32();
    }
    if (version >= 20) {
        referencedAssemblyStart = stream.readInt32();
        referencedAssemblyCount = stream.readInt32();
    }
    aname_nameIndex = stream.readUInt32();
    aname_cultureIndex = stream.readUInt32();
    if (version <= 24.3) {
        aname_hashValueIndex = stream.readInt32();
    }
    aname_publicKeyIndex = stream.readUInt32();
    aname_hash_alg = stream.readUInt32();
    aname_hash_len = stream.readInt32();
    aname_flags = stream.readUInt32();
    aname_major = stream.readInt32();
    aname_minor = stream.readInt32();
    aname_build = stream.readInt32();
    aname_revision = stream.readInt32();
    auto tokenBytes = stream.readBytes(8);
    std::memcpy(aname_public_key_token, tokenBytes.data(), 8);
}

void Il2CppTypeDefinition::read(BinaryStream& stream, double version) {
    nameIndex = stream.readUInt32();
    namespaceIndex = stream.readUInt32();
    if (version <= 24) {
        customAttributeIndex = stream.readInt32();
    }
    byvalTypeIndex = stream.readInt32();
    if (version <= 24.5) {
        byrefTypeIndex = stream.readInt32();
    }
    declaringTypeIndex = stream.readInt32();
    parentIndex = stream.readInt32();
    elementTypeIndex = stream.readInt32();
    if (version <= 24.1) {
        rgctxStartIndex = stream.readInt32();
        rgctxCount = stream.readInt32();
    }
    genericContainerIndex = stream.readInt32();
    if (version <= 22) {
        delegateWrapperFromManagedToNativeIndex = stream.readInt32();
        marshalingFunctionsIndex = stream.readInt32();
    }
    if (version >= 21 && version <= 22) {
        ccwFunctionIndex = stream.readInt32();
        guidIndex = stream.readInt32();
    }
    flags = stream.readUInt32();
    fieldStart = stream.readInt32();
    methodStart = stream.readInt32();
    eventStart = stream.readInt32();
    propertyStart = stream.readInt32();
    nestedTypesStart = stream.readInt32();
    interfacesStart = stream.readInt32();
    vtableStart = stream.readInt32();
    interfaceOffsetsStart = stream.readInt32();
    method_count = stream.readUInt16();
    property_count = stream.readUInt16();
    field_count = stream.readUInt16();
    event_count = stream.readUInt16();
    nested_type_count = stream.readUInt16();
    vtable_count = stream.readUInt16();
    interfaces_count = stream.readUInt16();
    interface_offsets_count = stream.readUInt16();
    bitfield = stream.readUInt32();
    if (version >= 19) {
        token = stream.readUInt32();
    }
}

void Il2CppMethodDefinition::read(BinaryStream& stream, double version) {
    nameIndex = stream.readUInt32();
    declaringType = stream.readInt32();
    returnType = stream.readInt32();
    if (version >= 31) {
        returnParameterToken = stream.readInt32();
    }
    parameterStart = stream.readInt32();
    if (version <= 24) {
        customAttributeIndex = stream.readInt32();
    }
    genericContainerIndex = stream.readInt32();
    if (version <= 24.1) {
        methodIndex = stream.readInt32();
        invokerIndex = stream.readInt32();
        delegateWrapperIndex = stream.readInt32();
        rgctxStartIndex = stream.readInt32();
        rgctxCount = stream.readInt32();
    }
    token = stream.readUInt32();
    flags = stream.readUInt16();
    iflags = stream.readUInt16();
    slot = stream.readUInt16();
    parameterCount = stream.readUInt16();
}

void Il2CppParameterDefinition::read(BinaryStream& stream, double version) {
    nameIndex = stream.readUInt32();
    token = stream.readUInt32();
    if (version <= 24) {
        customAttributeIndex = stream.readInt32();
    }
    typeIndex = stream.readInt32();
}

void Il2CppFieldDefinition::read(BinaryStream& stream, double version) {
    nameIndex = stream.readUInt32();
    typeIndex = stream.readInt32();
    if (version <= 24) {
        customAttributeIndex = stream.readInt32();
    }
    if (version >= 19) {
        token = stream.readUInt32();
    }
}

void Il2CppPropertyDefinition::read(BinaryStream& stream, double version) {
    nameIndex = stream.readUInt32();
    get = stream.readInt32();
    set = stream.readInt32();
    attrs = stream.readUInt32();
    if (version <= 24) {
        customAttributeIndex = stream.readInt32();
    }
    if (version >= 19) {
        token = stream.readUInt32();
    }
}

void Il2CppCustomAttributeTypeRange::read(BinaryStream& stream, double version) {
    if (version >= 24.1) {
        token = stream.readUInt32();
    }
    start = stream.readInt32();
    count = stream.readInt32();
}

void Il2CppEventDefinition::read(BinaryStream& stream, double version) {
    nameIndex = stream.readUInt32();
    typeIndex = stream.readInt32();
    add = stream.readInt32();
    remove = stream.readInt32();
    raise = stream.readInt32();
    if (version <= 24) {
        customAttributeIndex = stream.readInt32();
    }
    if (version >= 19) {
        token = stream.readUInt32();
    }
}

void Il2CppRGCTXDefinition::read(BinaryStream& stream, double version) {
    if (version >= 27.2) {
        // { Il2CppRGCTXDataType type; const void* data; } - the enum is padded
        // to pointer size, so both members are pointer-sized (4 bytes on 32-bit).
        type_post29 = stream.readUIntPtr();
        _data = stream.readUIntPtr();
    } else {
        type_pre29 = stream.readInt32();
        data.rgctxDataDummy = stream.readInt32();
    }
}

// --- Metadata class implementation ---

template<>
int Metadata::sizeOfStruct<Il2CppImageDefinition>() const {
    int size = 8; // nameIndex + assemblyIndex
    size += 8; // typeStart + typeCount
    if (version >= 24) size += 8; // exportedTypeStart + exportedTypeCount
    size += 4; // entryPointIndex
    if (version >= 19) size += 4; // token
    if (version >= 24.1) size += 8; // customAttributeStart + customAttributeCount
    return size;
}

template<>
int Metadata::sizeOfStruct<Il2CppAssemblyDefinition>() const {
    int size = 4; // imageIndex
    if (version >= 24.1) size += 4; // token
    else size += 4; // customAttributeIndex
    if (version >= 20) size += 8; // referencedAssemblyStart + Count
    size += 8; // nameIndex + cultureIndex
    if (version <= 24.3) size += 4; // hashValueIndex
    size += 4 + 4 + 4 + 4 + 4*4 + 8; // publicKeyIndex, hash_alg, hash_len, flags, major/minor/build/revision, public_key_token
    return size;
}

template<>
int Metadata::sizeOfStruct<Il2CppTypeDefinition>() const {
    int size = 8; // nameIndex + namespaceIndex
    if (version <= 24) size += 4;
    size += 4; // byvalTypeIndex
    if (version <= 24.5) size += 4;
    size += 4 + 4 + 4; // declaringType, parent, elementType
    if (version <= 24.1) size += 8;
    size += 4; // genericContainerIndex
    if (version <= 22) size += 8;
    if (version >= 21 && version <= 22) size += 8;
    size += 4 + 4*8 + 2*8 + 4; // flags, 8 ints, 8 shorts, bitfield
    if (version >= 19) size += 4;
    return size;
}

template<>
int Metadata::sizeOfStruct<Il2CppMethodDefinition>() const {
    int size = 4 + 4 + 4; // nameIndex, declaringType, returnType
    if (version >= 31) size += 4;
    size += 4; // parameterStart
    if (version <= 24) size += 4;
    size += 4; // genericContainerIndex
    if (version <= 24.1) size += 5*4;
    size += 4 + 2*4; // token, flags, iflags, slot, parameterCount
    return size;
}

template<>
int Metadata::sizeOfStruct<Il2CppParameterDefinition>() const {
    int size = 4 + 4; // nameIndex + token
    if (version <= 24) size += 4;
    size += 4; // typeIndex
    return size;
}

template<>
int Metadata::sizeOfStruct<Il2CppFieldDefinition>() const {
    int size = 4 + 4; // nameIndex + typeIndex
    if (version <= 24) size += 4;
    if (version >= 19) size += 4;
    return size;
}

template<>
int Metadata::sizeOfStruct<Il2CppFieldDefaultValue>() const { return 12; }

template<>
int Metadata::sizeOfStruct<Il2CppPropertyDefinition>() const {
    int size = 4 + 4 + 4 + 4; // nameIndex, get, set, attrs
    if (version <= 24) size += 4;
    if (version >= 19) size += 4;
    return size;
}

template<>
int Metadata::sizeOfStruct<Il2CppCustomAttributeTypeRange>() const {
    int size = 4 + 4; // start + count
    if (version >= 24.1) size += 4; // token
    return size;
}

template<>
int Metadata::sizeOfStruct<Il2CppMetadataUsageList>() const { return 8; }

template<>
int Metadata::sizeOfStruct<Il2CppMetadataUsagePair>() const { return 8; }

template<>
int Metadata::sizeOfStruct<Il2CppStringLiteral>() const { return 8; }

template<>
int Metadata::sizeOfStruct<Il2CppParameterDefaultValue>() const { return 12; }

template<>
int Metadata::sizeOfStruct<Il2CppEventDefinition>() const {
    int size = 4 + 4 + 4 + 4 + 4; // nameIndex, typeIndex, add, remove, raise
    if (version <= 24) size += 4;
    if (version >= 19) size += 4;
    return size;
}

template<>
int Metadata::sizeOfStruct<Il2CppGenericContainer>() const { return 16; }

template<>
int Metadata::sizeOfStruct<Il2CppFieldRef>() const { return 8; }

template<>
int Metadata::sizeOfStruct<Il2CppGenericParameter>() const { return 16; }

template<>
int Metadata::sizeOfStruct<Il2CppRGCTXDefinition>() const {
    // Only read from metadata for version <= 24.1 (8 bytes); later versions
    // live in the binary and are pointer-sized.
    if (version >= 27.2) return 16;
    return 8;
}

template<>
int Metadata::sizeOfStruct<Il2CppCustomAttributeDataRange>() const { return 8; }

template<typename T>
std::vector<T> Metadata::readMetadataClassArray(uint32_t addr, int32_t size) {
    int elemSize = sizeOfStruct<T>();
    if (elemSize == 0) return {};
    int count = size / elemSize;
    setPosition(addr);
    return readClassArray<T>(count);
}

Metadata::Metadata(const uint8_t* data, size_t size)
    : BinaryStream(data, size) {
    initialize();
}

Metadata::Metadata(std::vector<uint8_t>&& data)
    : BinaryStream(std::move(data)) {
    initialize();
}

void Metadata::initialize() {
    auto sanity = readUInt32();
    if (sanity != 0xFAB11BAF) {
        throw std::runtime_error("ERROR: Metadata file supplied is not valid metadata file.");
    }
    auto ver = readInt32();
    if (ver < 0 || ver > 1000) {
        throw std::runtime_error("ERROR: Metadata file supplied is not valid metadata file.");
    }
    if (ver < 16 || ver > 31) {
        throw std::runtime_error("ERROR: Metadata version not supported.");
    }
    version = ver;
    header = readClass<Il2CppGlobalMetadataHeader>(0);

    if (version == 24) {
        if (header.stringLiteralOffset == 264) {
            version = 24.2;
            header = readClass<Il2CppGlobalMetadataHeader>(0);
        } else {
            imageDefs = readMetadataClassArray<Il2CppImageDefinition>(header.imagesOffset, header.imagesSize);
            bool hasNonOneToken = false;
            for (auto& img : imageDefs) {
                if (img.token != 1) { hasNonOneToken = true; break; }
            }
            if (hasNonOneToken) version = 24.1;
        }
    }

    imageDefs = readMetadataClassArray<Il2CppImageDefinition>(header.imagesOffset, header.imagesSize);

    if (version == 24.2 && header.assembliesSize / 68 < static_cast<int>(imageDefs.size())) {
        version = 24.4;
    }

    bool v241Plus = false;
    if (version == 24.1 && header.assembliesSize / 64 == static_cast<int>(imageDefs.size())) {
        v241Plus = true;
    }
    if (v241Plus) version = 24.4;

    assemblyDefs = readMetadataClassArray<Il2CppAssemblyDefinition>(header.assembliesOffset, header.assembliesSize);
    if (v241Plus) version = 24.1;

    typeDefs = readMetadataClassArray<Il2CppTypeDefinition>(header.typeDefinitionsOffset, header.typeDefinitionsSize);
    methodDefs = readMetadataClassArray<Il2CppMethodDefinition>(header.methodsOffset, header.methodsSize);
    parameterDefs = readMetadataClassArray<Il2CppParameterDefinition>(header.parametersOffset, header.parametersSize);
    fieldDefs = readMetadataClassArray<Il2CppFieldDefinition>(header.fieldsOffset, header.fieldsSize);

    auto fieldDefaultValues = readMetadataClassArray<Il2CppFieldDefaultValue>(header.fieldDefaultValuesOffset, header.fieldDefaultValuesSize);
    auto parameterDefaultValues = readMetadataClassArray<Il2CppParameterDefaultValue>(header.parameterDefaultValuesOffset, header.parameterDefaultValuesSize);
    for (auto& fdv : fieldDefaultValues) fieldDefaultValuesDic[fdv.fieldIndex] = fdv;
    for (auto& pdv : parameterDefaultValues) parameterDefaultValuesDic[pdv.parameterIndex] = pdv;

    propertyDefs = readMetadataClassArray<Il2CppPropertyDefinition>(header.propertiesOffset, header.propertiesSize);
    interfaceIndices = readPrimitiveArray<int32_t>(header.interfacesOffset, header.interfacesSize / 4);
    nestedTypeIndices = readPrimitiveArray<int32_t>(header.nestedTypesOffset, header.nestedTypesSize / 4);
    eventDefs = readMetadataClassArray<Il2CppEventDefinition>(header.eventsOffset, header.eventsSize);
    genericContainers = readMetadataClassArray<Il2CppGenericContainer>(header.genericContainersOffset, header.genericContainersSize);
    genericParameters = readMetadataClassArray<Il2CppGenericParameter>(header.genericParametersOffset, header.genericParametersSize);
    constraintIndices = readPrimitiveArray<int32_t>(header.genericParameterConstraintsOffset, header.genericParameterConstraintsSize / 4);
    vtableMethods = readPrimitiveArray<uint32_t>(header.vtableMethodsOffset, header.vtableMethodsSize / 4);
    stringLiterals = readMetadataClassArray<Il2CppStringLiteral>(header.stringLiteralOffset, header.stringLiteralSize);

    if (version > 16) {
        fieldRefs = readMetadataClassArray<Il2CppFieldRef>(header.fieldRefsOffset, header.fieldRefsSize);
        if (version < 27) {
            auto metadataUsageLists = readMetadataClassArray<Il2CppMetadataUsageList>(header.metadataUsageListsOffset, header.metadataUsageListsCount);
            auto metadataUsagePairs = readMetadataClassArray<Il2CppMetadataUsagePair>(header.metadataUsagePairsOffset, header.metadataUsagePairsCount);
            processingMetadataUsage(metadataUsageLists, metadataUsagePairs);
        }
    }

    if (version > 20 && version < 29) {
        attributeTypeRanges = readMetadataClassArray<Il2CppCustomAttributeTypeRange>(header.attributesInfoOffset, header.attributesInfoCount);
        attributeTypes = readPrimitiveArray<int32_t>(header.attributeTypesOffset, header.attributeTypesCount / 4);
    }

    if (version >= 29) {
        attributeDataRanges = readMetadataClassArray<Il2CppCustomAttributeDataRange>(header.attributeDataRangeOffset, header.attributeDataRangeSize);
    }

    if (version > 24) {
        for (size_t i = 0; i < imageDefs.size(); i++) {
            std::unordered_map<uint32_t, int> dic;
            auto end = imageDefs[i].customAttributeStart + imageDefs[i].customAttributeCount;
            for (int j = imageDefs[i].customAttributeStart; j < static_cast<int>(end); j++) {
                if (version >= 29) {
                    if (static_cast<size_t>(j) < attributeDataRanges.size())
                        dic[attributeDataRanges[j].token] = j;
                } else {
                    if (static_cast<size_t>(j) < attributeTypeRanges.size())
                        dic[attributeTypeRanges[j].token] = j;
                }
            }
            attributeTypeRangesDic[static_cast<int>(i)] = std::move(dic);
        }
    }

    if (version <= 24.1) {
        rgctxEntries = readMetadataClassArray<Il2CppRGCTXDefinition>(header.rgctxEntriesOffset, header.rgctxEntriesCount);
    }

    LOGI("Metadata loaded: version=%.1f, types=%zu, methods=%zu, images=%zu",
         version, typeDefs.size(), methodDefs.size(), imageDefs.size());
}

bool Metadata::getFieldDefaultValueFromIndex(int index, Il2CppFieldDefaultValue& value) const {
    auto it = fieldDefaultValuesDic.find(index);
    if (it != fieldDefaultValuesDic.end()) {
        value = it->second;
        return true;
    }
    return false;
}

bool Metadata::getParameterDefaultValueFromIndex(int index, Il2CppParameterDefaultValue& value) const {
    auto it = parameterDefaultValuesDic.find(index);
    if (it != parameterDefaultValuesDic.end()) {
        value = it->second;
        return true;
    }
    return false;
}

uint32_t Metadata::getDefaultValueFromIndex(int index) const {
    return header.fieldAndParameterDefaultValueDataOffset + index;
}

std::string Metadata::getStringFromIndex(uint32_t index) {
    auto it = stringCache.find(index);
    if (it != stringCache.end()) return it->second;
    auto result = readStringToNull(header.stringOffset + index);
    stringCache[index] = result;
    return result;
}

int Metadata::getCustomAttributeIndex(const Il2CppImageDefinition& imageDef, int customAttributeIndex, uint32_t token) const {
    if (version > 24) {
        // The dictionary is keyed by position in imageDefs (assemblyIndex does
        // not necessarily equal the image index).
        auto imageIndex = static_cast<int>(&imageDef - imageDefs.data());
        if (imageIndex < 0 || static_cast<size_t>(imageIndex) >= imageDefs.size()) {
            imageIndex = -1;
            for (size_t i = 0; i < imageDefs.size(); i++) {
                if (imageDefs[i].nameIndex == imageDef.nameIndex) { imageIndex = static_cast<int>(i); break; }
            }
        }
        auto imgIt = attributeTypeRangesDic.find(imageIndex);
        if (imgIt != attributeTypeRangesDic.end()) {
            auto it = imgIt->second.find(token);
            if (it != imgIt->second.end()) return it->second;
        }
        return -1;
    }
    return customAttributeIndex;
}

std::string Metadata::getStringLiteralFromIndex(uint32_t index) {
    if (index >= stringLiterals.size()) return "";
    auto& sl = stringLiterals[index];
    setPosition(header.stringLiteralDataOffset + sl.dataIndex);
    auto bytes = readBytes(static_cast<int>(sl.length));
    return std::string(bytes.begin(), bytes.end());
}

uint32_t Metadata::getEncodedIndexType(uint32_t index) {
    return (index & 0xE0000000) >> 29;
}

uint32_t Metadata::getDecodedMethodIndex(uint32_t index) const {
    if (version >= 27) return (index & 0x1FFFFFFEu) >> 1;
    return index & 0x1FFFFFFFu;
}

void Metadata::processingMetadataUsage(const std::vector<Il2CppMetadataUsageList>& usageLists,
                                        const std::vector<Il2CppMetadataUsagePair>& usagePairs) {
    for (uint32_t i = 1; i <= 6; i++) {
        metadataUsageDic[static_cast<int>(i)] = {};
    }
    for (auto& usageList : usageLists) {
        for (uint32_t i = 0; i < usageList.count; i++) {
            auto offset = usageList.start + i;
            if (offset >= usagePairs.size()) continue;
            auto& pair = usagePairs[offset];
            auto usage = getEncodedIndexType(pair.encodedSourceIndex);
            auto decodedIndex = getDecodedMethodIndex(pair.encodedSourceIndex);
            metadataUsageDic[static_cast<int>(usage)][pair.destinationIndex] = decodedIndex;
        }
    }
    // Compute metadataUsagesCount as max destinationIndex + 1
    uint32_t maxIndex = 0;
    for (auto& [type, dic] : metadataUsageDic) {
        if (!dic.empty()) {
            auto lastKey = dic.rbegin()->first;
            if (lastKey > maxIndex) maxIndex = lastKey;
        }
    }
    metadataUsagesCount = static_cast<int64_t>(maxIndex) + 1;
}

} // namespace il2cpp_dumper
