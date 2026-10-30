#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "binary_stream.h"
#include "il2cpp_class.h"
#include "metadata_class.h"
#include "search_section.h"

namespace il2cpp_dumper {

// Forward declarations
class ElfBase;
class SectionHelper;

class Il2CppEngine : public BinaryStream {
public:
    Il2CppEngine(const uint8_t* data, size_t size, bool isElf64);
    Il2CppEngine(std::vector<uint8_t>&& data, bool isElf64);
    virtual ~Il2CppEngine() = default;

    // Virtual address mapping (delegated to format-specific parser)
    virtual uint64_t mapVATR(uint64_t addr) = 0;
    virtual uint64_t mapRTVA(uint64_t addr) = 0;
    virtual bool search() = 0;
    virtual bool plusSearch(int methodCount, int typeDefinitionsCount, int imageCount) = 0;
    virtual bool symbolSearch() = 0;
    virtual SectionHelper* getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount) = 0;
    virtual SectionHelper* getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount, int64_t metadataUsagesCount) = 0;
    virtual bool checkDump() = 0;
    virtual uint64_t getRVA(uint64_t pointer) = 0;
    virtual bool checkProtection() { return false; }
    // Marks the binary as a memory dump loaded at `address`. When `redirect` is
    // true the format parser rebuilds its segment/dynamic tables for that base.
    virtual void applyDumpAddress(uint64_t address, bool redirect) {
        imageBase = address;
        isDumped = true;
        (void)redirect;
    }

    // Common Il2Cpp operations
    void setProperties(double version, int64_t metadataUsagesCount);
    bool autoPlusInit(uint64_t codeRegistration, uint64_t metadataRegistration);
    virtual void init(uint64_t codeRegistration, uint64_t metadataRegistration);

    // Type access
    Il2CppType* getIl2CppType(uint64_t pointer);
    uint64_t getMethodPointer(const std::string& imageName, const Il2CppMethodDefinition& methodDef);
    int getFieldOffsetFromIndex(int typeIndex, int fieldIndexInType, int fieldIndex,
                                 bool isValueType, bool isStatic);
    // Index stored in an RGCTX entry (type index or method spec index).
    int32_t getRGCTXDataIndex(const Il2CppRGCTXDefinition& def);

    size_t getTypeIndex(uint64_t addr) const {
        auto it = typeDic.find(addr);
        return it != typeDic.end() ? it->second : SIZE_MAX;
    }

    // Public data
    std::vector<uint64_t> methodPointers;
    std::vector<uint64_t> genericMethodPointers;
    std::vector<uint64_t> invokerPointers;
    std::vector<uint64_t> customAttributeGenerators;
    std::vector<uint64_t> reversePInvokeWrappers;
    std::vector<uint64_t> unresolvedVirtualCallPointers;
    std::vector<Il2CppType> types;
    std::vector<uint64_t> metadataUsages;
    std::vector<Il2CppGenericInst> genericInsts;
    std::vector<uint64_t> genericInstPointers;
    std::vector<Il2CppMethodSpec> methodSpecs;
    std::unordered_map<int, std::vector<Il2CppMethodSpec>> methodDefinitionMethodSpecs;
    std::unordered_map<Il2CppMethodSpec, uint64_t, Il2CppMethodSpecHash> methodSpecGenericMethodPointers;
    std::unordered_map<std::string, Il2CppCodeGenModule> codeGenModules;
    std::unordered_map<std::string, std::vector<uint64_t>> codeGenModuleMethodPointers;
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<Il2CppRGCTXDefinition>>> rgctxsDictionary;
    bool isDumped = false;

protected:
    int64_t metadataUsagesCount = 0;
    bool fieldOffsetsArePointers = false;
    std::vector<uint64_t> fieldOffsets;
    std::unordered_map<uint64_t, size_t> typeDic; // address -> index in types vector

    Il2CppCodeRegistration pCodeRegistration{};
    Il2CppMetadataRegistration pMetadataRegistration{};
};

// ELF-based Il2Cpp implementation
class ElfIl2Cpp : public Il2CppEngine {
public:
    ElfIl2Cpp(const uint8_t* data, size_t size, bool isElf64);
    ElfIl2Cpp(std::vector<uint8_t>&& data, bool isElf64);
    ~ElfIl2Cpp() override;

    uint64_t mapVATR(uint64_t addr) override;
    uint64_t mapRTVA(uint64_t addr) override;
    bool search() override;
    bool plusSearch(int methodCount, int typeDefinitionsCount, int imageCount) override;
    bool symbolSearch() override;
    SectionHelper* getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount) override;
    SectionHelper* getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount, int64_t metadataUsagesCount) override;
    bool checkDump() override;
    uint64_t getRVA(uint64_t pointer) override;
    bool checkProtection() override;
    void applyDumpAddress(uint64_t address, bool redirect) override;

private:
    ElfBase* elfParser = nullptr;
    void initElf();
    void syncParser();
};

} // namespace il2cpp_dumper
