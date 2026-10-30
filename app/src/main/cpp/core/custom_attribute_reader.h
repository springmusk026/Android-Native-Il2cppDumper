#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "blob_value.h"
#include "binary_stream.h"
#include "metadata_class.h"

namespace il2cpp_dumper {

// Forward declarations
class Il2CppExecutor;

struct CustomAttributeArgumentInfo {
    int index = 0;
    BlobValue value;
};

struct CustomAttributeVisitor {
    int ctorIndex = 0;
    std::vector<CustomAttributeArgumentInfo> arguments;
    std::vector<CustomAttributeArgumentInfo> fields;
    std::vector<CustomAttributeArgumentInfo> properties;
};

// Reader for the v29+ attribute data blob:
//   compressed count, int32 ctorIndex[count], then per attribute
//   argCount/fieldCount/propertyCount and their encoded values.
class CustomAttributeDataReader {
public:
    CustomAttributeDataReader(Il2CppExecutor* executor, const uint8_t* data, size_t size);

    uint32_t count() const { return count_; }
    CustomAttributeVisitor visitCustomAttributeData();
    std::string getStringCustomAttributeData();

private:
    Il2CppExecutor* executor;
    BinaryStream stream;
    uint32_t count_ = 0;
    uint64_t ctorBuffer = 0;
    uint64_t dataBuffer = 0;

    BlobValue readAttributeDataValue();
    std::pair<const Il2CppTypeDefinition*, int> readCustomAttributeNamedArgumentClassAndIndex(const Il2CppTypeDefinition& typeDef);
    std::string attributeDataToString(const BlobValue& blobValue);
};

} // namespace il2cpp_dumper
