#include "struct_generator.h"
#include "header_constants.h"
#include "il2cpp_constants.h"
#include "section_helper.h"
#include "config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

namespace {

// Decodes one UTF-8 code point at s[i] and advances i. Ill-formed sequences
// decode to U+FFFD per maximal subpart, as .NET's UTF8 decoder does.
uint32_t decodeUtf8(const std::string& s, size_t& i) {
    auto b0 = static_cast<unsigned char>(s[i++]);
    if (b0 < 0x80) return b0;
    int len;
    uint32_t cp;
    unsigned char lo = 0x80, hi = 0xBF;
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        len = 1; cp = b0 & 0x1F;
    } else if (b0 >= 0xE0 && b0 <= 0xEF) {
        len = 2; cp = b0 & 0x0F;
        if (b0 == 0xE0) lo = 0xA0;
        if (b0 == 0xED) hi = 0x9F;
    } else if (b0 >= 0xF0 && b0 <= 0xF4) {
        len = 3; cp = b0 & 0x07;
        if (b0 == 0xF0) lo = 0x90;
        if (b0 == 0xF4) hi = 0x8F;
    } else {
        return 0xFFFD;
    }
    for (int k = 0; k < len; k++) {
        if (i >= s.size()) return 0xFFFD;
        auto b = static_cast<unsigned char>(s[i]);
        if (b < lo || b > hi) return 0xFFFD;
        lo = 0x80; hi = 0xBF;
        cp = (cp << 6) | (b & 0x3F);
        i++;
    }
    return cp;
}

void appendUnicodeEscape(std::string& out, uint32_t unit) {
    char buf[8];
    snprintf(buf, sizeof(buf), "\\u%04X", unit);
    out += buf;
}

// System.Text.Json default (JavaScriptEncoder.Default) string escaping.
std::string jsonString(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out += '"';
    for (size_t i = 0; i < s.size();) {
        auto cp = decodeUtf8(s, i);
        switch (cp) {
            case '\n': out += "\\n"; continue;
            case '\r': out += "\\r"; continue;
            case '\t': out += "\\t"; continue;
            case '\b': out += "\\b"; continue;
            case '\f': out += "\\f"; continue;
            case '\\': out += "\\\\"; continue;
            default: break;
        }
        if (cp >= 0x20 && cp < 0x7F && !strchr("\"&'+<>`", static_cast<int>(cp))) {
            out += static_cast<char>(cp);
        } else if (cp > 0xFFFF) {
            cp -= 0x10000;
            appendUnicodeEscape(out, 0xD800 + (cp >> 10));
            appendUnicodeEscape(out, 0xDC00 + (cp & 0x3FF));
        } else {
            appendUnicodeEscape(out, cp);
        }
    }
    out += '"';
    return out;
}

std::string jsonString(const std::optional<std::string>& s) {
    return s ? jsonString(*s) : "null";
}

std::string hexUpper(uint64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(value));
    return buf;
}

std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    std::string result;
    size_t start = 0, pos;
    while ((pos = str.find(from, start)) != std::string::npos) {
        result.append(str, start, pos - start);
        result += to;
        start = pos + from.size();
    }
    result.append(str, start, std::string::npos);
    return result;
}

std::string join(const std::vector<std::string>& parts, const char* sep) {
    std::string result;
    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) result += sep;
        result += parts[i];
    }
    return result;
}

using JsonFields = std::vector<std::pair<const char*, std::string>>;

// Indented array of objects, laid out like System.Text.Json WriteIndented.
template <typename T, typename F>
void writeObjectArray(std::ostream& f, const std::string& indent, const std::vector<T>& items, F toFields) {
    if (items.empty()) {
        f << "[]";
        return;
    }
    f << "[\n";
    for (size_t i = 0; i < items.size(); i++) {
        f << indent << "  {\n";
        JsonFields fields = toFields(items[i]);
        for (size_t k = 0; k < fields.size(); k++) {
            f << indent << "    \"" << fields[k].first << "\": " << fields[k].second;
            if (k + 1 < fields.size()) f << ",";
            f << "\n";
        }
        f << indent << "  }";
        if (i + 1 < items.size()) f << ",";
        f << "\n";
    }
    f << indent << "]";
}

} // namespace

const std::unordered_set<std::string> StructGenerator::keywords = {
    "klass", "monitor", "register", "_cs", "auto", "friend", "template", "flat", "default",
    "_ds", "interrupt", "unsigned", "signed", "asm", "if", "case", "break", "continue",
    "do", "new", "_", "short", "union", "class", "namespace"
};

const std::unordered_set<std::string> StructGenerator::specialKeywords = {
    "inline", "near", "far"
};

StructGenerator::StructGenerator(Il2CppExecutor& executor)
    : executor(executor), metadata(executor.metadata), il2Cpp(executor.il2Cpp) {}

std::string StructGenerator::fixName(const std::string& input) {
    std::string str = input;
    if (keywords.count(str)) {
        str = "_" + str;
    } else if (specialKeywords.count(str)) {
        str = "_" + str + "_";
    }
    if (!str.empty() && str[0] >= '0' && str[0] <= '9') return "_" + str;
    // [^a-zA-Z0-9_] -> '_' per UTF-16 code unit, as the .NET regex does.
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size();) {
        auto c = static_cast<unsigned char>(str[i]);
        if (c < 0x80) {
            bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
            result += ok ? static_cast<char>(c) : '_';
            i++;
            continue;
        }
        result += decodeUtf8(str, i) > 0xFFFF ? "__" : "_";
    }
    return result;
}

const Il2CppType& StructGenerator::typeAt(int64_t index) const {
    if (index < 0 || static_cast<uint64_t>(index) >= il2Cpp.types.size())
        throw std::out_of_range("type index out of range: " + std::to_string(index));
    return il2Cpp.types[index];
}

const Il2CppType& StructGenerator::typeFromPointer(uint64_t pointer) {
    auto* type = il2Cpp.getIl2CppType(pointer);
    if (!type) throw std::out_of_range("unknown Il2CppType pointer 0x" + hexUpper(pointer));
    return *type;
}

const Il2CppType& StructGenerator::genericArgument(const Il2CppType& il2CppType, uint64_t genericInstPointer) {
    auto& genericParameter = executor.getGenericParameterFromIl2CppType(il2CppType);
    auto genericInst = il2Cpp.readClass<Il2CppGenericInst>(il2Cpp.mapVATR(genericInstPointer));
    auto pointers = il2Cpp.readPointerArray(il2Cpp.mapVATR(genericInst.type_argv), genericInst.type_argc);
    return typeFromPointer(pointers.at(genericParameter.num));
}

const Il2CppTypeDefinition& StructGenerator::genericClassTypeDefinition(uint64_t genericClassPointer, Il2CppGenericClass* out) {
    auto genericClass = il2Cpp.readClass<Il2CppGenericClass>(il2Cpp.mapVATR(genericClassPointer));
    auto* typeDef = executor.getGenericClassTypeDefinition(genericClass);
    if (!typeDef) throw std::out_of_range("unresolved generic class 0x" + hexUpper(genericClassPointer));
    if (out) *out = genericClass;
    return *typeDef;
}

