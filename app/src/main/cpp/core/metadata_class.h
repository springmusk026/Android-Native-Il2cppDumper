#pragma once

#include <cstdint>
#include <vector>
#include "blob_value.h"

namespace il2cpp_dumper {

// Forward declaration
class BinaryStream;

struct Il2CppGlobalMetadataHeader {
    uint32_t sanity = 0;
    int32_t version = 0;
    uint32_t stringLiteralOffset = 0;
    int32_t stringLiteralSize = 0;
    uint32_t stringLiteralDataOffset = 0;
    int32_t stringLiteralDataSize = 0;
    uint32_t stringOffset = 0;
    int32_t stringSize = 0;
    uint32_t eventsOffset = 0;
    int32_t eventsSize = 0;
    uint32_t propertiesOffset = 0;
    int32_t propertiesSize = 0;
    uint32_t methodsOffset = 0;
    int32_t methodsSize = 0;
    uint32_t parameterDefaultValuesOffset = 0;
    int32_t parameterDefaultValuesSize = 0;
    uint32_t fieldDefaultValuesOffset = 0;
    int32_t fieldDefaultValuesSize = 0;
    uint32_t fieldAndParameterDefaultValueDataOffset = 0;
    int32_t fieldAndParameterDefaultValueDataSize = 0;
    int32_t fieldMarshaledSizesOffset = 0;
    int32_t fieldMarshaledSizesSize = 0;
    uint32_t parametersOffset = 0;
    int32_t parametersSize = 0;
    uint32_t fieldsOffset = 0;
    int32_t fieldsSize = 0;
    uint32_t genericParametersOffset = 0;
    int32_t genericParametersSize = 0;
    uint32_t genericParameterConstraintsOffset = 0;
    int32_t genericParameterConstraintsSize = 0;
    uint32_t genericContainersOffset = 0;
    int32_t genericContainersSize = 0;
    uint32_t nestedTypesOffset = 0;
    int32_t nestedTypesSize = 0;
    uint32_t interfacesOffset = 0;
    int32_t interfacesSize = 0;
    uint32_t vtableMethodsOffset = 0;
    int32_t vtableMethodsSize = 0;
    int32_t interfaceOffsetsOffset = 0;
    int32_t interfaceOffsetsSize = 0;
    uint32_t typeDefinitionsOffset = 0;
    int32_t typeDefinitionsSize = 0;
    // Version <= 24.1
    uint32_t rgctxEntriesOffset = 0;
    int32_t rgctxEntriesCount = 0;
    uint32_t imagesOffset = 0;
    int32_t imagesSize = 0;
    uint32_t assembliesOffset = 0;
    int32_t assembliesSize = 0;
    // Version 19-24.5
    uint32_t metadataUsageListsOffset = 0;
    int32_t metadataUsageListsCount = 0;
    uint32_t metadataUsagePairsOffset = 0;
    int32_t metadataUsagePairsCount = 0;
    // Version >= 19
    uint32_t fieldRefsOffset = 0;
    int32_t fieldRefsSize = 0;
    // Version >= 20
    int32_t referencedAssembliesOffset = 0;
    int32_t referencedAssembliesSize = 0;
    // Version 21-27.2
    uint32_t attributesInfoOffset = 0;
    int32_t attributesInfoCount = 0;
    uint32_t attributeTypesOffset = 0;
    int32_t attributeTypesCount = 0;
    // Version >= 29
    uint32_t attributeDataOffset = 0;
    int32_t attributeDataSize = 0;
    uint32_t attributeDataRangeOffset = 0;
    int32_t attributeDataRangeSize = 0;
    // Version >= 22
    int32_t unresolvedVirtualCallParameterTypesOffset = 0;
    int32_t unresolvedVirtualCallParameterTypesSize = 0;
    int32_t unresolvedVirtualCallParameterRangesOffset = 0;
    int32_t unresolvedVirtualCallParameterRangesSize = 0;
    // Version >= 23
    int32_t windowsRuntimeTypeNamesOffset = 0;
    int32_t windowsRuntimeTypeNamesSize = 0;
    // Version >= 27
    int32_t windowsRuntimeStringsOffset = 0;
    int32_t windowsRuntimeStringsSize = 0;
    // Version >= 24
    int32_t exportedTypeDefinitionsOffset = 0;
    int32_t exportedTypeDefinitionsSize = 0;

    void read(BinaryStream& stream, double version);
};

struct Il2CppAssemblyDefinition {
    int32_t imageIndex = 0;
    uint32_t token = 0;          // Version >= 24.1
    int32_t customAttributeIndex = 0; // Version <= 24
    int32_t referencedAssemblyStart = 0; // Version >= 20
    int32_t referencedAssemblyCount = 0; // Version >= 20
    // Il2CppAssemblyNameDefinition inlined
    uint32_t aname_nameIndex = 0;
    uint32_t aname_cultureIndex = 0;
    int32_t aname_hashValueIndex = 0; // Version <= 24.3
    uint32_t aname_publicKeyIndex = 0;
    uint32_t aname_hash_alg = 0;
    int32_t aname_hash_len = 0;
    uint32_t aname_flags = 0;
    int32_t aname_major = 0;
    int32_t aname_minor = 0;
    int32_t aname_build = 0;
    int32_t aname_revision = 0;
    uint8_t aname_public_key_token[8]{};

