#pragma once

#include <cstdint>
#include "blob_value.h"

namespace il2cpp_dumper {

// Forward declaration
class BinaryStream;

struct Il2CppCodeRegistration {
    uint64_t methodPointersCount = 0;     // Version <= 24.1
    uint64_t methodPointers = 0;          // Version <= 24.1
    uint64_t delegateWrappersFromNativeToManagedCount = 0; // Version <= 21
    uint64_t delegateWrappersFromNativeToManaged = 0;      // Version <= 21
    uint64_t reversePInvokeWrapperCount = 0; // Version >= 22
    uint64_t reversePInvokeWrappers = 0;     // Version >= 22
    uint64_t delegateWrappersFromManagedToNativeCount = 0; // Version <= 22
    uint64_t delegateWrappersFromManagedToNative = 0;      // Version <= 22
    uint64_t marshalingFunctionsCount = 0;   // Version <= 22
    uint64_t marshalingFunctions = 0;        // Version <= 22
    uint64_t ccwMarshalingFunctionsCount = 0; // Version 21-22
    uint64_t ccwMarshalingFunctions = 0;      // Version 21-22
    uint64_t genericMethodPointersCount = 0;
    uint64_t genericMethodPointers = 0;
    uint64_t genericAdjustorThunks = 0;    // Version 24.5 or >= 27.1
    uint64_t invokerPointersCount = 0;
    uint64_t invokerPointers = 0;
    uint64_t customAttributeCount = 0;     // Version <= 24.5
    uint64_t customAttributeGenerators = 0; // Version <= 24.5
    uint64_t guidCount = 0;                // Version 21-22
    uint64_t guids = 0;                    // Version 21-22
    uint64_t unresolvedVirtualCallCount = 0; // Version >= 22
    uint64_t unresolvedVirtualCallPointers = 0; // Version >= 22
    uint64_t unresolvedInstanceCallPointers = 0; // Version >= 29.1
    uint64_t unresolvedStaticCallPointers = 0;   // Version >= 29.1
    uint64_t interopDataCount = 0;         // Version >= 23
    uint64_t interopData = 0;              // Version >= 23
    uint64_t windowsRuntimeFactoryCount = 0; // Version >= 24.3
    uint64_t windowsRuntimeFactoryTable = 0; // Version >= 24.3
    uint64_t codeGenModulesCount = 0;      // Version >= 24.2
    uint64_t codeGenModules = 0;           // Version >= 24.2

    void read(BinaryStream& stream, double version);
};

struct Il2CppMetadataRegistration {
    int64_t genericClassesCount = 0;
    uint64_t genericClasses = 0;
    int64_t genericInstsCount = 0;
    uint64_t genericInsts = 0;
    int64_t genericMethodTableCount = 0;
    uint64_t genericMethodTable = 0;
    int64_t typesCount = 0;
    uint64_t types = 0;
    int64_t methodSpecsCount = 0;
    uint64_t methodSpecs = 0;
    int64_t methodReferencesCount = 0;  // Version <= 16
    uint64_t methodReferences = 0;      // Version <= 16
    int64_t fieldOffsetsCount = 0;
    uint64_t fieldOffsets = 0;
    int64_t typeDefinitionsSizesCount = 0;
    uint64_t typeDefinitionsSizes = 0;
    uint64_t metadataUsagesCount = 0;   // Version >= 19
    uint64_t metadataUsages = 0;        // Version >= 19

    void read(BinaryStream& stream, double version);
};

struct Il2CppType {
    uint64_t datapoint = 0;
    uint32_t bits = 0;
    // Parsed fields
    uint32_t attrs = 0;
    Il2CppTypeEnum type = Il2CppTypeEnum::IL2CPP_TYPE_END;
    uint32_t num_mods = 0;
    uint32_t byref = 0;
    uint32_t pinned = 0;
    uint32_t valuetype = 0;

    // Union data accessors
    int64_t klassIndex() const { return static_cast<int64_t>(datapoint); }
    uint64_t typeHandle() const { return datapoint; }
    uint64_t typeRef() const { return datapoint; }
    uint64_t array() const { return datapoint; }
    int64_t genericParameterIndex() const { return static_cast<int64_t>(datapoint); }
    uint64_t genericParameterHandle() const { return datapoint; }
    uint64_t generic_class() const { return datapoint; }