int StructGenerator::typeDefIndexOf(const Il2CppTypeDefinition& typeDef) const {
    auto base = reinterpret_cast<uintptr_t>(metadata.typeDefs.data());
    auto addr = reinterpret_cast<uintptr_t>(&typeDef);
    auto size = metadata.typeDefs.size() * sizeof(Il2CppTypeDefinition);
    if (addr < base || addr >= base + size)
        throw std::out_of_range("type definition is not part of metadata");
    return static_cast<int>((addr - base) / sizeof(Il2CppTypeDefinition));
}

const std::string& StructGenerator::structNameOf(const Il2CppTypeDefinition& typeDef) const {
    return structNameDic.at(typeDefIndexOf(typeDef));
}

void StructGenerator::writeScript(const std::string& outputDir) {
    LOGI("Generating struct info...");
    ScriptJson json;

    for (auto& imageDef : metadata.imageDefs) {
        auto imageName = metadata.getStringFromIndex(imageDef.nameIndex);
        int64_t typeEnd = static_cast<int64_t>(imageDef.typeStart) + imageDef.typeCount;
        for (int64_t typeIndex = imageDef.typeStart; typeIndex < typeEnd; typeIndex++) {
            auto& typeDef = metadata.typeDefs.at(typeIndex);
            typeDefImageNames[static_cast<int>(typeIndex)] = imageName;
            createStructNameDic(typeDef, static_cast<int>(typeIndex));
        }
    }

    for (auto& il2CppType : il2Cpp.types) {
        if (il2CppType.type != Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST) continue;
        auto genericClass = il2Cpp.readClass<Il2CppGenericClass>(il2Cpp.mapVATR(il2CppType.generic_class()));
        auto* typeDef = executor.getGenericClassTypeDefinition(genericClass);
        if (!typeDef) continue;
        auto& typeBaseName = structNameOf(*typeDef);
        auto typeToReplaceName = fixName(executor.getTypeDefName(*typeDef, true, true));
        auto typeReplaceName = fixName(executor.getTypeName(il2CppType, true, false));
        auto typeStructName = replaceAll(typeBaseName, typeToReplaceName, typeReplaceName);
        nameGenericClassDic[typeStructName] = &il2CppType;
        genericClassStructNameDic[il2CppType.generic_class()] = typeStructName;
    }

    auto appendParameters = [&](const Il2CppMethodDefinition& methodDef, const Il2CppGenericContext* context,
                                std::vector<Il2CppTypeEnum>& methodTypeSignature, std::vector<std::string>& parameterStrs) {
        for (int j = 0; j < methodDef.parameterCount; j++) {
            auto& parameterDef = metadata.parameterDefs.at(static_cast<int64_t>(methodDef.parameterStart) + j);
            auto parameterName = metadata.getStringFromIndex(parameterDef.nameIndex);
            auto& parameterType = typeAt(parameterDef.typeIndex);
            auto parameterCType = parseType(parameterType, context);
            if (parameterType.byref == 1) parameterCType += "*";
            methodTypeSignature.push_back(parameterType.byref == 1 ? Il2CppTypeEnum::IL2CPP_TYPE_PTR : parameterType.type);
            parameterStrs.push_back(parameterCType + " " + fixName(parameterName));
        }
    };

    for (auto& imageDef : metadata.imageDefs) {
        if (g_cancelled.load()) break;
        auto imageName = metadata.getStringFromIndex(imageDef.nameIndex);
        int64_t typeEnd = static_cast<int64_t>(imageDef.typeStart) + imageDef.typeCount;
        for (int64_t typeIndex = imageDef.typeStart; typeIndex < typeEnd; typeIndex++) {
            auto& typeDef = metadata.typeDefs.at(typeIndex);
            addStruct(typeDef, static_cast<int>(typeIndex));
            auto typeName = executor.getTypeDefName(typeDef, true, true);
            int64_t methodEnd = static_cast<int64_t>(typeDef.methodStart) + typeDef.method_count;
            for (int64_t i = typeDef.methodStart; i < methodEnd; ++i) {
                auto& methodDef = metadata.methodDefs.at(i);
                auto methodName = metadata.getStringFromIndex(methodDef.nameIndex);
                auto methodPointer = il2Cpp.getMethodPointer(imageName, methodDef);
                if (methodPointer > 0) {
                    std::vector<Il2CppTypeEnum> methodTypeSignature;
                    ScriptMethod scriptMethod;
                    scriptMethod.address = il2Cpp.getRVA(methodPointer);
                    auto methodFullName = typeName + "$$" + methodName;
                    scriptMethod.name = methodFullName;

                    auto& methodReturnType = typeAt(methodDef.returnType);
                    auto returnType = parseType(methodReturnType);
                    if (methodReturnType.byref == 1) returnType += "*";
                    methodTypeSignature.push_back(methodReturnType.byref == 1 ? Il2CppTypeEnum::IL2CPP_TYPE_PTR : methodReturnType.type);
                    auto signature = returnType + " " + fixName(methodFullName) + " (";
                    std::vector<std::string> parameterStrs;
                    if ((methodDef.flags & METHOD_ATTRIBUTE_STATIC) == 0) {
                        auto& thisIl2CppType = typeAt(typeDef.byvalTypeIndex);
                        auto thisType = parseType(thisIl2CppType);
                        methodTypeSignature.push_back(thisIl2CppType.type);
                        parameterStrs.push_back(thisType + " __this");
                    } else if (il2Cpp.version <= 24) {
                        methodTypeSignature.push_back(Il2CppTypeEnum::IL2CPP_TYPE_PTR);
                        parameterStrs.push_back("Il2CppObject* __this");
                    }
                    appendParameters(methodDef, nullptr, methodTypeSignature, parameterStrs);
                    methodTypeSignature.push_back(Il2CppTypeEnum::IL2CPP_TYPE_PTR);
                    parameterStrs.push_back("const MethodInfo* method");
                    signature += join(parameterStrs, ", ");
                    signature += ");";
                    scriptMethod.signature = signature;
                    scriptMethod.typeSignature = getMethodTypeSignature(methodTypeSignature);
                    json.scriptMethod.push_back(std::move(scriptMethod));
                }

                auto specIt = il2Cpp.methodDefinitionMethodSpecs.find(static_cast<int>(i));
                if (specIt == il2Cpp.methodDefinitionMethodSpecs.end()) continue;
                for (auto& methodSpec : specIt->second) {
                    auto genericMethodPointer = il2Cpp.methodSpecGenericMethodPointers.at(methodSpec);
                    if (genericMethodPointer == 0) continue;
                    std::vector<Il2CppTypeEnum> methodTypeSignature;
                    ScriptMethod scriptMethod;
                    scriptMethod.address = il2Cpp.getRVA(genericMethodPointer);
                    auto methodInfoName = "MethodInfo_" + hexUpper(scriptMethod.address);
                    auto& structTypeName = structNameDic.at(static_cast<int>(typeIndex));
                    auto rgctxs = generateRGCTX(imageName, methodDef);
                    if (methodInfoCache.insert(genericMethodPointer).second) {
                        generateMethodInfo(methodInfoName, structTypeName, rgctxs);
                    }
                    auto [methodSpecTypeName, methodSpecMethodName] = executor.getMethodSpecName(methodSpec, true);
                    auto methodFullName = methodSpecTypeName + "$$" + methodSpecMethodName;
                    scriptMethod.name = methodFullName;

                    auto genericContext = executor.getMethodSpecGenericContext(methodSpec);
                    auto& methodReturnType = typeAt(methodDef.returnType);
                    auto returnType = parseType(methodReturnType, &genericContext);
                    if (methodReturnType.byref == 1) returnType += "*";
                    methodTypeSignature.push_back(methodReturnType.byref == 1 ? Il2CppTypeEnum::IL2CPP_TYPE_PTR : methodReturnType.type);
                    auto signature = returnType + " " + fixName(methodFullName) + " (";
                    std::vector<std::string> parameterStrs;
                    if ((methodDef.flags & METHOD_ATTRIBUTE_STATIC) == 0) {
                        std::string thisType;
                        const Il2CppType* genericInstType = nullptr;
                        if (methodSpec.classIndexIndex != -1) {
                            auto typeStructName = replaceAll(structTypeName, fixName(typeName), fixName(methodSpecTypeName));
                            auto it = nameGenericClassDic.find(typeStructName);
                            if (it != nameGenericClassDic.end()) genericInstType = it->second;
                        }
                        if (genericInstType) {
                            thisType = parseType(*genericInstType);
                            methodTypeSignature.push_back(genericInstType->type);
                        } else {
                            auto& thisIl2CppType = typeAt(typeDef.byvalTypeIndex);
                            thisType = parseType(thisIl2CppType);
                            methodTypeSignature.push_back(thisIl2CppType.type);
                        }
                        parameterStrs.push_back(thisType + " __this");
                    } else if (il2Cpp.version <= 24) {
                        methodTypeSignature.push_back(Il2CppTypeEnum::IL2CPP_TYPE_PTR);
                        parameterStrs.push_back("Il2CppObject* __this");
                    }
                    appendParameters(methodDef, &genericContext, methodTypeSignature, parameterStrs);
                    methodTypeSignature.push_back(Il2CppTypeEnum::IL2CPP_TYPE_PTR);
                    parameterStrs.push_back("const " + methodInfoName + "* method");
                    signature += join(parameterStrs, ", ");
                    signature += ");";
                    scriptMethod.signature = signature;
                    scriptMethod.typeSignature = getMethodTypeSignature(methodTypeSignature);
                    json.scriptMethod.push_back(std::move(scriptMethod));
                }
            }
        }
    }

    {
        std::vector<uint64_t> orderedPointers;
        if (il2Cpp.version >= 24.2) {
            for (auto& [name, ptrs] : il2Cpp.codeGenModuleMethodPointers)
                orderedPointers.insert(orderedPointers.end(), ptrs.begin(), ptrs.end());
        } else {
            orderedPointers = il2Cpp.methodPointers;
        }
        orderedPointers.insert(orderedPointers.end(), il2Cpp.genericMethodPointers.begin(), il2Cpp.genericMethodPointers.end());
        orderedPointers.insert(orderedPointers.end(), il2Cpp.invokerPointers.begin(), il2Cpp.invokerPointers.end());
        if (il2Cpp.version < 29) {
            orderedPointers.insert(orderedPointers.end(), executor.customAttributeGenerators.begin(), executor.customAttributeGenerators.end());
        }
        if (il2Cpp.version >= 22) {
            orderedPointers.insert(orderedPointers.end(), il2Cpp.reversePInvokeWrappers.begin(), il2Cpp.reversePInvokeWrappers.end());
            orderedPointers.insert(orderedPointers.end(), il2Cpp.unresolvedVirtualCallPointers.begin(), il2Cpp.unresolvedVirtualCallPointers.end());
        }
        std::sort(orderedPointers.begin(), orderedPointers.end());
        orderedPointers.erase(std::unique(orderedPointers.begin(), orderedPointers.end()), orderedPointers.end());
        orderedPointers.erase(std::remove(orderedPointers.begin(), orderedPointers.end(), 0ULL), orderedPointers.end());
        json.addresses.reserve(orderedPointers.size());
        for (auto p : orderedPointers) json.addresses.push_back(il2Cpp.getRVA(p));
    }

    if (il2Cpp.version >= 27) {
        std::unique_ptr<SectionHelper> sectionHelper(executor.getSectionHelper());
        if (sectionHelper) {
            auto pointerSize = il2Cpp.getPointerSize();
            for (auto& sec : sectionHelper->getData()) {
                auto limit = std::min<uint64_t>(sec.offsetEnd, il2Cpp.getLength());
                if (limit < pointerSize) continue;
                auto end = limit - pointerSize;
                il2Cpp.setPosition(sec.offset);
                while (il2Cpp.getPosition() < end) {
                    auto addr = il2Cpp.getPosition();
                    auto metadataValue = il2Cpp.readUIntPtr();
                    auto position = il2Cpp.getPosition();
                    if (metadataValue >= UINT32_MAX) continue;
                    auto encodedToken = static_cast<uint32_t>(metadataValue);
                    auto usage = Metadata::getEncodedIndexType(encodedToken);
                    if (usage == 0 || usage > 6) continue;
                    auto decodedIndex = metadata.getDecodedMethodIndex(encodedToken);
                    if (metadataValue != static_cast<uint64_t>(((usage << 29) | (decodedIndex << 1)) + 1u)) continue;
                    auto va = il2Cpp.mapRTVA(addr);
                    if (va == 0) continue;
                    switch (static_cast<Il2CppMetadataUsage>(usage)) {
                        case Il2CppMetadataUsage::kIl2CppMetadataUsageTypeInfo:
                            if (decodedIndex < il2Cpp.types.size()) addMetadataUsageTypeInfo(json, decodedIndex, va);
                            break;
                        case Il2CppMetadataUsage::kIl2CppMetadataUsageIl2CppType:
                            if (decodedIndex < il2Cpp.types.size()) addMetadataUsageIl2CppType(json, decodedIndex, va);
                            break;
                        case Il2CppMetadataUsage::kIl2CppMetadataUsageMethodDef:
                            if (decodedIndex < metadata.methodDefs.size()) addMetadataUsageMethodDef(json, decodedIndex, va);
                            break;
                        case Il2CppMetadataUsage::kIl2CppMetadataUsageFieldInfo:
                            if (decodedIndex < metadata.fieldRefs.size()) addMetadataUsageFieldInfo(json, decodedIndex, va);
                            break;
                        case Il2CppMetadataUsage::kIl2CppMetadataUsageStringLiteral:
                            if (decodedIndex < metadata.stringLiterals.size()) addMetadataUsageStringLiteral(json, decodedIndex, va);
                            break;
                        case Il2CppMetadataUsage::kIl2CppMetadataUsageMethodRef:
                            if (decodedIndex < il2Cpp.methodSpecs.size()) addMetadataUsageMethodRef(json, decodedIndex, va);
                            break;
                        default:
                            break;
                    }
                    il2Cpp.setPosition(position);
                }
            }
        }
    } else if (il2Cpp.version > 16 && il2Cpp.version < 27) {
        auto processUsage = [&](Il2CppMetadataUsage usage, void (StructGenerator::*handler)(ScriptJson&, uint32_t, uint64_t)) {
            auto it = metadata.metadataUsageDic.find(static_cast<int>(usage));
            if (it == metadata.metadataUsageDic.end()) return;
            for (auto& [destinationIndex, decodedIndex] : it->second) {
                (this->*handler)(json, decodedIndex, il2Cpp.metadataUsages.at(destinationIndex));
            }
        };
        processUsage(Il2CppMetadataUsage::kIl2CppMetadataUsageTypeInfo, &StructGenerator::addMetadataUsageTypeInfo);
        processUsage(Il2CppMetadataUsage::kIl2CppMetadataUsageIl2CppType, &StructGenerator::addMetadataUsageIl2CppType);
        processUsage(Il2CppMetadataUsage::kIl2CppMetadataUsageMethodDef, &StructGenerator::addMetadataUsageMethodDef);
        processUsage(Il2CppMetadataUsage::kIl2CppMetadataUsageFieldInfo, &StructGenerator::addMetadataUsageFieldInfo);
        processUsage(Il2CppMetadataUsage::kIl2CppMetadataUsageStringLiteral, &StructGenerator::addMetadataUsageStringLiteral);
        processUsage(Il2CppMetadataUsage::kIl2CppMetadataUsageMethodRef, &StructGenerator::addMetadataUsageMethodRef);
    }

    {
        std::ofstream f(outputDir + "/stringliteral.json", std::ios::binary);
        writeObjectArray(f, "", json.scriptString, [](const ScriptString& s) {
            return JsonFields{{"value", jsonString(s.value)}, {"address", jsonString("0x" + hexUpper(s.address))}};
        });
        if (!f) throw std::runtime_error("failed to write stringliteral.json");
        LOGI("stringliteral.json written: %zu entries", json.scriptString.size());
    }

    {
        std::ofstream f(outputDir + "/script.json", std::ios::binary);
        f << "{\n  \"ScriptMethod\": ";
        writeObjectArray(f, "  ", json.scriptMethod, [](const ScriptMethod& m) {
            return JsonFields{{"Address", std::to_string(m.address)}, {"Name", jsonString(m.name)},
                              {"Signature", jsonString(m.signature)}, {"TypeSignature", jsonString(m.typeSignature)}};
        });
        f << ",\n  \"ScriptString\": ";
        writeObjectArray(f, "  ", json.scriptString, [](const ScriptString& s) {
            return JsonFields{{"Address", std::to_string(s.address)}, {"Value", jsonString(s.value)}};
        });
        f << ",\n  \"ScriptMetadata\": ";
        writeObjectArray(f, "  ", json.scriptMetadata, [](const ScriptMetadata& m) {
            return JsonFields{{"Address", std::to_string(m.address)}, {"Name", jsonString(m.name)},
                              {"Signature", jsonString(m.signature)}};
        });
        f << ",\n  \"ScriptMetadataMethod\": ";
        writeObjectArray(f, "  ", json.scriptMetadataMethod, [](const ScriptMetadataMethod& m) {
            return JsonFields{{"Address", std::to_string(m.address)}, {"Name", jsonString(m.name)},
                              {"MethodAddress", std::to_string(m.methodAddress)}};
        });
        f << ",\n  \"Addresses\": ";
        if (json.addresses.empty()) {
            f << "[]";
        } else {
            f << "[\n";
            for (size_t i = 0; i < json.addresses.size(); i++) {
                f << "    " << json.addresses[i];
                if (i + 1 < json.addresses.size()) f << ",";
                f << "\n";
            }
            f << "  ]";
        }
        f << "\n}";
        if (!f) throw std::runtime_error("failed to write script.json");
        LOGI("script.json written: %zu methods, %zu metadata, %zu addresses",
             json.scriptMethod.size(), json.scriptMetadata.size(), json.addresses.size());
    }

    for (size_t i = 0; i < genericClassList.size(); i++) {
        addGenericClassStruct(genericClassList[i]);
    }
    for (auto& info : structInfoList) {
        structInfoWithStructName.emplace(info.typeName + "_o", &info);
    }
    std::string headerStruct;
    for (auto& info : structInfoList) {
        recursionStructInfo(info, headerStruct);
    }

    const std::string* versionHeader = nullptr;
    auto ver = il2Cpp.version;
    if (ver == 22) versionHeader = &HeaderConstants::HeaderV22;
    else if (ver == 23 || ver == 24) versionHeader = &HeaderConstants::HeaderV240;
    else if (ver == 24.1) versionHeader = &HeaderConstants::HeaderV241;
    else if (ver == 24.2 || ver == 24.3 || ver == 24.4 || ver == 24.5) versionHeader = &HeaderConstants::HeaderV242;
    else if (ver == 27 || ver == 27.1 || ver == 27.2) versionHeader = &HeaderConstants::HeaderV27;
    else if (ver == 29 || ver == 29.1 || ver == 31) versionHeader = &HeaderConstants::HeaderV29;
    if (!versionHeader) {
        LOGI("WARNING: This il2cpp version [%g] does not support generating .h files", ver);
        return;
    }

    {
        std::ofstream f(outputDir + "/il2cpp.h", std::ios::binary);
        f << HeaderConstants::GenericHeader << *versionHeader << headerStruct
          << arrayClassHeader.str() << methodInfoHeader.str();
        if (!f) throw std::runtime_error("failed to write il2cpp.h");
        LOGI("il2cpp.h written: %zu structs", structInfoList.size());
    }

    LOGI("Struct generation complete");
}

