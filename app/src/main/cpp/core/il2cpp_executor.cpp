#include "il2cpp_executor.h"
#include "section_helper.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

const std::unordered_map<int, std::string> Il2CppExecutor::typeStringMap = {
    {1, "void"}, {2, "bool"}, {3, "char"}, {4, "sbyte"}, {5, "byte"},
    {6, "short"}, {7, "ushort"}, {8, "int"}, {9, "uint"},
    {10, "long"}, {11, "ulong"}, {12, "float"}, {13, "double"},
    {14, "string"}, {22, "TypedReference"}, {24, "IntPtr"}, {25, "UIntPtr"}, {28, "object"}
};

static std::string joinNames(const std::vector<std::string>& names) {
    std::string result = "<";
    for (size_t i = 0; i < names.size(); i++) {
        if (i > 0) result += ", ";
        result += names[i];
    }
    result += ">";
    return result;
}

Il2CppExecutor::Il2CppExecutor(Metadata& metadata, Il2CppEngine& il2Cpp)
    : metadata(metadata), il2Cpp(il2Cpp) {
    if (il2Cpp.version >= 27 && il2Cpp.version < 29) {
        uint64_t total = 0;
        for (auto& img : metadata.imageDefs) total += img.customAttributeCount;
        customAttributeGenerators.assign(total, 0);
        for (auto& imageDef : metadata.imageDefs) {
            if (imageDef.customAttributeCount == 0) continue;
            auto imageName = metadata.getStringFromIndex(imageDef.nameIndex);
            auto it = il2Cpp.codeGenModules.find(imageName);
            if (it == il2Cpp.codeGenModules.end()) continue;
            auto start = static_cast<uint64_t>(std::max(0, imageDef.customAttributeStart));
            if (start + imageDef.customAttributeCount > total) continue;
            try {
                auto pointers = il2Cpp.readPointerArray(
                    il2Cpp.mapVATR(it->second.customAttributeCacheGenerator),
                    imageDef.customAttributeCount);
                std::copy(pointers.begin(), pointers.end(), customAttributeGenerators.begin() + start);
            } catch (const std::exception& e) {
                LOGE("customAttributeCacheGenerator for %s: %s", imageName.c_str(), e.what());
            }
        }
    } else if (il2Cpp.version < 27) {
        customAttributeGenerators = il2Cpp.customAttributeGenerators;
    }
}

const Il2CppType& Il2CppExecutor::typeAt(int64_t index) const {
    if (index < 0 || static_cast<size_t>(index) >= il2Cpp.types.size())
        throw std::out_of_range("type index out of range: " + std::to_string(index));
    return il2Cpp.types[index];
}

static const Il2CppType& resolveType(Il2CppEngine& il2Cpp, uint64_t pointer) {
    auto* type = il2Cpp.getIl2CppType(pointer);
    if (!type) {
        char msg[64];
        snprintf(msg, sizeof(msg), "unknown Il2CppType pointer 0x%llx", (unsigned long long)pointer);
        throw std::out_of_range(msg);
    }
    return *type;
}