    void init(double version) {
        attrs = bits & 0xffff;
        type = static_cast<Il2CppTypeEnum>((bits >> 16) & 0xff);
        if (version >= 27.2) {
            num_mods = (bits >> 24) & 0x1f;
            byref = (bits >> 29) & 1;
            pinned = (bits >> 30) & 1;
            valuetype = bits >> 31;
        } else {
            num_mods = (bits >> 24) & 0x3f;
            byref = (bits >> 30) & 1;
            pinned = bits >> 31;
        }
    }

    void read(BinaryStream& stream, double version);
};

struct Il2CppGenericClass {
    int64_t typeDefinitionIndex = 0; // Version <= 24.5
    uint64_t type = 0;              // Version >= 27
    uint64_t context_class_inst = 0;
    uint64_t context_method_inst = 0;
    uint64_t cached_class = 0;

    void read(BinaryStream& stream, double version);
};

struct Il2CppGenericContext {
    uint64_t class_inst = 0;
    uint64_t method_inst = 0;
};

struct Il2CppGenericInst {
    int64_t type_argc = 0;
    uint64_t type_argv = 0;

    void read(BinaryStream& stream, double /*version*/) {
        type_argc = stream.readIntPtr();
        type_argv = stream.readUIntPtr();
    }
};

struct Il2CppArrayType {
    uint64_t etype = 0;
    uint8_t rank = 0;
    uint8_t numsizes = 0;
    uint8_t numlobounds = 0;
    uint64_t sizes = 0;
    uint64_t lobounds = 0;

    void read(BinaryStream& stream, double version);
};

struct Il2CppGenericMethodFunctionsDefinitions {
    int32_t genericMethodIndex = 0;
    int32_t indices_methodIndex = 0;
    int32_t indices_invokerIndex = 0;
    int32_t indices_adjustorThunk = 0; // Version 24.5 or >= 27.1

    void read(BinaryStream& stream, double version);
};

struct Il2CppMethodSpec {
    int32_t methodDefinitionIndex = 0;
    int32_t classIndexIndex = 0;
    int32_t methodIndexIndex = 0;

    void read(BinaryStream& stream, double /*version*/) {
        methodDefinitionIndex = stream.readInt32();
        classIndexIndex = stream.readInt32();
        methodIndexIndex = stream.readInt32();
    }

    bool operator==(const Il2CppMethodSpec& o) const {
        return methodDefinitionIndex == o.methodDefinitionIndex &&
               classIndexIndex == o.classIndexIndex &&
               methodIndexIndex == o.methodIndexIndex;
    }
};

struct Il2CppMethodSpecHash {
    size_t operator()(const Il2CppMethodSpec& m) const {
        uint64_t h = static_cast<uint32_t>(m.methodDefinitionIndex);
        h = h * 0x9E3779B97F4A7C15ull ^ static_cast<uint32_t>(m.classIndexIndex);
        h = h * 0x9E3779B97F4A7C15ull ^ static_cast<uint32_t>(m.methodIndexIndex);
        return static_cast<size_t>(h ^ (h >> 32));
    }
};

struct Il2CppCodeGenModule {
    uint64_t moduleName = 0;
    int64_t methodPointerCount = 0;
    uint64_t methodPointers = 0;
    int64_t adjustorThunkCount = 0;  // Version 24.5 or >= 27.1
    uint64_t adjustorThunks = 0;    // Version 24.5 or >= 27.1
    uint64_t invokerIndices = 0;
    uint64_t reversePInvokeWrapperCount = 0;
    uint64_t reversePInvokeWrapperIndices = 0;
    int64_t rgctxRangesCount = 0;
    uint64_t rgctxRanges = 0;
    int64_t rgctxsCount = 0;
    uint64_t rgctxs = 0;
    uint64_t debuggerMetadata = 0;
    uint64_t customAttributeCacheGenerator = 0; // Version 27-27.2
    uint64_t moduleInitializer = 0;  // Version >= 27
    uint64_t staticConstructorTypeIndices = 0; // Version >= 27
    uint64_t metadataRegistration = 0;  // Version >= 27
    uint64_t codeRegistration = 0;      // Version >= 27

    void read(BinaryStream& stream, double version);
};

struct Il2CppRange {
    int32_t start = 0;
    int32_t length = 0;
};

struct Il2CppTokenRangePair {
    uint32_t token = 0;
    Il2CppRange range;

    void read(BinaryStream& stream, double /*version*/) {
        token = stream.readUInt32();
        range.start = stream.readInt32();
        range.length = stream.readInt32();
    }
};

} // namespace il2cpp_dumper