void StructGenerator::addMetadataUsageTypeInfo(ScriptJson& json, uint32_t index, uint64_t address) {
    auto& type = typeAt(index);
    auto typeName = executor.getTypeName(type, true, false);
    ScriptMetadata scriptMetadata;
    scriptMetadata.address = il2Cpp.getRVA(address);
    scriptMetadata.name = typeName + "_TypeInfo";
    auto signature = getIl2CppStructName(type);
    if (signature.size() >= 6 && signature.compare(signature.size() - 6, 6, "_array") == 0) {
        scriptMetadata.signature = "Il2CppClass*";
    } else {
        scriptMetadata.signature = fixName(signature) + "_c*";
    }
    json.scriptMetadata.push_back(std::move(scriptMetadata));
}

void StructGenerator::addMetadataUsageIl2CppType(ScriptJson& json, uint32_t index, uint64_t address) {
    auto& type = typeAt(index);
    auto typeName = executor.getTypeName(type, true, false);
    ScriptMetadata scriptMetadata;
    scriptMetadata.address = il2Cpp.getRVA(address);
    scriptMetadata.name = typeName + "_var";
    scriptMetadata.signature = "Il2CppType*";
    json.scriptMetadata.push_back(std::move(scriptMetadata));
}

void StructGenerator::addMetadataUsageMethodDef(ScriptJson& json, uint32_t index, uint64_t address) {
    auto& methodDef = metadata.methodDefs.at(index);
    auto& typeDef = metadata.typeDefs.at(methodDef.declaringType);
    auto typeName = executor.getTypeDefName(typeDef, true, true);
    auto methodName = typeName + "." + metadata.getStringFromIndex(methodDef.nameIndex) + "()";
    ScriptMetadataMethod scriptMetadataMethod;
    scriptMetadataMethod.address = il2Cpp.getRVA(address);
    scriptMetadataMethod.name = "Method$" + methodName;
    auto& imageName = typeDefImageNames.at(methodDef.declaringType);
    auto methodPointer = il2Cpp.getMethodPointer(imageName, methodDef);
    if (methodPointer > 0) {
        scriptMetadataMethod.methodAddress = il2Cpp.getRVA(methodPointer);
    }
    json.scriptMetadataMethod.push_back(std::move(scriptMetadataMethod));
}

