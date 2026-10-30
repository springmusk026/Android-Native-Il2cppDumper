#include "il2cpp_decompiler.h"
#include "il2cpp_constants.h"
#include "custom_attribute_reader.h"
#include "config.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

static std::string hex(uint64_t v) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(v));
    return buf;
}

Il2CppDecompiler::Il2CppDecompiler(Il2CppExecutor& executor)
    : executor(executor), metadata(executor.metadata), il2Cpp(executor.il2Cpp) {}

std::string Il2CppDecompiler::formatDefaultValue(int typeIndex, int dataIndex) {
    BlobValue value;
    uint32_t pointer = 0;
    if (!executor.tryGetDefaultValue(typeIndex, dataIndex, value, pointer)) {
        return " /*Metadata offset 0x" + hex(pointer) + "*/";
    }
    if (auto* str = std::get_if<std::string>(&value.value)) {
        return " = \"" + Il2CppExecutor::escapeString(*str) + "\"";
    }
    if (auto* c = std::get_if<char16_t>(&value.value)) {
        char buf[16];
        snprintf(buf, sizeof(buf), "'\\x%x'", static_cast<unsigned>(*c));
        return std::string(" = ") + buf;
    }
    return " = " + Il2CppExecutor::scalarToString(value);
}

std::string Il2CppDecompiler::formatPointer(uint64_t pointer) {
    return "RVA: 0x" + hex(il2Cpp.getRVA(pointer)) + " Offset: 0x" + hex(il2Cpp.mapVATR(pointer)) +
           " VA: 0x" + hex(pointer);
}

void Il2CppDecompiler::decompile(const Config& config, const std::string& outputDir) {
    LOGI("Dumping...");
    auto path = outputDir + "/dump.cs";
    std::ofstream writer(path, std::ios::binary);
    if (!writer.is_open()) {
        throw std::runtime_error("Failed to open output file: " + path);
    }

    for (size_t imageIndex = 0; imageIndex < metadata.imageDefs.size(); imageIndex++) {
        auto& imageDef = metadata.imageDefs[imageIndex];
        writer << "// Image " << imageIndex << ": "
               << metadata.getStringFromIndex(imageDef.nameIndex)
               << " - " << imageDef.typeStart << "\n";
    }

    for (auto& imageDef : metadata.imageDefs) {
        if (g_cancelled.load()) { writer << "// Dump cancelled\n"; break; }
        auto imageName = metadata.getStringFromIndex(imageDef.nameIndex);
        auto typeEnd = static_cast<int64_t>(imageDef.typeStart) + imageDef.typeCount;
        for (int typeDefIndex = imageDef.typeStart; typeDefIndex < typeEnd; typeDefIndex++) {
            if (g_cancelled.load()) break;
            // Build each type in a buffer so a failure mid-type can't leave a
            // half-written declaration behind.
            std::ostringstream out;
            try {
                dumpType(out, config, imageDef, imageName, typeDefIndex);
                writer << out.str();
            } catch (const std::exception& e) {
                LOGE("Error dumping type %d: %s", typeDefIndex, e.what());
                writer << out.str() << "/*" << e.what() << "*/\n}\n";
            }
        }
    }

    writer.close();
    LOGI("Dump complete: %s", path.c_str());
}

