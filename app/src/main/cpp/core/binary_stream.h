#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace il2cpp_dumper {

class BinaryStream {
public:
    double version = 0;
    bool is32Bit = false;
    uint64_t imageBase = 0;

    BinaryStream(const uint8_t* data, size_t size);
    BinaryStream(std::vector<uint8_t>&& data);
    virtual ~BinaryStream() = default;

    // Non-copyable, movable
    BinaryStream(const BinaryStream&) = delete;
    BinaryStream& operator=(const BinaryStream&) = delete;
    BinaryStream(BinaryStream&&) = default;
    BinaryStream& operator=(BinaryStream&&) = default;

    // Primitive reads
    bool readBoolean();
    uint8_t readByte();
    std::vector<uint8_t> readBytes(int count);
    int8_t readSByte();
    int16_t readInt16();
    uint16_t readUInt16();
    int32_t readInt32();
    uint32_t readUInt32();
    int64_t readInt64();
    uint64_t readUInt64();
    float readFloat();
    double readDouble();

    // Compressed integer reading
    uint32_t readCompressedUInt32();
    int32_t readCompressedInt32();
    uint32_t readULeb128();

    // Pointer-sized reads (depends on is32Bit)
    int64_t readIntPtr();
    uint64_t readUIntPtr();

    // Write operations (for relocation processing)
    void writeUInt32(uint32_t value);
    void writeUInt64(uint64_t value);

    // Position
    uint64_t getPosition() const { return position; }
    void setPosition(uint64_t pos);

    // Length
    uint64_t getLength() const { return dataSize; }

    // Pointer size
    uint64_t getPointerSize() const { return is32Bit ? 4 : 8; }

    // String reading
    std::string readStringToNull(uint64_t addr);
    std::string readString(int numChars);

    // Raw data access
    const uint8_t* getData() const { return data; }
    uint8_t* getDataMutable() { return data; }

    // Template class reading
    template<typename T>
    T readClass() {
        T t{};
        t.read(*this, version);
        return t;
    }

    template<typename T>
    T readClass(uint64_t addr) {
        setPosition(addr);
        return readClass<T>();
    }

    template<typename T>
    std::vector<T> readClassArray(int64_t count) {
        checkCount(count, 1);
        std::vector<T> result(count);
        for (int64_t i = 0; i < count; i++) {
            result[i].read(*this, version);
        }
        return result;
    }

    template<typename T>
    std::vector<T> readClassArray(uint64_t addr, int64_t count) {
        setPosition(addr);
        return readClassArray<T>(count);
    }

    // For types without a read() method (primitives, POD)
    template<typename T>
    T readPrimitive();

    // Read an array of pointer-sized values, respecting is32Bit.
    // Always returns uint64_t vector (widened from uint32_t on 32-bit).
    std::vector<uint64_t> readPointerArray(int64_t count) {
        checkCount(count, getPointerSize());
        std::vector<uint64_t> result(count);
        if (is32Bit) {
            for (int64_t i = 0; i < count; i++) result[i] = readUInt32();
        } else {
            for (int64_t i = 0; i < count; i++) result[i] = readUInt64();
        }
        return result;
    }

    std::vector<uint64_t> readPointerArray(uint64_t addr, int64_t count) {
        setPosition(addr);
        return readPointerArray(count);
    }

    template<typename T>
    std::vector<T> readPrimitiveArray(int64_t count) {
        checkCount(count, sizeof(T));
        std::vector<T> result(count);
        for (int64_t i = 0; i < count; i++) {
            result[i] = readPrimitive<T>();
        }
        return result;
    }

    template<typename T>
    std::vector<T> readPrimitiveArray(uint64_t addr, int64_t count) {
        setPosition(addr);
        return readPrimitiveArray<T>(count);
    }

protected:
    std::vector<uint8_t> ownedData;

    // Rejects element counts that cannot fit in the remaining data. Garbage
    // counts (e.g. from a wrong registration candidate) would otherwise trigger
    // multi-GB allocations instead of a catchable error.
    void checkCount(int64_t count, uint64_t elemSize) const {
        if (count < 0 || static_cast<uint64_t>(count) > (dataSize - std::min<uint64_t>(position, dataSize)) / elemSize) {
            throw std::out_of_range("array read out of bounds: count=" + std::to_string(count) +
                                    " pos=" + std::to_string(position));
        }
    }

private:
    uint8_t* data = nullptr;
    size_t dataSize = 0;
    uint64_t position = 0;
};

// Template specializations for primitive types
template<>
inline int32_t BinaryStream::readPrimitive<int32_t>() { return readInt32(); }
template<>
inline uint32_t BinaryStream::readPrimitive<uint32_t>() { return readUInt32(); }
template<>
inline int64_t BinaryStream::readPrimitive<int64_t>() { return readInt64(); }
template<>
inline uint64_t BinaryStream::readPrimitive<uint64_t>() { return readUInt64(); }
template<>
inline int16_t BinaryStream::readPrimitive<int16_t>() { return readInt16(); }
template<>
inline uint16_t BinaryStream::readPrimitive<uint16_t>() { return readUInt16(); }

} // namespace il2cpp_dumper