void StructGenerator::addMetadataUsageFieldInfo(ScriptJson& json, uint32_t index, uint64_t address) {
    auto& fieldRef = metadata.fieldRefs.at(index);
    auto& type = typeAt(fieldRef.typeIndex);
    auto& typeDef = getTypeDefinition(type);
    auto& fieldDef = metadata.fieldDefs.at(static_cast<int64_t>(typeDef.fieldStart) + fieldRef.fieldIndex);
    auto fieldName = executor.getTypeName(type, true, false) + "." + metadata.getStringFromIndex(fieldDef.nameIndex);
    ScriptMetadata scriptMetadata;
    scriptMetadata.address = il2Cpp.getRVA(address);
    scriptMetadata.name = "Field$" + fieldName;
    json.scriptMetadata.push_back(std::move(scriptMetadata));
}

void StructGenerator::addMetadataUsageStringLiteral(ScriptJson& json, uint32_t index, uint64_t address) {
    ScriptString scriptString;
    scriptString.address = il2Cpp.getRVA(address);
    scriptString.value = metadata.getStringLiteralFromIndex(index);
    json.scriptString.push_back(std::move(scriptString));
}

void StructGenerator::addMetadataUsageMethodRef(ScriptJson& json, uint32_t index, uint64_t address) {
    auto& methodSpec = il2Cpp.methodSpecs.at(index);
    ScriptMetadataMethod scriptMetadataMethod;
    scriptMetadataMethod.address = il2Cpp.getRVA(address);
    auto [methodSpecTypeName, methodSpecMethodName] = executor.getMethodSpecName(methodSpec, true);
    scriptMetadataMethod.name = "Method$" + methodSpecTypeName + "." + methodSpecMethodName + "()";
    auto it = il2Cpp.methodSpecGenericMethodPointers.find(methodSpec);
    if (it != il2Cpp.methodSpecGenericMethodPointers.end() && it->second > 0) {
        scriptMetadataMethod.methodAddress = il2Cpp.getRVA(it->second);
    }
    json.scriptMetadataMethod.push_back(std::move(scriptMetadataMethod));
}