std::string Il2CppExecutor::getTypeName(const Il2CppType& il2CppType, bool addNamespace, bool isNested) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY: {
            auto arrayType = il2Cpp.readClass<Il2CppArrayType>(il2Cpp.mapVATR(il2CppType.array()));
            auto& elementType = resolveType(il2Cpp, arrayType.etype);
            std::string suffix = "[";
            for (int i = 1; i < arrayType.rank; i++) suffix += ",";
            suffix += "]";
            return getTypeName(elementType, addNamespace, false) + suffix;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY:
            return getTypeName(resolveType(il2Cpp, il2CppType.typeRef()), addNamespace, false) + "[]";
        case Il2CppTypeEnum::IL2CPP_TYPE_PTR:
            return getTypeName(resolveType(il2Cpp, il2CppType.typeRef()), addNamespace, false) + "*";
        case Il2CppTypeEnum::IL2CPP_TYPE_VAR:
        case Il2CppTypeEnum::IL2CPP_TYPE_MVAR: {
            auto& param = getGenericParameterFromIl2CppType(il2CppType);
            return metadata.getStringFromIndex(param.nameIndex);
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS:
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST: {
            std::string str;
            const Il2CppTypeDefinition* typeDef;
            Il2CppGenericClass genericClass{};
            bool hasGenericClass = false;

            if (il2CppType.type == Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST) {
                genericClass = il2Cpp.readClass<Il2CppGenericClass>(il2Cpp.mapVATR(il2CppType.generic_class()));
                hasGenericClass = true;
                typeDef = getGenericClassTypeDefinition(genericClass);
                if (!typeDef) throw std::out_of_range("unresolved generic class");
            } else {
                typeDef = &getTypeDefinitionFromIl2CppType(il2CppType);
            }

            if (typeDef->declaringTypeIndex != -1) {
                str += getTypeName(typeAt(typeDef->declaringTypeIndex), addNamespace, true) + ".";
            } else if (addNamespace) {
                auto ns = metadata.getStringFromIndex(typeDef->namespaceIndex);
                if (!ns.empty()) str += ns + ".";
            }

            auto typeName = metadata.getStringFromIndex(typeDef->nameIndex);
            auto tickPos = typeName.find('`');
            str += tickPos != std::string::npos ? typeName.substr(0, tickPos) : typeName;

            if (isNested) return str;

            if (hasGenericClass) {
                auto genericInst = il2Cpp.readClass<Il2CppGenericInst>(il2Cpp.mapVATR(genericClass.context_class_inst));
                str += getGenericInstParams(genericInst);
            } else if (typeDef->genericContainerIndex >= 0) {
                str += getGenericContainerParams(metadata.genericContainers.at(typeDef->genericContainerIndex));
            }
            return str;
        }
        default: {
            auto it = typeStringMap.find(static_cast<int>(il2CppType.type));
            if (it != typeStringMap.end()) return it->second;
            throw std::out_of_range("unsupported Il2CppTypeEnum " + std::to_string(static_cast<int>(il2CppType.type)));
        }
    }
}

std::string Il2CppExecutor::getTypeDefName(const Il2CppTypeDefinition& typeDef, bool addNamespace, bool genericParameter) {
    std::string prefix;
    if (typeDef.declaringTypeIndex != -1) {
        prefix = getTypeName(typeAt(typeDef.declaringTypeIndex), addNamespace, true) + ".";
    } else if (addNamespace) {
        auto ns = metadata.getStringFromIndex(typeDef.namespaceIndex);
        if (!ns.empty()) prefix = ns + ".";
    }
    auto typeName = metadata.getStringFromIndex(typeDef.nameIndex);
    if (typeDef.genericContainerIndex >= 0) {
        auto tickPos = typeName.find('`');
        if (tickPos != std::string::npos) typeName = typeName.substr(0, tickPos);
        if (genericParameter) {
            typeName += getGenericContainerParams(metadata.genericContainers.at(typeDef.genericContainerIndex));
        }
    }
    return prefix + typeName;
}

std::string Il2CppExecutor::getGenericInstParams(const Il2CppGenericInst& genericInst) {
    std::vector<std::string> names;
    auto pointers = il2Cpp.readPointerArray(il2Cpp.mapVATR(genericInst.type_argv), genericInst.type_argc);
    for (auto ptr : pointers) {
        names.push_back(getTypeName(resolveType(il2Cpp, ptr), false, false));
    }
    return joinNames(names);
}

std::string Il2CppExecutor::getGenericContainerParams(const Il2CppGenericContainer& container) {
    std::vector<std::string> names;
    for (int i = 0; i < container.type_argc; i++) {
        auto& param = metadata.genericParameters.at(container.genericParameterStart + i);
        names.push_back(metadata.getStringFromIndex(param.nameIndex));
    }
    return joinNames(names);
}

