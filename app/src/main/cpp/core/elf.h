#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "binary_stream.h"
#include "elf_class.h"
#include "search_section.h"

namespace il2cpp_dumper {

// Forward declarations
class SectionHelper;

// Abstract base for ELF parsing
class ElfBase : public BinaryStream {
public:
    ElfBase(const uint8_t* data, size_t size);
    ElfBase(std::vector<uint8_t>&& data);
    virtual ~ElfBase() = default;

    virtual uint64_t mapVATR(uint64_t addr) = 0;
    virtual uint64_t mapRTVA(uint64_t addr) = 0;
    // Locate CodeRegistration/MetadataRegistration; the caller initializes the engine.
    virtual bool search(uint64_t& codeRegistration, uint64_t& metadataRegistration) = 0;
    virtual bool symbolSearch(uint64_t& codeRegistration, uint64_t& metadataRegistration) = 0;
    virtual SectionHelper* getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount, int64_t metadataUsagesCount = 0) = 0;
    virtual bool checkDump() = 0;
    virtual uint64_t getRVA(uint64_t pointer) = 0;

    bool isDumped = false;
    bool isProtected = false;
    void reload() { load(); }

protected:
    virtual void load() = 0;
    virtual bool checkSection() = 0;
};

// Combined 32/64-bit ELF parser
class Elf : public ElfBase {
public:
    Elf(const uint8_t* data, size_t size);
    Elf(std::vector<uint8_t>&& data);

    uint64_t mapVATR(uint64_t addr) override;
    uint64_t mapRTVA(uint64_t addr) override;
    bool search(uint64_t& codeRegistration, uint64_t& metadataRegistration) override;
    bool symbolSearch(uint64_t& codeRegistration, uint64_t& metadataRegistration) override;
    SectionHelper* getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount, int64_t metadataUsagesCount = 0) override;
    bool checkDump() override;
    uint64_t getRVA(uint64_t pointer) override;

protected:
    void load() override;
    bool checkSection() override;

private:
    bool is64Bit = false;

    // 32-bit structures
    Elf32_Ehdr elf32Header{};
    std::vector<Elf32_Phdr> programSegment32;
    std::vector<Elf32_Dyn> dynamicSection32;
    std::vector<Elf32_Sym> symbolTable32;
    std::vector<Elf32_Shdr> sectionTable32;
    Elf32_Phdr* pt_dynamic32 = nullptr;

    // 64-bit structures
    Elf64_Ehdr elf64Header{};
    std::vector<Elf64_Phdr> programSegment64;
    std::vector<Elf64_Dyn> dynamicSection64;
    std::vector<Elf64_Sym> symbolTable64;
    std::vector<Elf64_Shdr> sectionTable64;
    Elf64_Phdr* pt_dynamic64 = nullptr;

    void readSymbol();
    uint64_t findDynamic(int64_t tag, bool& found) const;
    void relocationProcessing();
    bool checkProtection();
    void fixedProgramSegment();
    void fixedDynamicSection();

    // Template helpers for 32/64 bit operations
    template<typename Phdr>
    uint64_t mapVATRTemplate(uint64_t addr, const std::vector<Phdr>& segments);
    template<typename Phdr>
    uint64_t mapRTVATemplate(uint64_t addr, const std::vector<Phdr>& segments);
};

} // namespace il2cpp_dumper