    void read(BinaryStream& stream, double version);
};

struct Il2CppImageDefinition {
    uint32_t nameIndex = 0;
    int32_t assemblyIndex = 0;
    int32_t typeStart = 0;
    uint32_t typeCount = 0;
    int32_t exportedTypeStart = 0;  // Version >= 24
    uint32_t exportedTypeCount = 0; // Version >= 24
    int32_t entryPointIndex = 0;
    uint32_t token = 0;             // Version >= 19
    int32_t customAttributeStart = 0; // Version >= 24.1
    uint32_t customAttributeCount = 0; // Version >= 24.1

    void read(BinaryStream& stream, double version);
};

struct Il2CppTypeDefinition {
    uint32_t nameIndex = 0;
    uint32_t namespaceIndex = 0;
    int32_t customAttributeIndex = 0; // Version <= 24
    int32_t byvalTypeIndex = 0;
    int32_t byrefTypeIndex = 0;       // Version <= 24.5
    // -1 defaults so a default-constructed ("not found") definition is not
    // mistaken for a nested/generic type pointing at index 0.
    int32_t declaringTypeIndex = -1;
    int32_t parentIndex = -1;
    int32_t elementTypeIndex = -1;
    int32_t rgctxStartIndex = 0;      // Version <= 24.1
    int32_t rgctxCount = 0;           // Version <= 24.1
    int32_t genericContainerIndex = -1;
    int32_t delegateWrapperFromManagedToNativeIndex = 0; // Version <= 22
    int32_t marshalingFunctionsIndex = 0; // Version <= 22
    int32_t ccwFunctionIndex = 0;     // Version 21-22
    int32_t guidIndex = 0;            // Version 21-22
    uint32_t flags = 0;
    int32_t fieldStart = 0;
    int32_t methodStart = 0;
    int32_t eventStart = 0;
    int32_t propertyStart = 0;
    int32_t nestedTypesStart = 0;
    int32_t interfacesStart = 0;
    int32_t vtableStart = 0;
    int32_t interfaceOffsetsStart = 0;
    uint16_t method_count = 0;
    uint16_t property_count = 0;
    uint16_t field_count = 0;
    uint16_t event_count = 0;
    uint16_t nested_type_count = 0;
    uint16_t vtable_count = 0;
    uint16_t interfaces_count = 0;
    uint16_t interface_offsets_count = 0;
    uint32_t bitfield = 0;
    uint32_t token = 0;               // Version >= 19

    bool isValueType() const { return (bitfield & 0x1) == 1; }
    bool isEnum() const { return ((bitfield >> 1) & 0x1) == 1; }

    void read(BinaryStream& stream, double version);
};

struct Il2CppMethodDefinition {
    uint32_t nameIndex = 0;
    int32_t declaringType = 0;
    int32_t returnType = 0;
    int32_t returnParameterToken = 0; // Version >= 31
    int32_t parameterStart = 0;
    int32_t customAttributeIndex = 0; // Version <= 24
    int32_t genericContainerIndex = 0;
    int32_t methodIndex = 0;          // Version <= 24.1
    int32_t invokerIndex = 0;         // Version <= 24.1
    int32_t delegateWrapperIndex = 0; // Version <= 24.1
    int32_t rgctxStartIndex = 0;     // Version <= 24.1
    int32_t rgctxCount = 0;          // Version <= 24.1
    uint32_t token = 0;
    uint16_t flags = 0;
    uint16_t iflags = 0;
    uint16_t slot = 0;
    uint16_t parameterCount = 0;

    void read(BinaryStream& stream, double version);
};

struct Il2CppParameterDefinition {
    uint32_t nameIndex = 0;
    uint32_t token = 0;
    int32_t customAttributeIndex = 0; // Version <= 24
    int32_t typeIndex = 0;

    void read(BinaryStream& stream, double version);
};

struct Il2CppFieldDefinition {
    uint32_t nameIndex = 0;
    int32_t typeIndex = 0;
    int32_t customAttributeIndex = 0; // Version <= 24
    uint32_t token = 0;               // Version >= 19

    void read(BinaryStream& stream, double version);
};

struct Il2CppFieldDefaultValue {
    int32_t fieldIndex = 0;
    int32_t typeIndex = 0;
    int32_t dataIndex = 0;

    void read(BinaryStream& stream, double /*version*/) {
        fieldIndex = stream.readInt32();
        typeIndex = stream.readInt32();
        dataIndex = stream.readInt32();
    }
};

struct Il2CppPropertyDefinition {
    uint32_t nameIndex = 0;
    int32_t get = 0;
    int32_t set = 0;
    uint32_t attrs = 0;
    int32_t customAttributeIndex = 0; // Version <= 24
    uint32_t token = 0;               // Version >= 19