void Il2CppDecompiler::dumpType(std::ostream& writer, const Config& config, const Il2CppImageDefinition& imageDef,
                                const std::string& imageName, int typeDefIndex) {
    auto& typeDef = metadata.typeDefs.at(typeDefIndex);
    std::vector<std::string> extends;
    if (typeDef.parentIndex >= 0) {
        auto& parent = il2Cpp.types.at(typeDef.parentIndex);
        auto parentName = executor.getTypeName(parent, false, false);
        if (!typeDef.isValueType() && !typeDef.isEnum() && parentName != "object") {
            extends.push_back(parentName);
        }
    }
    for (int i = 0; i < typeDef.interfaces_count; i++) {
        auto& iface = il2Cpp.types.at(metadata.interfaceIndices.at(typeDef.interfacesStart + i));
        extends.push_back(executor.getTypeName(iface, false, false));
    }

    writer << "\n// Namespace: " << metadata.getStringFromIndex(typeDef.namespaceIndex) << "\n";
    if (config.dumpAttribute) {
        writer << getCustomAttribute(imageDef, typeDef.customAttributeIndex, typeDef.token);
        if (typeDef.flags & TYPE_ATTRIBUTE_SERIALIZABLE) writer << "[Serializable]\n";
    }

    switch (typeDef.flags & TYPE_ATTRIBUTE_VISIBILITY_MASK) {
        case TYPE_ATTRIBUTE_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_PUBLIC: writer << "public "; break;
        case TYPE_ATTRIBUTE_NOT_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
        case TYPE_ATTRIBUTE_NESTED_ASSEMBLY: writer << "internal "; break;
        case TYPE_ATTRIBUTE_NESTED_PRIVATE: writer << "private "; break;
        case TYPE_ATTRIBUTE_NESTED_FAMILY: writer << "protected "; break;
        case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM: writer << "protected internal "; break;
    }

    if ((typeDef.flags & TYPE_ATTRIBUTE_ABSTRACT) && (typeDef.flags & TYPE_ATTRIBUTE_SEALED))
        writer << "static ";
    else if (!(typeDef.flags & TYPE_ATTRIBUTE_INTERFACE) && (typeDef.flags & TYPE_ATTRIBUTE_ABSTRACT))
        writer << "abstract ";
    else if (!typeDef.isValueType() && !typeDef.isEnum() && (typeDef.flags & TYPE_ATTRIBUTE_SEALED))
        writer << "sealed ";

    if (typeDef.flags & TYPE_ATTRIBUTE_INTERFACE) writer << "interface ";
    else if (typeDef.isEnum()) writer << "enum ";
    else if (typeDef.isValueType()) writer << "struct ";
    else writer << "class ";

    writer << executor.getTypeDefName(typeDef, false, true);
    if (!extends.empty()) {
        writer << " : ";
        for (size_t i = 0; i < extends.size(); i++) {
            if (i > 0) writer << ", ";
            writer << extends[i];
        }
    }
    if (config.dumpTypeDefIndex)
        writer << " // TypeDefIndex: " << typeDefIndex << "\n{";
    else
        writer << "\n{";

    if (config.dumpField && typeDef.field_count > 0) {
        writer << "\n\t// Fields\n";
        auto fieldEnd = typeDef.fieldStart + typeDef.field_count;
        for (int i = typeDef.fieldStart; i < fieldEnd; i++) {
            auto& fieldDef = metadata.fieldDefs.at(i);
            auto& fieldType = il2Cpp.types.at(fieldDef.typeIndex);
            bool isStatic = false, isConst = false;

            if (config.dumpAttribute) {
                writer << getCustomAttribute(imageDef, fieldDef.customAttributeIndex, fieldDef.token, "\t");
            }
            writer << "\t";
            switch (fieldType.attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK) {
                case FIELD_ATTRIBUTE_PRIVATE: writer << "private "; break;
                case FIELD_ATTRIBUTE_PUBLIC: writer << "public "; break;
                case FIELD_ATTRIBUTE_FAMILY: writer << "protected "; break;
                case FIELD_ATTRIBUTE_ASSEMBLY:
                case FIELD_ATTRIBUTE_FAM_AND_ASSEM: writer << "internal "; break;
                case FIELD_ATTRIBUTE_FAM_OR_ASSEM: writer << "protected internal "; break;
            }
            if (fieldType.attrs & FIELD_ATTRIBUTE_LITERAL) {
                isConst = true;
                writer << "const ";
            } else {
                if (fieldType.attrs & FIELD_ATTRIBUTE_STATIC) {
                    isStatic = true;
                    writer << "static ";
                }
                if (fieldType.attrs & FIELD_ATTRIBUTE_INIT_ONLY) writer << "readonly ";
            }
            writer << executor.getTypeName(fieldType, false, false) << " "
                   << metadata.getStringFromIndex(fieldDef.nameIndex);

            Il2CppFieldDefaultValue fieldDefaultValue;
            if (metadata.getFieldDefaultValueFromIndex(i, fieldDefaultValue) && fieldDefaultValue.dataIndex != -1) {
                writer << formatDefaultValue(fieldDefaultValue.typeIndex, fieldDefaultValue.dataIndex);
            }
            if (config.dumpFieldOffset && !isConst) {
                auto offset = il2Cpp.getFieldOffsetFromIndex(typeDefIndex, i - typeDef.fieldStart, i,
                                                              typeDef.isValueType(), isStatic);
                writer << "; // 0x" << hex(static_cast<uint32_t>(offset)) << "\n";
            } else {
                writer << ";\n";
            }
        }
    }

    if (config.dumpProperty && typeDef.property_count > 0) {
        writer << "\n\t// Properties\n";
        auto propertyEnd = typeDef.propertyStart + typeDef.property_count;
        for (int i = typeDef.propertyStart; i < propertyEnd; i++) {
            auto& propertyDef = metadata.propertyDefs.at(i);
            if (config.dumpAttribute) {
                writer << getCustomAttribute(imageDef, propertyDef.customAttributeIndex, propertyDef.token, "\t");
            }
            writer << "\t";
            if (propertyDef.get >= 0) {
                auto& methodDef = metadata.methodDefs.at(typeDef.methodStart + propertyDef.get);
                writer << getModifiers(methodDef);
                auto& propertyType = il2Cpp.types.at(methodDef.returnType);
                writer << executor.getTypeName(propertyType, false, false) << " "
                       << metadata.getStringFromIndex(propertyDef.nameIndex) << " { ";
            } else if (propertyDef.set >= 0) {
                // Setter-only: the property type is the setter's (first) parameter.
                auto& methodDef = metadata.methodDefs.at(typeDef.methodStart + propertyDef.set);
                writer << getModifiers(methodDef);
                auto& parameterDef = metadata.parameterDefs.at(methodDef.parameterStart);
                auto& propertyType = il2Cpp.types.at(parameterDef.typeIndex);
                writer << executor.getTypeName(propertyType, false, false) << " "
                       << metadata.getStringFromIndex(propertyDef.nameIndex) << " { ";
            }
            if (propertyDef.get >= 0) writer << "get; ";
            if (propertyDef.set >= 0) writer << "set; ";
            writer << "}\n";
        }
    }

    if (config.dumpMethod && typeDef.method_count > 0) {
        writer << "\n\t// Methods\n";
        auto methodEnd = typeDef.methodStart + typeDef.method_count;
        for (int i = typeDef.methodStart; i < methodEnd; i++) {
            writer << "\n";
            auto& methodDef = metadata.methodDefs.at(i);
            bool isAbstract = (methodDef.flags & METHOD_ATTRIBUTE_ABSTRACT) != 0;
            if (config.dumpAttribute) {
                writer << getCustomAttribute(imageDef, methodDef.customAttributeIndex, methodDef.token, "\t");
            }
            if (config.dumpMethodOffset) {
                auto methodPointer = il2Cpp.getMethodPointer(imageName, methodDef);
                if (!isAbstract && methodPointer > 0) {
                    writer << "\t// " << formatPointer(methodPointer);
                } else {
                    writer << "\t// RVA: -1 Offset: -1";
                }
                if (methodDef.slot != 0xFFFF) {
                    writer << " Slot: " << methodDef.slot;
                }
                writer << "\n";
            }
            writer << "\t" << getModifiers(methodDef);
            auto& methodReturnType = il2Cpp.types.at(methodDef.returnType);
            auto methodName = metadata.getStringFromIndex(methodDef.nameIndex);
            if (methodDef.genericContainerIndex >= 0) {
                methodName += executor.getGenericContainerParams(metadata.genericContainers.at(methodDef.genericContainerIndex));
            }
            if (methodReturnType.byref == 1) writer << "ref ";
            writer << executor.getTypeName(methodReturnType, false, false) << " " << methodName << "(";

            for (int j = 0; j < methodDef.parameterCount; j++) {
                if (j > 0) writer << ", ";
                auto parameterIndex = methodDef.parameterStart + j;
                auto& parameterDef = metadata.parameterDefs.at(parameterIndex);
                auto& parameterType = il2Cpp.types.at(parameterDef.typeIndex);
                bool in = (parameterType.attrs & PARAM_ATTRIBUTE_IN) != 0;
                bool outAttr = (parameterType.attrs & PARAM_ATTRIBUTE_OUT) != 0;
                if (parameterType.byref == 1) {
                    if (outAttr && !in) writer << "out ";
                    else if (!outAttr && in) writer << "in ";
                    else writer << "ref ";
                } else {
                    if (in) writer << "[In] ";
                    if (outAttr) writer << "[Out] ";
                }
                writer << executor.getTypeName(parameterType, false, false) << " "
                       << metadata.getStringFromIndex(parameterDef.nameIndex);
                Il2CppParameterDefaultValue parameterDefault;
                if (metadata.getParameterDefaultValueFromIndex(parameterIndex, parameterDefault) &&
                    parameterDefault.dataIndex != -1) {
                    writer << formatDefaultValue(parameterDefault.typeIndex, parameterDefault.dataIndex);
                }
            }
            writer << (isAbstract ? ");\n" : ") { }\n");

            auto specsIt = il2Cpp.methodDefinitionMethodSpecs.find(i);
            if (specsIt != il2Cpp.methodDefinitionMethodSpecs.end()) {
                writeGenericInstMethods(writer, specsIt->second);
            }
        }
    }
    writer << "}\n";
}

