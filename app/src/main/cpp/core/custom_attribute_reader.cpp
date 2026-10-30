#include "custom_attribute_reader.h"
#include "il2cpp_executor.h"

namespace il2cpp_dumper {

CustomAttributeDataReader::CustomAttributeDataReader(Il2CppExecutor* executor,
                                                       const uint8_t* data, size_t size)
    : executor(executor), stream(data, size) {
    if (size == 0) return;
    count_ = stream.readCompressedUInt32();
    ctorBuffer = stream.getPosition();
    dataBuffer = ctorBuffer + static_cast<uint64_t>(count_) * 4;
}

BlobValue CustomAttributeDataReader::readAttributeDataValue() {
    int64_t enumTypeIndex = -1;
    auto type = executor->readEncodedTypeEnum(stream, enumTypeIndex);
    BlobValue blobValue;
    executor->getConstantValueFromBlob(type, stream, blobValue);
    if (enumTypeIndex != -1) blobValue.enumTypeIndex = enumTypeIndex;
    return blobValue;
}

std::pair<const Il2CppTypeDefinition*, int>
CustomAttributeDataReader::readCustomAttributeNamedArgumentClassAndIndex(const Il2CppTypeDefinition& typeDef) {
    auto memberIndex = stream.readCompressedInt32();
    if (memberIndex >= 0) return {&typeDef, memberIndex};
    memberIndex = -(memberIndex + 1);
    auto typeIndex = stream.readCompressedUInt32();
    return {&executor->metadata.typeDefs.at(typeIndex), memberIndex};
}

std::string CustomAttributeDataReader::attributeDataToString(const BlobValue& blobValue) {
    //TODO enum (as upstream)
    if (blobValue.isNull()) return "null";
    switch (blobValue.il2cppTypeEnum) {
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING:
            return "\"" + Il2CppExecutor::scalarToString(blobValue) + "\"";
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY: {
            std::string list;
            for (size_t i = 0; i < blobValue.array.size(); i++) {
                if (i > 0) list += ", ";
                list += attributeDataToString(blobValue.array[i]);
            }
            return "new[] { " + list + " }";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_IL2CPP_TYPE_INDEX: {
            auto typeIndex = std::get<int32_t>(blobValue.value);
            return "typeof(" + executor->getTypeName(executor->il2Cpp.types.at(typeIndex), false, false) + ")";
        }
        default:
            return Il2CppExecutor::scalarToString(blobValue);
    }
}

CustomAttributeVisitor CustomAttributeDataReader::visitCustomAttributeData() {
    auto& metadata = executor->metadata;
    CustomAttributeVisitor visitor;

    stream.setPosition(ctorBuffer);
    visitor.ctorIndex = stream.readInt32();
    auto& methodDef = metadata.methodDefs.at(visitor.ctorIndex);
    auto& typeDef = metadata.typeDefs.at(methodDef.declaringType);
    ctorBuffer = stream.getPosition();

    stream.setPosition(dataBuffer);
    auto argumentCount = stream.readCompressedUInt32();
    auto fieldCount = stream.readCompressedUInt32();
    auto propertyCount = stream.readCompressedUInt32();

    for (uint32_t i = 0; i < argumentCount; i++) {
        CustomAttributeArgumentInfo argument;
        argument.value = readAttributeDataValue();
        argument.index = static_cast<int>(i);
        visitor.arguments.push_back(std::move(argument));
    }
    for (uint32_t i = 0; i < fieldCount; i++) {
        CustomAttributeArgumentInfo field;
        field.value = readAttributeDataValue();
        auto [declaring, fieldIndex] = readCustomAttributeNamedArgumentClassAndIndex(typeDef);
        field.index = declaring->fieldStart + fieldIndex;
        visitor.fields.push_back(std::move(field));
    }
    for (uint32_t i = 0; i < propertyCount; i++) {
        CustomAttributeArgumentInfo property;
        property.value = readAttributeDataValue();
        auto [declaring, propertyIndex] = readCustomAttributeNamedArgumentClassAndIndex(typeDef);
        property.index = declaring->propertyStart + propertyIndex;
        visitor.properties.push_back(std::move(property));
    }

    dataBuffer = stream.getPosition();
    return visitor;
}

std::string CustomAttributeDataReader::getStringCustomAttributeData() {
    auto& metadata = executor->metadata;

    stream.setPosition(ctorBuffer);
    auto ctorIndex = stream.readInt32();
    auto& methodDef = metadata.methodDefs.at(ctorIndex);
    auto& typeDef = metadata.typeDefs.at(methodDef.declaringType);
    ctorBuffer = stream.getPosition();

    stream.setPosition(dataBuffer);
    auto argumentCount = stream.readCompressedUInt32();
    auto fieldCount = stream.readCompressedUInt32();
    auto propertyCount = stream.readCompressedUInt32();

    std::vector<std::string> argList;
    for (uint32_t i = 0; i < argumentCount; i++) {
        argList.push_back(attributeDataToString(readAttributeDataValue()));
    }
    for (uint32_t i = 0; i < fieldCount; i++) {
        auto str = attributeDataToString(readAttributeDataValue());
        auto [declaring, fieldIndex] = readCustomAttributeNamedArgumentClassAndIndex(typeDef);
        auto& fieldDef = metadata.fieldDefs.at(declaring->fieldStart + fieldIndex);
        argList.push_back(metadata.getStringFromIndex(fieldDef.nameIndex) + " = " + str);
    }
    for (uint32_t i = 0; i < propertyCount; i++) {
        auto str = attributeDataToString(readAttributeDataValue());
        auto [declaring, propertyIndex] = readCustomAttributeNamedArgumentClassAndIndex(typeDef);
        auto& propertyDef = metadata.propertyDefs.at(declaring->propertyStart + propertyIndex);
        argList.push_back(metadata.getStringFromIndex(propertyDef.nameIndex) + " = " + str);
    }
    dataBuffer = stream.getPosition();

    // Upstream uses string.Replace("Attribute", ""), i.e. every occurrence.
    auto typeName = metadata.getStringFromIndex(typeDef.nameIndex);
    for (size_t pos; (pos = typeName.find("Attribute")) != std::string::npos;) {
        typeName.erase(pos, 9);
    }
    if (argList.empty()) return "[" + typeName + "]";
    std::string args;
    for (size_t i = 0; i < argList.size(); i++) {
        if (i > 0) args += ", ";
        args += argList[i];
    }
    return "[" + typeName + "(" + args + ")]";
}

} // namespace il2cpp_dumper
