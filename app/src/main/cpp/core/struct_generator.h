#pragma once

#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "il2cpp_executor.h"
#include "struct_info.h"
#include "script_json.h"

namespace il2cpp_dumper {

class StructGenerator {
public:
    StructGenerator(Il2CppExecutor& executor);

    void writeScript(const std::string& outputDir);

private:
    Il2CppExecutor& executor;
    Metadata& metadata;
    Il2CppEngine& il2Cpp;

    std::unordered_map<int, std::string> typeDefImageNames;
    std::unordered_set<std::string> structNameHashSet;
    std::vector<StructInfo> structInfoList;
    std::unordered_map<std::string, StructInfo*> structInfoWithStructName;
    std::unordered_set<const StructInfo*> structCache;
    std::unordered_map<int, std::string> structNameDic;
    std::unordered_map<uint64_t, std::string> genericClassStructNameDic;
    std::unordered_map<std::string, const Il2CppType*> nameGenericClassDic;
    std::vector<uint64_t> genericClassList;
    std::unordered_set<uint64_t> methodInfoCache;
    std::ostringstream arrayClassHeader;
    std::ostringstream methodInfoHeader;

    static const std::unordered_set<std::string> keywords;
    static const std::unordered_set<std::string> specialKeywords;

    static std::string fixName(const std::string& str);
    std::string parseType(const Il2CppType& il2CppType, const Il2CppGenericContext* context = nullptr);
    std::string getIl2CppStructName(const Il2CppType& il2CppType, const Il2CppGenericContext* context = nullptr);
    bool isValueType(const Il2CppType& il2CppType, const Il2CppGenericContext* context);
    bool isCustomType(const Il2CppType& il2CppType, const Il2CppGenericContext* context);

    const Il2CppType& typeAt(int64_t index) const;
    const Il2CppType& typeFromPointer(uint64_t pointer);
    const Il2CppType& genericArgument(const Il2CppType& il2CppType, uint64_t genericInstPointer);
    const Il2CppTypeDefinition& genericClassTypeDefinition(uint64_t genericClassPointer, Il2CppGenericClass* out = nullptr);
    int typeDefIndexOf(const Il2CppTypeDefinition& typeDef) const;
    const std::string& structNameOf(const Il2CppTypeDefinition& typeDef) const;
    const Il2CppTypeDefinition& getTypeDefinition(const Il2CppType& il2CppType);

    void createStructNameDic(const Il2CppTypeDefinition& typeDef, int typeDefIndex);
    std::string getUniqueName(const std::string& name);
    void addStruct(const Il2CppTypeDefinition& typeDef, int typeDefIndex);
    void addGenericClassStruct(uint64_t pointer);
    void addParents(const Il2CppTypeDefinition& typeDef, StructInfo& info);
    void addFields(const Il2CppTypeDefinition& typeDef, StructInfo& info, const Il2CppGenericContext* context);
    void addVTableMethod(StructInfo& info, const Il2CppTypeDefinition& typeDef);
    void addRGCTX(StructInfo& info, int typeDefIndex);
    std::vector<StructRGCTXInfo> buildRGCTX(const std::vector<Il2CppRGCTXDefinition>* collection);
    void parseArrayClassStruct(const Il2CppType& il2CppType, const Il2CppGenericContext* context);
    void recursionStructInfo(const StructInfo& info, std::string& out);
    void generateMethodInfo(const std::string& methodInfoName, const std::string& structTypeName,
                            const std::vector<StructRGCTXInfo>& rgctxs);
    std::vector<StructRGCTXInfo> generateRGCTX(const std::string& imageName,
                                                 const Il2CppMethodDefinition& methodDef);

    void addMetadataUsageTypeInfo(ScriptJson& json, uint32_t index, uint64_t address);
    void addMetadataUsageIl2CppType(ScriptJson& json, uint32_t index, uint64_t address);
    void addMetadataUsageMethodDef(ScriptJson& json, uint32_t index, uint64_t address);
    void addMetadataUsageFieldInfo(ScriptJson& json, uint32_t index, uint64_t address);
    void addMetadataUsageStringLiteral(ScriptJson& json, uint32_t index, uint64_t address);
    void addMetadataUsageMethodRef(ScriptJson& json, uint32_t index, uint64_t address);

    static std::string getMethodTypeSignature(const std::vector<Il2CppTypeEnum>& types);
};

} // namespace il2cpp_dumper