std::string StructGenerator::parseType(const Il2CppType& il2CppType, const Il2CppGenericContext* context) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_VOID: return "void";
        case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN: return "bool";
        case Il2CppTypeEnum::IL2CPP_TYPE_CHAR: return "uint16_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_I1: return "int8_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U1: return "uint8_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_I2: return "int16_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U2: return "uint16_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_I4: return "int32_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U4: return "uint32_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_I8: return "int64_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U8: return "uint64_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_R4: return "float";
        case Il2CppTypeEnum::IL2CPP_TYPE_R8: return "double";
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING: return "System_String_o*";
        case Il2CppTypeEnum::IL2CPP_TYPE_PTR:
            return parseType(typeFromPointer(il2CppType.typeRef())) + "*";
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE: {
            auto& typeDef = executor.getTypeDefinitionFromIl2CppType(il2CppType);
            if (typeDef.isEnum()) return parseType(typeAt(typeDef.elementTypeIndex));
            return structNameOf(typeDef) + "_o";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS: {
            auto& typeDef = executor.getTypeDefinitionFromIl2CppType(il2CppType);
            return structNameOf(typeDef) + "_o*";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_VAR:
            if (context) return parseType(genericArgument(il2CppType, context->class_inst));
            return "Il2CppObject*";
        case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY: {
            auto arrayType = il2Cpp.readClass<Il2CppArrayType>(il2Cpp.mapVATR(il2CppType.array()));
            auto& elementType = typeFromPointer(arrayType.etype);
            auto typeStructName = getIl2CppStructName(elementType, context) + "_array";
            if (structNameHashSet.insert(typeStructName).second) {
                parseArrayClassStruct(elementType, context);
            }
            return typeStructName + "*";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST: {
            auto genericClass = il2Cpp.readClass<Il2CppGenericClass>(il2Cpp.mapVATR(il2CppType.generic_class()));
            auto* typeDef = executor.getGenericClassTypeDefinition(genericClass);
            auto& typeStructName = genericClassStructNameDic.at(il2CppType.generic_class());
            if (structNameHashSet.insert(typeStructName).second) {
                genericClassList.push_back(il2CppType.generic_class());
            }
            if (!typeDef) throw std::out_of_range("unresolved generic class");
            if (typeDef->isValueType()) {
                if (typeDef->isEnum()) return parseType(typeAt(typeDef->elementTypeIndex));
                return typeStructName + "_o";
            }
            return typeStructName + "_o*";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_TYPEDBYREF: return "Il2CppObject*";
        case Il2CppTypeEnum::IL2CPP_TYPE_I: return "intptr_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U: return "uintptr_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_OBJECT: return "Il2CppObject*";
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY: {
            auto& elementType = typeFromPointer(il2CppType.typeRef());
            auto typeStructName = getIl2CppStructName(elementType, context) + "_array";
            if (structNameHashSet.insert(typeStructName).second) {
                parseArrayClassStruct(elementType, context);
            }
            return typeStructName + "*";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_MVAR:
            if (context) {
                // https://github.com/Perfare/Il2CppDumper/issues/687
                if (context->method_inst == 0 && context->class_inst != 0) {
                    return parseType(genericArgument(il2CppType, context->class_inst));
                }
                return parseType(genericArgument(il2CppType, context->method_inst));
            }
            return "Il2CppObject*";
        default:
            throw std::invalid_argument("parseType: unsupported type " + std::to_string(static_cast<int>(il2CppType.type)));
    }
}

std::string StructGenerator::getMethodTypeSignature(const std::vector<Il2CppTypeEnum>& types) {
    std::string signature;
    for (auto type : types) {
        switch (type) {
            case Il2CppTypeEnum::IL2CPP_TYPE_VOID:
                signature += "v"; break;
            case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN:
            case Il2CppTypeEnum::IL2CPP_TYPE_CHAR:
            case Il2CppTypeEnum::IL2CPP_TYPE_I1:
            case Il2CppTypeEnum::IL2CPP_TYPE_U1:
            case Il2CppTypeEnum::IL2CPP_TYPE_I2:
            case Il2CppTypeEnum::IL2CPP_TYPE_U2:
            case Il2CppTypeEnum::IL2CPP_TYPE_I4:
            case Il2CppTypeEnum::IL2CPP_TYPE_U4:
                signature += "i"; break;
            case Il2CppTypeEnum::IL2CPP_TYPE_I8:
            case Il2CppTypeEnum::IL2CPP_TYPE_U8:
                signature += "j"; break;
            case Il2CppTypeEnum::IL2CPP_TYPE_R4:
                signature += "f"; break;
            case Il2CppTypeEnum::IL2CPP_TYPE_R8:
                signature += "d"; break;
            case Il2CppTypeEnum::IL2CPP_TYPE_STRING:
            case Il2CppTypeEnum::IL2CPP_TYPE_PTR:
            case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
            case Il2CppTypeEnum::IL2CPP_TYPE_CLASS:
            case Il2CppTypeEnum::IL2CPP_TYPE_VAR:
            case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY:
            case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST:
            case Il2CppTypeEnum::IL2CPP_TYPE_TYPEDBYREF:
            case Il2CppTypeEnum::IL2CPP_TYPE_I:
            case Il2CppTypeEnum::IL2CPP_TYPE_U:
            case Il2CppTypeEnum::IL2CPP_TYPE_OBJECT:
            case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY:
            case Il2CppTypeEnum::IL2CPP_TYPE_MVAR:
                signature += "i"; break;
            default:
                throw std::invalid_argument("getMethodTypeSignature: unsupported type " + std::to_string(static_cast<int>(type)));
        }
    }
    return signature;
}

void StructGenerator::addStruct(const Il2CppTypeDefinition& typeDef, int typeDefIndex) {
    StructInfo info;
    info.typeName = structNameDic.at(typeDefIndex);
    info.isValueType = typeDef.isValueType();
    addParents(typeDef, info);
    addFields(typeDef, info, nullptr);
    addVTableMethod(info, typeDef);
    addRGCTX(info, typeDefIndex);
    structInfoList.push_back(std::move(info));
}

void StructGenerator::addGenericClassStruct(uint64_t pointer) {
    Il2CppGenericClass genericClass;
    auto& typeDef = genericClassTypeDefinition(pointer, &genericClass);
    StructInfo info;
    info.typeName = genericClassStructNameDic.at(pointer);
    info.isValueType = typeDef.isValueType();
    addParents(typeDef, info);
    Il2CppGenericContext context{genericClass.context_class_inst, genericClass.context_method_inst};
    addFields(typeDef, info, &context);
    addVTableMethod(info, typeDef);
    structInfoList.push_back(std::move(info));
}

void StructGenerator::addParents(const Il2CppTypeDefinition& typeDef, StructInfo& info) {
    if (!typeDef.isValueType() && !typeDef.isEnum()) {
        if (typeDef.parentIndex >= 0) {
            auto& parent = typeAt(typeDef.parentIndex);
            if (parent.type != Il2CppTypeEnum::IL2CPP_TYPE_OBJECT) {
                info.parent = getIl2CppStructName(parent);
            }
        }
    }
}

void StructGenerator::addFields(const Il2CppTypeDefinition& typeDef, StructInfo& info, const Il2CppGenericContext* context) {
    if (typeDef.field_count == 0) return;
    int64_t fieldEnd = static_cast<int64_t>(typeDef.fieldStart) + typeDef.field_count;
    std::unordered_set<std::string> cache;
    for (int64_t i = typeDef.fieldStart; i < fieldEnd; ++i) {
        auto& fieldDef = metadata.fieldDefs.at(i);
        auto& fieldType = typeAt(fieldDef.typeIndex);
        if (fieldType.attrs & FIELD_ATTRIBUTE_LITERAL) continue;
        StructFieldInfo structFieldInfo;
        structFieldInfo.fieldTypeName = parseType(fieldType, context);
        auto fieldName = fixName(metadata.getStringFromIndex(fieldDef.nameIndex));
        if (!cache.insert(fieldName).second) {
            fieldName = "_" + std::to_string(i - typeDef.fieldStart) + "_" + fieldName;
        }
        structFieldInfo.fieldName = fieldName;
        structFieldInfo.isValueType = isValueType(fieldType, context);
        structFieldInfo.isCustomType = isCustomType(fieldType, context);
        if (fieldType.attrs & FIELD_ATTRIBUTE_STATIC)
            info.staticFields.push_back(std::move(structFieldInfo));
        else
            info.fields.push_back(std::move(structFieldInfo));
    }
}

void StructGenerator::addVTableMethod(StructInfo& info, const Il2CppTypeDefinition& typeDef) {
    std::map<int, const Il2CppMethodDefinition*> dic;
    for (int i = 0; i < typeDef.vtable_count; i++) {
        int64_t vTableIndex = static_cast<int64_t>(typeDef.vtableStart) + i;
        auto encodedMethodIndex = metadata.vtableMethods.at(vTableIndex);
        auto usage = Metadata::getEncodedIndexType(encodedMethodIndex);
        auto index = metadata.getDecodedMethodIndex(encodedMethodIndex);
        const Il2CppMethodDefinition* methodDef;
        if (usage == 6) {
            auto& methodSpec = il2Cpp.methodSpecs.at(index);
            methodDef = &metadata.methodDefs.at(methodSpec.methodDefinitionIndex);
        } else {
            methodDef = &metadata.methodDefs.at(index);
        }
        if (methodDef->slot != 0xFFFF) {
            dic[methodDef->slot] = methodDef;
        }
    }
    if (!dic.empty()) {
        info.vTableMethod.assign(dic.rbegin()->first + 1, std::nullopt);
        for (auto& [slot, methodDef] : dic) {
            info.vTableMethod[slot] = StructVTableMethodInfo{fixName(metadata.getStringFromIndex(methodDef->nameIndex))};
        }
    }
}

std::vector<StructRGCTXInfo> StructGenerator::buildRGCTX(const std::vector<Il2CppRGCTXDefinition>* collection) {
    std::vector<StructRGCTXInfo> rgctxs;
    if (!collection) return rgctxs;
    for (auto& definitionData : *collection) {
        StructRGCTXInfo structRGCTXInfo;
        structRGCTXInfo.type = definitionData.getType();
        auto dataIndex = il2Cpp.getRGCTXDataIndex(definitionData);
        switch (structRGCTXInfo.type) {
            case Il2CppRGCTXDataType::IL2CPP_RGCTX_DATA_TYPE:
                structRGCTXInfo.typeName = fixName(executor.getTypeName(typeAt(dataIndex), true, false));
                break;
            case Il2CppRGCTXDataType::IL2CPP_RGCTX_DATA_CLASS:
                structRGCTXInfo.className = fixName(executor.getTypeName(typeAt(dataIndex), true, false));
                break;
            case Il2CppRGCTXDataType::IL2CPP_RGCTX_DATA_METHOD: {
                if (dataIndex < 0) throw std::out_of_range("negative RGCTX method index");
                auto [methodSpecTypeName, methodSpecMethodName] = executor.getMethodSpecName(il2Cpp.methodSpecs.at(dataIndex), true);
                structRGCTXInfo.methodName = fixName(methodSpecTypeName + "." + methodSpecMethodName);
                break;
            }
            default:
                break;
        }
        rgctxs.push_back(std::move(structRGCTXInfo));
    }
    return rgctxs;
}

void StructGenerator::addRGCTX(StructInfo& info, int typeDefIndex) {
    auto& imageName = typeDefImageNames.at(typeDefIndex);
    info.rgctxs = buildRGCTX(executor.getRGCTXDefinition(imageName, metadata.typeDefs.at(typeDefIndex)));
}

std::vector<StructRGCTXInfo> StructGenerator::generateRGCTX(const std::string& imageName, const Il2CppMethodDefinition& methodDef) {
    return buildRGCTX(executor.getRGCTXDefinition(imageName, methodDef));
}

void StructGenerator::parseArrayClassStruct(const Il2CppType& il2CppType, const Il2CppGenericContext* context) {
    auto structName = getIl2CppStructName(il2CppType, context);
    auto elementType = parseType(il2CppType, context);
    arrayClassHeader << "struct " << structName << "_array {\n"
                     << "\tIl2CppObject obj;\n"
                     << "\tIl2CppArrayBounds *bounds;\n"
                     << "\til2cpp_array_size_t max_length;\n"
                     << "\t" << elementType << " m_Items[65535];\n"
                     << "};\n";
}

const Il2CppTypeDefinition& StructGenerator::getTypeDefinition(const Il2CppType& il2CppType) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_VOID:
        case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN:
        case Il2CppTypeEnum::IL2CPP_TYPE_CHAR:
        case Il2CppTypeEnum::IL2CPP_TYPE_I1:
        case Il2CppTypeEnum::IL2CPP_TYPE_U1:
        case Il2CppTypeEnum::IL2CPP_TYPE_I2:
        case Il2CppTypeEnum::IL2CPP_TYPE_U2:
        case Il2CppTypeEnum::IL2CPP_TYPE_I4:
        case Il2CppTypeEnum::IL2CPP_TYPE_U4:
        case Il2CppTypeEnum::IL2CPP_TYPE_I8:
        case Il2CppTypeEnum::IL2CPP_TYPE_U8:
        case Il2CppTypeEnum::IL2CPP_TYPE_R4:
        case Il2CppTypeEnum::IL2CPP_TYPE_R8:
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING:
        case Il2CppTypeEnum::IL2CPP_TYPE_TYPEDBYREF:
        case Il2CppTypeEnum::IL2CPP_TYPE_I:
        case Il2CppTypeEnum::IL2CPP_TYPE_U:
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS:
        case Il2CppTypeEnum::IL2CPP_TYPE_OBJECT:
            return executor.getTypeDefinitionFromIl2CppType(il2CppType);
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST:
            return genericClassTypeDefinition(il2CppType.generic_class());
        default:
            throw std::invalid_argument("getTypeDefinition: unsupported type " + std::to_string(static_cast<int>(il2CppType.type)));
    }
}

void StructGenerator::createStructNameDic(const Il2CppTypeDefinition& typeDef, int typeDefIndex) {
    auto typeName = executor.getTypeDefName(typeDef, true, true);
    auto typeStructName = fixName(typeName);
    auto uniqueName = getUniqueName(typeStructName);
    structNameDic[typeDefIndex] = uniqueName;
}

std::string StructGenerator::getUniqueName(const std::string& name) {
    auto fixName = name;
    int i = 1;
    while (!structNameHashSet.insert(fixName).second) {
        fixName = name + "_" + std::to_string(i++);
    }
    return fixName;
}

static void appendRGCTXFields(std::string& out, const std::vector<StructRGCTXInfo>& rgctxs) {
    for (size_t i = 0; i < rgctxs.size(); i++) {
        auto& rgctx = rgctxs[i];
        auto index = std::to_string(i);
        switch (rgctx.type) {
            case Il2CppRGCTXDataType::IL2CPP_RGCTX_DATA_TYPE:
                out += "\tIl2CppType* _" + index + "_" + rgctx.typeName + ";\n"; break;
            case Il2CppRGCTXDataType::IL2CPP_RGCTX_DATA_CLASS:
                out += "\tIl2CppClass* _" + index + "_" + rgctx.className + ";\n"; break;
            case Il2CppRGCTXDataType::IL2CPP_RGCTX_DATA_METHOD:
                out += "\tMethodInfo* _" + index + "_" + rgctx.methodName + ";\n"; break;
            default: break;
        }
    }
}

// Dependencies (parent / value-type fields) are emitted into `out` before this struct.
void StructGenerator::recursionStructInfo(const StructInfo& info, std::string& out) {
    if (!structCache.insert(&info).second) return;

    std::string sb;
    if (!info.parent.empty()) {
        recursionStructInfo(*structInfoWithStructName.at(info.parent + "_o"), out);
        sb += "struct " + info.typeName + "_Fields : " + info.parent + "_Fields {\n";
    } else {
        sb += "struct " + info.typeName + "_Fields {\n";
    }
    auto appendField = [&](const StructFieldInfo& field) {
        if (field.isValueType) {
            recursionStructInfo(*structInfoWithStructName.at(field.fieldTypeName), out);
        }
        if (field.isCustomType)
            sb += "\tstruct " + field.fieldTypeName + " " + field.fieldName + ";\n";
        else
            sb += "\t" + field.fieldTypeName + " " + field.fieldName + ";\n";
    };
    for (auto& field : info.fields) appendField(field);
    sb += "};\n";

    if (!info.rgctxs.empty()) {
        sb += "struct " + info.typeName + "_RGCTXs {\n";
        appendRGCTXFields(sb, info.rgctxs);
        sb += "};\n";
    }

    if (!info.vTableMethod.empty()) {
        sb += "struct " + info.typeName + "_VTable {\n";
        for (size_t i = 0; i < info.vTableMethod.size(); i++) {
            sb += "\tVirtualInvokeData _" + std::to_string(i) + "_";
            sb += info.vTableMethod[i] ? info.vTableMethod[i]->methodName : "unknown";
            sb += ";\n";
        }
        sb += "};\n";
    }

    sb += "struct " + info.typeName + "_c {\n";
    sb += "\tIl2CppClass_1 _1;\n";
    if (!info.staticFields.empty())
        sb += "\tstruct " + info.typeName + "_StaticFields* static_fields;\n";
    else
        sb += "\tvoid* static_fields;\n";
    if (!info.rgctxs.empty())
        sb += "\t" + info.typeName + "_RGCTXs* rgctx_data;\n";
    else
        sb += "\tIl2CppRGCTXData* rgctx_data;\n";
    sb += "\tIl2CppClass_2 _2;\n";
    if (!info.vTableMethod.empty())
        sb += "\t" + info.typeName + "_VTable vtable;\n";
    else
        sb += "\tVirtualInvokeData vtable[32];\n";
    sb += "};\n";

    sb += "struct " + info.typeName + "_o {\n";
    if (!info.isValueType) {
        sb += "\t" + info.typeName + "_c *klass;\n";
        sb += "\tvoid *monitor;\n";
    }
    sb += "\t" + info.typeName + "_Fields fields;\n";
    sb += "};\n";

    if (!info.staticFields.empty()) {
        sb += "struct " + info.typeName + "_StaticFields {\n";
        for (auto& field : info.staticFields) appendField(field);
        sb += "};\n";
    }

    out += sb;
}

std::string StructGenerator::getIl2CppStructName(const Il2CppType& il2CppType, const Il2CppGenericContext* context) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_VOID:
        case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN:
        case Il2CppTypeEnum::IL2CPP_TYPE_CHAR:
        case Il2CppTypeEnum::IL2CPP_TYPE_I1:
        case Il2CppTypeEnum::IL2CPP_TYPE_U1:
        case Il2CppTypeEnum::IL2CPP_TYPE_I2:
        case Il2CppTypeEnum::IL2CPP_TYPE_U2:
        case Il2CppTypeEnum::IL2CPP_TYPE_I4:
        case Il2CppTypeEnum::IL2CPP_TYPE_U4:
        case Il2CppTypeEnum::IL2CPP_TYPE_I8:
        case Il2CppTypeEnum::IL2CPP_TYPE_U8:
        case Il2CppTypeEnum::IL2CPP_TYPE_R4:
        case Il2CppTypeEnum::IL2CPP_TYPE_R8:
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING:
        case Il2CppTypeEnum::IL2CPP_TYPE_TYPEDBYREF:
        case Il2CppTypeEnum::IL2CPP_TYPE_I:
        case Il2CppTypeEnum::IL2CPP_TYPE_U:
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS:
        case Il2CppTypeEnum::IL2CPP_TYPE_OBJECT:
            return structNameOf(executor.getTypeDefinitionFromIl2CppType(il2CppType));
        case Il2CppTypeEnum::IL2CPP_TYPE_PTR:
            return getIl2CppStructName(typeFromPointer(il2CppType.typeRef()));
        case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY: {
            auto arrayType = il2Cpp.readClass<Il2CppArrayType>(il2Cpp.mapVATR(il2CppType.array()));
            auto& elementType = typeFromPointer(arrayType.etype);
            auto typeStructName = getIl2CppStructName(elementType, context) + "_array";
            if (structNameHashSet.insert(typeStructName).second) {
                parseArrayClassStruct(elementType, context);
            }
            return typeStructName;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY: {
            auto& elementType = typeFromPointer(il2CppType.typeRef());
            auto typeStructName = getIl2CppStructName(elementType, context) + "_array";
            if (structNameHashSet.insert(typeStructName).second) {
                parseArrayClassStruct(elementType, context);
            }
            return typeStructName;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST: {
            auto& typeStructName = genericClassStructNameDic.at(il2CppType.generic_class());
            if (structNameHashSet.insert(typeStructName).second) {
                genericClassList.push_back(il2CppType.generic_class());
            }
            return typeStructName;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_VAR:
            if (context) return getIl2CppStructName(genericArgument(il2CppType, context->class_inst));
            return "System_Object";
        case Il2CppTypeEnum::IL2CPP_TYPE_MVAR:
            if (context) return getIl2CppStructName(genericArgument(il2CppType, context->method_inst));
            return "System_Object";
        default:
            throw std::invalid_argument("getIl2CppStructName: unsupported type " + std::to_string(static_cast<int>(il2CppType.type)));
    }
}

bool StructGenerator::isValueType(const Il2CppType& il2CppType, const Il2CppGenericContext* context) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
            return !executor.getTypeDefinitionFromIl2CppType(il2CppType).isEnum();
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST: {
            auto& typeDef = genericClassTypeDefinition(il2CppType.generic_class());
            return typeDef.isValueType() && !typeDef.isEnum();
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_VAR:
            if (context) return isValueType(genericArgument(il2CppType, context->class_inst), nullptr);
            return false;
        case Il2CppTypeEnum::IL2CPP_TYPE_MVAR:
            if (context) return isValueType(genericArgument(il2CppType, context->method_inst), nullptr);
            return false;
        default:
            return false;
    }
}

bool StructGenerator::isCustomType(const Il2CppType& il2CppType, const Il2CppGenericContext* context) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_PTR:
            return isCustomType(typeFromPointer(il2CppType.typeRef()), context);
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING:
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS:
        case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY:
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY:
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE: {
            auto& typeDef = executor.getTypeDefinitionFromIl2CppType(il2CppType);
            if (typeDef.isEnum()) return isCustomType(typeAt(typeDef.elementTypeIndex), context);
            return true;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST: {
            auto& typeDef = genericClassTypeDefinition(il2CppType.generic_class());
            if (typeDef.isEnum()) return isCustomType(typeAt(typeDef.elementTypeIndex), context);
            return true;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_VAR:
            if (context) return isCustomType(genericArgument(il2CppType, context->class_inst), nullptr);
            return false;
        case Il2CppTypeEnum::IL2CPP_TYPE_MVAR:
            if (context) return isCustomType(genericArgument(il2CppType, context->method_inst), nullptr);
            return false;
        default:
            return false;
    }
}

void StructGenerator::generateMethodInfo(const std::string& methodInfoName, const std::string& structTypeName,
                                         const std::vector<StructRGCTXInfo>& rgctxs) {
    std::string sb;
    if (!rgctxs.empty()) {
        sb += "struct " + methodInfoName + "_RGCTXs {\n";
        appendRGCTXFields(sb, rgctxs);
        sb += "};\n";
    }
    sb += "struct " + methodInfoName + " {\n";
    sb += "\tIl2CppMethodPointer methodPointer;\n";
    if (il2Cpp.version >= 29) {
        sb += "\tIl2CppMethodPointer virtualMethodPointer;\n";
        sb += "\tInvokerMethod invoker_method;\n";
    } else {
        sb += "\tvoid* invoker_method;\n";
    }
    sb += "\tconst char* name;\n";
    if (il2Cpp.version <= 24)
        sb += "\t" + structTypeName + "_c *declaring_type;\n";
    else
        sb += "\t" + structTypeName + "_c *klass;\n";
    sb += "\tconst Il2CppType *return_type;\n";
    if (il2Cpp.version >= 29)
        sb += "\tconst Il2CppType** parameters;\n";
    else
        sb += "\tconst void* parameters;\n";
    if (!rgctxs.empty())
        sb += "\tconst " + methodInfoName + "_RGCTXs* rgctx_data;\n";
    else
        sb += "\tconst Il2CppRGCTXData* rgctx_data;\n";
    sb += "\tunion\n";
    sb += "\t{\n";
    sb += "\t\tconst void* genericMethod;\n";
    if (il2Cpp.version >= 27)
        sb += "\t\tconst void* genericContainerHandle;\n";
    else
        sb += "\t\tconst void* genericContainer;\n";
    sb += "\t};\n";
    if (il2Cpp.version <= 24)
        sb += "\tint32_t customAttributeIndex;\n";
    sb += "\tuint32_t token;\n";
    sb += "\tuint16_t flags;\n";
    sb += "\tuint16_t iflags;\n";
    sb += "\tuint16_t slot;\n";
    sb += "\tuint8_t parameters_count;\n";
    sb += "\tuint8_t bitflags;\n";
    sb += "};\n";
    methodInfoHeader << sb;
}

} // namespace il2cpp_dumper