std::pair<std::string, std::string> Il2CppExecutor::getMethodSpecName(const Il2CppMethodSpec& methodSpec, bool addNamespace) {
    auto& methodDef = metadata.methodDefs.at(methodSpec.methodDefinitionIndex);
    auto& typeDef = metadata.typeDefs.at(methodDef.declaringType);
    auto typeName = getTypeDefName(typeDef, addNamespace, false);
    if (methodSpec.classIndexIndex != -1) {
        typeName += getGenericInstParams(il2Cpp.genericInsts.at(methodSpec.classIndexIndex));
    }
    auto methodName = metadata.getStringFromIndex(methodDef.nameIndex);
    if (methodSpec.methodIndexIndex != -1) {
        methodName += getGenericInstParams(il2Cpp.genericInsts.at(methodSpec.methodIndexIndex));
    }
    return {typeName, methodName};
}

Il2CppGenericContext Il2CppExecutor::getMethodSpecGenericContext(const Il2CppMethodSpec& methodSpec) {
    Il2CppGenericContext context{};
    if (methodSpec.classIndexIndex != -1)
        context.class_inst = il2Cpp.genericInstPointers.at(methodSpec.classIndexIndex);
    if (methodSpec.methodIndexIndex != -1)
        context.method_inst = il2Cpp.genericInstPointers.at(methodSpec.methodIndexIndex);
    return context;
}

// Pre-24.2 RGCTX tables live in metadata; slices are cached per (start,count).
static const std::vector<Il2CppRGCTXDefinition>* metadataRGCTX(
        Metadata& metadata, int32_t start, int32_t count,
        std::unordered_map<uint64_t, std::vector<Il2CppRGCTXDefinition>>& cache) {
    if (count <= 0 || start < 0 || static_cast<size_t>(start) + count > metadata.rgctxEntries.size())
        return nullptr;
    auto key = (static_cast<uint64_t>(start) << 32) | static_cast<uint32_t>(count);
    auto it = cache.find(key);
    if (it == cache.end()) {
        it = cache.emplace(key, std::vector<Il2CppRGCTXDefinition>(
            metadata.rgctxEntries.begin() + start, metadata.rgctxEntries.begin() + start + count)).first;
    }
    return &it->second;
}

static const std::vector<Il2CppRGCTXDefinition>* moduleRGCTX(Il2CppEngine& il2Cpp, const std::string& imageName, uint32_t token) {
    auto module = il2Cpp.rgctxsDictionary.find(imageName);
    if (module == il2Cpp.rgctxsDictionary.end()) return nullptr;
    auto it = module->second.find(token);
    return it == module->second.end() ? nullptr : &it->second;
}

const std::vector<Il2CppRGCTXDefinition>* Il2CppExecutor::getRGCTXDefinition(const std::string& imageName, const Il2CppTypeDefinition& typeDef) {
    if (il2Cpp.version >= 24.2) return moduleRGCTX(il2Cpp, imageName, typeDef.token);
    return metadataRGCTX(metadata, typeDef.rgctxStartIndex, typeDef.rgctxCount, rgctxCache);
}

const std::vector<Il2CppRGCTXDefinition>* Il2CppExecutor::getRGCTXDefinition(const std::string& imageName, const Il2CppMethodDefinition& methodDef) {
    if (il2Cpp.version >= 24.2) return moduleRGCTX(il2Cpp, imageName, methodDef.token);
    return metadataRGCTX(metadata, methodDef.rgctxStartIndex, methodDef.rgctxCount, rgctxCache);
}

const Il2CppTypeDefinition* Il2CppExecutor::getGenericClassTypeDefinition(const Il2CppGenericClass& genericClass) {
    if (il2Cpp.version >= 27) {
        auto* il2CppType = il2Cpp.getIl2CppType(genericClass.type);
        if (!il2CppType) return nullptr;
        return &getTypeDefinitionFromIl2CppType(*il2CppType);
    }
    auto index = genericClass.typeDefinitionIndex;
    if (index == 4294967295LL || index < 0 || static_cast<size_t>(index) >= metadata.typeDefs.size())
        return nullptr;
    return &metadata.typeDefs[index];
}