    void read(BinaryStream& stream, double version);
};

struct Il2CppCustomAttributeTypeRange {
    uint32_t token = 0;  // Version >= 24.1
    int32_t start = 0;
    int32_t count = 0;

    void read(BinaryStream& stream, double version);
};

struct Il2CppMetadataUsageList {
    uint32_t start = 0;
    uint32_t count = 0;

    void read(BinaryStream& stream, double /*version*/) {
        start = stream.readUInt32();
        count = stream.readUInt32();
    }
};

struct Il2CppMetadataUsagePair {
    uint32_t destinationIndex = 0;
    uint32_t encodedSourceIndex = 0;

    void read(BinaryStream& stream, double /*version*/) {
        destinationIndex = stream.readUInt32();
        encodedSourceIndex = stream.readUInt32();
    }
};

struct Il2CppStringLiteral {
    uint32_t length = 0;
    int32_t dataIndex = 0;

    void read(BinaryStream& stream, double /*version*/) {
        length = stream.readUInt32();
        dataIndex = stream.readInt32();
    }
};

struct Il2CppParameterDefaultValue {
    int32_t parameterIndex = 0;
    int32_t typeIndex = 0;
    int32_t dataIndex = 0;

    void read(BinaryStream& stream, double /*version*/) {
        parameterIndex = stream.readInt32();
        typeIndex = stream.readInt32();
        dataIndex = stream.readInt32();
    }
};

struct Il2CppEventDefinition {
    uint32_t nameIndex = 0;
    int32_t typeIndex = 0;
    int32_t add = 0;
    int32_t remove = 0;
    int32_t raise = 0;
    int32_t customAttributeIndex = 0; // Version <= 24
    uint32_t token = 0;               // Version >= 19

    void read(BinaryStream& stream, double version);
};

struct Il2CppGenericContainer {
    int32_t ownerIndex = 0;
    int32_t type_argc = 0;
    int32_t is_method = 0;
    int32_t genericParameterStart = 0;

    void read(BinaryStream& stream, double /*version*/) {
        ownerIndex = stream.readInt32();
        type_argc = stream.readInt32();
        is_method = stream.readInt32();
        genericParameterStart = stream.readInt32();
    }
};

struct Il2CppFieldRef {
    int32_t typeIndex = 0;
    int32_t fieldIndex = 0;

    void read(BinaryStream& stream, double /*version*/) {
        typeIndex = stream.readInt32();
        fieldIndex = stream.readInt32();
    }
};

struct Il2CppGenericParameter {
    int32_t ownerIndex = 0;
    uint32_t nameIndex = 0;
    int16_t constraintsStart = 0;
    int16_t constraintsCount = 0;
    uint16_t num = 0;
    uint16_t flags = 0;

    void read(BinaryStream& stream, double /*version*/) {
        ownerIndex = stream.readInt32();
        nameIndex = stream.readUInt32();
        constraintsStart = stream.readInt16();
        constraintsCount = stream.readInt16();
        num = stream.readUInt16();
        flags = stream.readUInt16();
    }
};

struct Il2CppRGCTXDefinitionData {
    int32_t rgctxDataDummy = 0;

    int32_t methodIndex() const { return rgctxDataDummy; }
    int32_t typeIndex() const { return rgctxDataDummy; }
};

struct Il2CppRGCTXDefinition {
    Il2CppRGCTXDataType type{};
    int32_t type_pre29 = 0;       // Version <= 27.1
    uint64_t type_post29 = 0;     // Version >= 27.2 (pointer-sized, padded enum)
    Il2CppRGCTXDefinitionData data{}; // Version <= 27.1
    uint64_t _data = 0;           // Version >= 27.2: pointer to Il2CppRGCTXDefinitionData

    Il2CppRGCTXDataType getType() const {
        if (type_post29 == 0)
            return static_cast<Il2CppRGCTXDataType>(type_pre29);
        return static_cast<Il2CppRGCTXDataType>(type_post29);
    }

    void read(BinaryStream& stream, double version);
};

enum class Il2CppMetadataUsage {
    kIl2CppMetadataUsageInvalid = 0,
    kIl2CppMetadataUsageTypeInfo = 1,
    kIl2CppMetadataUsageIl2CppType = 2,
    kIl2CppMetadataUsageMethodDef = 3,
    kIl2CppMetadataUsageFieldInfo = 4,
    kIl2CppMetadataUsageStringLiteral = 5,
    kIl2CppMetadataUsageMethodRef = 6,
};

struct Il2CppCustomAttributeDataRange {
    uint32_t token = 0;
    uint32_t startOffset = 0;

    void read(BinaryStream& stream, double /*version*/) {
        token = stream.readUInt32();
        startOffset = stream.readUInt32();
    }
};

} // namespace il2cpp_dumper
