#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace il2cpp_dumper {

enum class Il2CppTypeEnum : uint32_t {
    IL2CPP_TYPE_END = 0x00,
    IL2CPP_TYPE_VOID = 0x01,
    IL2CPP_TYPE_BOOLEAN = 0x02,
    IL2CPP_TYPE_CHAR = 0x03,
    IL2CPP_TYPE_I1 = 0x04,
    IL2CPP_TYPE_U1 = 0x05,
    IL2CPP_TYPE_I2 = 0x06,
    IL2CPP_TYPE_U2 = 0x07,
    IL2CPP_TYPE_I4 = 0x08,
    IL2CPP_TYPE_U4 = 0x09,
    IL2CPP_TYPE_I8 = 0x0a,
    IL2CPP_TYPE_U8 = 0x0b,
    IL2CPP_TYPE_R4 = 0x0c,
    IL2CPP_TYPE_R8 = 0x0d,
    IL2CPP_TYPE_STRING = 0x0e,
    IL2CPP_TYPE_PTR = 0x0f,
    IL2CPP_TYPE_BYREF = 0x10,
    IL2CPP_TYPE_VALUETYPE = 0x11,
    IL2CPP_TYPE_CLASS = 0x12,
    IL2CPP_TYPE_VAR = 0x13,
    IL2CPP_TYPE_ARRAY = 0x14,
    IL2CPP_TYPE_GENERICINST = 0x15,
    IL2CPP_TYPE_TYPEDBYREF = 0x16,
    IL2CPP_TYPE_I = 0x18,
    IL2CPP_TYPE_U = 0x19,
    IL2CPP_TYPE_FNPTR = 0x1b,
    IL2CPP_TYPE_OBJECT = 0x1c,
    IL2CPP_TYPE_SZARRAY = 0x1d,
    IL2CPP_TYPE_MVAR = 0x1e,
    IL2CPP_TYPE_CMOD_REQD = 0x1f,
    IL2CPP_TYPE_CMOD_OPT = 0x20,
    IL2CPP_TYPE_INTERNAL = 0x21,
    IL2CPP_TYPE_MODIFIER = 0x40,
    IL2CPP_TYPE_SENTINEL = 0x41,
    IL2CPP_TYPE_PINNED = 0x45,
    IL2CPP_TYPE_ENUM = 0x55,
    IL2CPP_TYPE_IL2CPP_TYPE_INDEX = 0xff
};

enum class Il2CppRGCTXDataType {
    IL2CPP_RGCTX_DATA_INVALID,
    IL2CPP_RGCTX_DATA_TYPE,
    IL2CPP_RGCTX_DATA_CLASS,
    IL2CPP_RGCTX_DATA_METHOD,
    IL2CPP_RGCTX_DATA_ARRAY,
    IL2CPP_RGCTX_DATA_CONSTRAINED,
};

struct BlobValue {
    Il2CppTypeEnum il2cppTypeEnum = Il2CppTypeEnum::IL2CPP_TYPE_END;
    // Scalar payload. std::nullptr_t means a null string/array/type, monostate
    // means "no scalar" (e.g. a non-null SZARRAY whose elements are in `array`).
    // IL2CPP_TYPE_IL2CPP_TYPE_INDEX stores the type index as int32_t.
    using ValueVariant = std::variant<
        std::monostate,
        bool,
        int8_t, uint8_t,
        char16_t,
        int16_t, uint16_t,
        int32_t, uint32_t,
        int64_t, uint64_t,
        float, double,
        std::string,
        std::nullptr_t
    >;
    ValueVariant value{};
    std::vector<BlobValue> array;   // IL2CPP_TYPE_SZARRAY elements
    int64_t enumTypeIndex = -1;     // index into il2Cpp.types when the value is an enum

    bool isNull() const { return std::holds_alternative<std::nullptr_t>(value); }
};

} // namespace il2cpp_dumper
