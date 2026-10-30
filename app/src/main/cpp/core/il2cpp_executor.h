#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "metadata.h"
#include "il2cpp.h"
#include "blob_value.h"

namespace il2cpp_dumper {

class Il2CppExecutor {
public:
    Metadata& metadata;
    Il2CppEngine& il2Cpp;

    Il2CppExecutor(Metadata& metadata, Il2CppEngine& il2Cpp);

    std::vector<uint64_t> customAttributeGenerators;

    std::string getTypeName(const Il2CppType& il2CppType, bool addNamespace, bool isNested);
    std::string getTypeDefName(const Il2CppTypeDefinition& typeDef, bool addNamespace, bool genericParameter);
    std::string getGenericInstParams(const Il2CppGenericInst& genericInst);
    std::string getGenericContainerParams(const Il2CppGenericContainer& genericContainer);
    std::pair<std::string, std::string> getMethodSpecName(const Il2CppMethodSpec& methodSpec, bool addNamespace = false);
    Il2CppGenericContext getMethodSpecGenericContext(const Il2CppMethodSpec& methodSpec);

    // RGCTX entries for a type/method, or nullptr when there are none.
    const std::vector<Il2CppRGCTXDefinition>* getRGCTXDefinition(const std::string& imageName, const Il2CppTypeDefinition& typeDef);
    const std::vector<Il2CppRGCTXDefinition>* getRGCTXDefinition(const std::string& imageName, const Il2CppMethodDefinition& methodDef);

    // Returns nullptr when the generic class does not resolve to a type definition.
    const Il2CppTypeDefinition* getGenericClassTypeDefinition(const Il2CppGenericClass& genericClass);
    // Throws std::out_of_range when the type does not resolve.
    const Il2CppTypeDefinition& getTypeDefinitionFromIl2CppType(const Il2CppType& il2CppType);
    const Il2CppGenericParameter& getGenericParameterFromIl2CppType(const Il2CppType& il2CppType);

    SectionHelper* getSectionHelper();

    // Reads a field/parameter default value. On failure `pointer` holds the
    // metadata offset of the raw blob (upstream prints it as a comment).
    bool tryGetDefaultValue(int typeIndex, int dataIndex, BlobValue& value, uint32_t& pointer);
    bool getConstantValueFromBlob(Il2CppTypeEnum type, BinaryStream& reader, BlobValue& value);
    Il2CppTypeEnum readEncodedTypeEnum(BinaryStream& reader, int64_t& enumTypeIndex);

    // .NET-style ToString() of a scalar blob value (numbers, bool, raw string).
    static std::string scalarToString(const BlobValue& value);
    // C# string literal escaping (upstream StringExtensions.ToEscapedString).
    static std::string escapeString(const std::string& s);

private:
    static const std::unordered_map<int, std::string> typeStringMap;
    // Pre-24.2 RGCTX slices of metadata.rgctxEntries, keyed by (start << 32 | count).
    std::unordered_map<uint64_t, std::vector<Il2CppRGCTXDefinition>> rgctxCache;

    const Il2CppType& typeAt(int64_t index) const;
};

} // namespace il2cpp_dumper