const Il2CppTypeDefinition& Il2CppExecutor::getTypeDefinitionFromIl2CppType(const Il2CppType& il2CppType) {
    if (il2Cpp.version >= 27 && il2Cpp.isDumped) {
        auto offset = il2CppType.typeHandle() - metadata.imageBase - metadata.header.typeDefinitionsOffset;
        auto index = offset / static_cast<uint64_t>(metadata.sizeOfStruct<Il2CppTypeDefinition>());
        return metadata.typeDefs.at(index);
    }
    return metadata.typeDefs.at(il2CppType.klassIndex());
}

const Il2CppGenericParameter& Il2CppExecutor::getGenericParameterFromIl2CppType(const Il2CppType& il2CppType) {
    if (il2Cpp.version >= 27 && il2Cpp.isDumped) {
        auto offset = il2CppType.genericParameterHandle() - metadata.imageBase - metadata.header.genericParametersOffset;
        auto index = offset / static_cast<uint64_t>(metadata.sizeOfStruct<Il2CppGenericParameter>());
        return metadata.genericParameters.at(index);
    }
    return metadata.genericParameters.at(il2CppType.genericParameterIndex());
}

SectionHelper* Il2CppExecutor::getSectionHelper() {
    int methodCount = 0;
    for (auto& md : metadata.methodDefs) {
        if (md.methodIndex >= 0) methodCount++;
    }
    return il2Cpp.getSectionHelper(methodCount, metadata.typeDefs.size(), metadata.imageDefs.size());
}

bool Il2CppExecutor::tryGetDefaultValue(int typeIndex, int dataIndex, BlobValue& value, uint32_t& pointer) {
    pointer = metadata.getDefaultValueFromIndex(dataIndex);
    auto& defaultValueType = typeAt(typeIndex);
    metadata.setPosition(pointer);
    return getConstantValueFromBlob(defaultValueType.type, metadata, value);
}

