#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "blob_value.h"

namespace il2cpp_dumper {

struct StructFieldInfo {
    std::string fieldTypeName;
    std::string fieldName;
    bool isValueType = false;
    bool isCustomType = false;
};

struct StructVTableMethodInfo {
    std::string methodName;
};

struct StructRGCTXInfo {
    Il2CppRGCTXDataType type{};
    std::string typeName;
    std::string className;
    std::string methodName;
};

struct StructInfo {
    std::string typeName;
    bool isValueType = false;
    std::string parent;
    std::vector<StructFieldInfo> fields;
    std::vector<StructFieldInfo> staticFields;
    std::vector<std::optional<StructVTableMethodInfo>> vTableMethod;
    std::vector<StructRGCTXInfo> rgctxs;
};

} // namespace il2cpp_dumper