void Il2CppDecompiler::writeGenericInstMethods(std::ostream& writer, const std::vector<Il2CppMethodSpec>& methodSpecs) {
    // Group by shared generic method pointer, keeping first-appearance order (LINQ GroupBy).
    std::vector<std::pair<uint64_t, std::vector<const Il2CppMethodSpec*>>> groups;
    for (auto& methodSpec : methodSpecs) {
        auto ptrIt = il2Cpp.methodSpecGenericMethodPointers.find(methodSpec);
        uint64_t pointer = ptrIt != il2Cpp.methodSpecGenericMethodPointers.end() ? ptrIt->second : 0;
        auto group = std::find_if(groups.begin(), groups.end(), [pointer](auto& g) { return g.first == pointer; });
        if (group == groups.end()) {
            groups.push_back({pointer, {}});
            group = groups.end() - 1;
        }
        group->second.push_back(&methodSpec);
    }

    writer << "\t/* GenericInstMethod :\n";
    for (auto& [genericMethodPointer, specs] : groups) {
        writer << "\t|\n";
        if (genericMethodPointer > 0) {
            writer << "\t|-" << formatPointer(genericMethodPointer) << "\n";
        } else {
            writer << "\t|-RVA: -1 Offset: -1\n";
        }
        for (auto* methodSpec : specs) {
            auto [typeName, methodName] = executor.getMethodSpecName(*methodSpec);
            writer << "\t|-" << typeName << "." << methodName << "\n";
        }
    }
    writer << "\t*/\n";
}