bool Il2CppExecutor::getConstantValueFromBlob(Il2CppTypeEnum type, BinaryStream& reader, BlobValue& value) {
    value = BlobValue{};
    value.il2cppTypeEnum = type;
    switch (type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN:
            value.value = reader.readBoolean();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_U1:
            value.value = reader.readByte();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_I1:
            value.value = reader.readSByte();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_CHAR:
            value.value = static_cast<char16_t>(reader.readUInt16());
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_U2:
            value.value = reader.readUInt16();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_I2:
            value.value = reader.readInt16();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_U4:
            if (il2Cpp.version >= 29) value.value = reader.readCompressedUInt32();
            else value.value = reader.readUInt32();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_I4:
            if (il2Cpp.version >= 29) value.value = reader.readCompressedInt32();
            else value.value = reader.readInt32();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_U8:
            value.value = reader.readUInt64();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_I8:
            value.value = reader.readInt64();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_R4:
            value.value = reader.readFloat();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_R8:
            value.value = reader.readDouble();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING: {
            int32_t length;
            if (il2Cpp.version >= 29) {
                length = reader.readCompressedInt32();
                if (length == -1) { value.value = nullptr; return true; }
            } else {
                length = reader.readInt32();
            }
            // The blob stores UTF-8 bytes; the length is a byte count.
            auto bytes = reader.readBytes(length);
            value.value = std::string(bytes.begin(), bytes.end());
            return true;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY: {
            auto arrayLen = reader.readCompressedInt32();
            if (arrayLen == -1) { value.value = nullptr; return true; }
            if (arrayLen < 0 || static_cast<uint64_t>(arrayLen) > reader.getLength())
                throw std::out_of_range("invalid blob array length");
            int64_t enumTypeIndex = -1;
            auto arrayElementType = readEncodedTypeEnum(reader, enumTypeIndex);
            auto arrayElementsAreDifferent = reader.readByte();
            value.array.reserve(arrayLen);
            for (int32_t i = 0; i < arrayLen; i++) {
                auto elementType = arrayElementType;
                if (arrayElementsAreDifferent == 1) {
                    elementType = readEncodedTypeEnum(reader, enumTypeIndex);
                }
                BlobValue data;
                getConstantValueFromBlob(elementType, reader, data);
                data.il2cppTypeEnum = elementType;
                data.enumTypeIndex = enumTypeIndex;
                value.array.push_back(std::move(data));
            }
            return true;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_IL2CPP_TYPE_INDEX: {
            auto typeIndex = reader.readCompressedInt32();
            if (typeIndex == -1) value.value = nullptr;
            else value.value = typeIndex;
            return true;
        }
        default:
            return false;
    }
}

Il2CppTypeEnum Il2CppExecutor::readEncodedTypeEnum(BinaryStream& reader, int64_t& enumTypeIndex) {
    enumTypeIndex = -1;
    auto type = static_cast<Il2CppTypeEnum>(reader.readByte());
    if (type == Il2CppTypeEnum::IL2CPP_TYPE_ENUM) {
        enumTypeIndex = reader.readCompressedInt32();
        auto& typeDef = getTypeDefinitionFromIl2CppType(typeAt(enumTypeIndex));
        type = typeAt(typeDef.elementTypeIndex).type;
    }
    return type;
}

// Shortest representation that round-trips, in .NET Core style ("1E+20", "NaN", "∞").
template<typename T>
static std::string formatFloating(T v) {
    if (std::isnan(v)) return "NaN";
    if (std::isinf(v)) return v < 0 ? "-\xE2\x88\x9E" : "\xE2\x88\x9E";
    char buf[64];
    const int maxDigits = sizeof(T) == 4 ? 9 : 17;
    for (int precision = 1; precision <= maxDigits; precision++) {
        snprintf(buf, sizeof(buf), "%.*g", precision, static_cast<double>(v));
        T parsed = sizeof(T) == 4 ? static_cast<T>(strtof(buf, nullptr)) : static_cast<T>(strtod(buf, nullptr));
        if (parsed == v) break;
    }
    std::string s = buf;
    auto e = s.find('e');
    if (e != std::string::npos) s[e] = 'E';
    return s;
}

std::string Il2CppExecutor::scalarToString(const BlobValue& value) {
    return std::visit([](auto&& v) -> std::string {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, std::monostate> || std::is_same_v<V, std::nullptr_t>) return "null";
        else if constexpr (std::is_same_v<V, bool>) return v ? "True" : "False";
        else if constexpr (std::is_same_v<V, std::string>) return v;
        else if constexpr (std::is_same_v<V, char16_t>) return std::to_string(static_cast<int>(v));
        else if constexpr (std::is_same_v<V, float> || std::is_same_v<V, double>) return formatFloating(v);
        else if constexpr (std::is_same_v<V, int8_t>) return std::to_string(static_cast<int>(v));
        else if constexpr (std::is_same_v<V, uint8_t>) return std::to_string(static_cast<unsigned>(v));
        else return std::to_string(v);
    }, value.value);
}

std::string Il2CppExecutor::escapeString(const std::string& s) {
    std::string re;
    re.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        auto c = static_cast<unsigned char>(s[i]);
        switch (c) {
            case '\'': re += "\\'"; break;
            case '"': re += "\\\""; break;
            case '\\': re += "\\\\"; break;
            case '\0': re += "\\0"; break;
            case '\a': re += "\\a"; break;
            case '\b': re += "\\b"; break;
            case '\f': re += "\\f"; break;
            case '\n': re += "\\n"; break;
            case '\r': re += "\\r"; break;
            case '\t': re += "\\t"; break;
            case '\v': re += "\\v"; break;
            default:
                // U+0085, U+2028, U+2029 in UTF-8
                if (c == 0xC2 && i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x85) {
                    re += "\\u0085"; i += 1;
                } else if (c == 0xE2 && i + 2 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x80 &&
                           (static_cast<unsigned char>(s[i + 2]) == 0xA8 || static_cast<unsigned char>(s[i + 2]) == 0xA9)) {
                    re += static_cast<unsigned char>(s[i + 2]) == 0xA8 ? "\\u2028" : "\\u2029"; i += 2;
                } else {
                    re += static_cast<char>(c);
                }
        }
    }
    return re;
}

} // namespace il2cpp_dumper
