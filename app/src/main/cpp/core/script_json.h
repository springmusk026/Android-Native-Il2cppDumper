#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace il2cpp_dumper {

struct ScriptMethod {
    uint64_t address = 0;
    std::string name;
    std::string signature;
    std::string typeSignature;
};

struct ScriptString {
    uint64_t address = 0;
    std::string value;
};

struct ScriptMetadata {
    uint64_t address = 0;
    std::string name;
    std::optional<std::string> signature;
};

struct ScriptMetadataMethod {
    uint64_t address = 0;
    std::string name;
    uint64_t methodAddress = 0;
};

struct ScriptJson {
    std::vector<ScriptMethod> scriptMethod;
    std::vector<ScriptString> scriptString;
    std::vector<ScriptMetadata> scriptMetadata;
    std::vector<ScriptMetadataMethod> scriptMetadataMethod;
    std::vector<uint64_t> addresses;
};

} // namespace il2cpp_dumper