std::string Il2CppDecompiler::getCustomAttribute(const Il2CppImageDefinition& imageDef,
                                                 int customAttributeIndex, uint32_t token,
                                                 const std::string& padding) {
    if (il2Cpp.version < 21) return "";
    auto attributeIndex = metadata.getCustomAttributeIndex(imageDef, customAttributeIndex, token);
    if (attributeIndex < 0) return "";

    std::ostringstream ss;
    if (il2Cpp.version < 29) {
        auto methodPointer = executor.customAttributeGenerators.at(attributeIndex);
        auto& range = metadata.attributeTypeRanges.at(attributeIndex);
        for (int i = 0; i < range.count; i++) {
            auto typeIndex = metadata.attributeTypes.at(range.start + i);
            ss << padding << "[" << executor.getTypeName(il2Cpp.types.at(typeIndex), false, false)
               << "] // " << formatPointer(methodPointer) << "\n";
        }
    } else {
        auto& startRange = metadata.attributeDataRanges.at(attributeIndex);
        auto& endRange = metadata.attributeDataRanges.at(attributeIndex + 1);
        metadata.setPosition(metadata.header.attributeDataOffset + startRange.startOffset);
        auto buff = metadata.readBytes(static_cast<int>(endRange.startOffset - startRange.startOffset));
        CustomAttributeDataReader reader(&executor, buff.data(), buff.size());
        for (uint32_t i = 0; i < reader.count(); i++) {
            ss << padding << reader.getStringCustomAttributeData() << "\n";
        }
    }
    return ss.str();
}

std::string Il2CppDecompiler::getModifiers(const Il2CppMethodDefinition& methodDef) {
    std::string str;
    switch (methodDef.flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK) {
        case METHOD_ATTRIBUTE_PRIVATE: str += "private "; break;
        case METHOD_ATTRIBUTE_PUBLIC: str += "public "; break;
        case METHOD_ATTRIBUTE_FAMILY: str += "protected "; break;
        case METHOD_ATTRIBUTE_ASSEM:
        case METHOD_ATTRIBUTE_FAM_AND_ASSEM: str += "internal "; break;
        case METHOD_ATTRIBUTE_FAM_OR_ASSEM: str += "protected internal "; break;
    }
    if (methodDef.flags & METHOD_ATTRIBUTE_STATIC) str += "static ";
    if (methodDef.flags & METHOD_ATTRIBUTE_ABSTRACT) {
        str += "abstract ";
        if ((methodDef.flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            str += "override ";
    } else if (methodDef.flags & METHOD_ATTRIBUTE_FINAL) {
        if ((methodDef.flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            str += "sealed override ";
    } else if (methodDef.flags & METHOD_ATTRIBUTE_VIRTUAL) {
        if ((methodDef.flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_NEW_SLOT)
            str += "virtual ";
        else
            str += "override ";
    }
    if (methodDef.flags & METHOD_ATTRIBUTE_PINVOKE_IMPL) str += "extern ";
    return str;
}

} // namespace il2cpp_dumper
