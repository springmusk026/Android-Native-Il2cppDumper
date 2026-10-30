#include "elf.h"
#include "section_helper.h"
#include "boyer_moore.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

static const std::string ARMFeatureBytes = "? 0x10 ? 0xE7 ? 0x00 ? 0xE0 ? 0x20 ? 0xE0";

// ElfBase
ElfBase::ElfBase(const uint8_t* data, size_t size) : BinaryStream(data, size) {}
ElfBase::ElfBase(std::vector<uint8_t>&& data) : BinaryStream(std::move(data)) {}

// Elf
Elf::Elf(const uint8_t* data, size_t size) : ElfBase(data, size) {
    load();
}

Elf::Elf(std::vector<uint8_t>&& data) : ElfBase(std::move(data)) {
    load();
}

void Elf::load() {
    // Read ELF class byte to determine 32/64 bit
    setPosition(4); // e_class offset
    auto elfClass = readByte();
    is64Bit = (elfClass == 2);
    pt_dynamic32 = nullptr;
    pt_dynamic64 = nullptr;
    dynamicSection32.clear();
    dynamicSection64.clear();
    symbolTable32.clear();
    symbolTable64.clear();

    if (is64Bit) {
        is32Bit = false;
        // Read Elf64_Ehdr manually (POD struct, no read() method)
        setPosition(0);
        elf64Header.ei_mag = readUInt32();
        elf64Header.ei_class = readByte();
        elf64Header.ei_data = readByte();
        elf64Header.ei_version = readByte();
        elf64Header.ei_osabi = readByte();
        elf64Header.ei_abiversion = readByte();
        auto pad = readBytes(7);
        std::memcpy(elf64Header.ei_pad, pad.data(), 7);
        elf64Header.e_type = readUInt16();
        elf64Header.e_machine = readUInt16();
        elf64Header.e_version = readUInt32();
        elf64Header.e_entry = readUInt64();
        elf64Header.e_phoff = readUInt64();
        elf64Header.e_shoff = readUInt64();
        elf64Header.e_flags = readUInt32();
        elf64Header.e_ehsize = readUInt16();
        elf64Header.e_phentsize = readUInt16();
        elf64Header.e_phnum = readUInt16();
        elf64Header.e_shentsize = readUInt16();
        elf64Header.e_shnum = readUInt16();
        elf64Header.e_shstrndx = readUInt16();
        setPosition(elf64Header.e_phoff);
        programSegment64.resize(elf64Header.e_phnum);
        for (int i = 0; i < elf64Header.e_phnum; i++) {
            programSegment64[i].p_type = readUInt32();
            programSegment64[i].p_flags = readUInt32();
            programSegment64[i].p_offset = readUInt64();
            programSegment64[i].p_vaddr = readUInt64();
            programSegment64[i].p_paddr = readUInt64();
            programSegment64[i].p_filesz = readUInt64();
            programSegment64[i].p_memsz = readUInt64();
            programSegment64[i].p_align = readUInt64();
        }

        if (isDumped) fixedProgramSegment();

        for (auto& phdr : programSegment64) {
            if (phdr.p_type == ElfConstants::PT_DYNAMIC) {
                pt_dynamic64 = &phdr;
                break;
            }
        }

        if (pt_dynamic64) {
            auto count = pt_dynamic64->p_filesz / 16;
            setPosition(pt_dynamic64->p_offset);
            dynamicSection64.resize(count);
            for (size_t i = 0; i < count; i++) {
                dynamicSection64[i].d_tag = static_cast<int64_t>(readInt64());
                dynamicSection64[i].d_un = readUInt64();
            }
        }

        if (isDumped) fixedDynamicSection();

        readSymbol();

        if (!isDumped) {
            relocationProcessing();
            if (checkProtection()) {
                LOGI("WARNING: This file may be protected.");
                isProtected = true;
            }
        }
    } else {
        is32Bit = true;
        // Read Elf32_Ehdr manually (POD struct, no read() method)
        setPosition(0);
        elf32Header.ei_mag = readUInt32();
        elf32Header.ei_class = readByte();
        elf32Header.ei_data = readByte();
        elf32Header.ei_version = readByte();
        elf32Header.ei_osabi = readByte();
        elf32Header.ei_abiversion = readByte();
        auto pad32 = readBytes(7);
        std::memcpy(elf32Header.ei_pad, pad32.data(), 7);
        elf32Header.e_type = readUInt16();
        elf32Header.e_machine = readUInt16();
        elf32Header.e_version = readUInt32();
        elf32Header.e_entry = readUInt32();
        elf32Header.e_phoff = readUInt32();
        elf32Header.e_shoff = readUInt32();
        elf32Header.e_flags = readUInt32();
        elf32Header.e_ehsize = readUInt16();
        elf32Header.e_phentsize = readUInt16();
        elf32Header.e_phnum = readUInt16();
        elf32Header.e_shentsize = readUInt16();
        elf32Header.e_shnum = readUInt16();
        elf32Header.e_shstrndx = readUInt16();
        setPosition(elf32Header.e_phoff);
        programSegment32.resize(elf32Header.e_phnum);
        for (int i = 0; i < elf32Header.e_phnum; i++) {
            programSegment32[i].p_type = readUInt32();
            programSegment32[i].p_offset = readUInt32();
            programSegment32[i].p_vaddr = readUInt32();
            programSegment32[i].p_paddr = readUInt32();
            programSegment32[i].p_filesz = readUInt32();
            programSegment32[i].p_memsz = readUInt32();
            programSegment32[i].p_flags = readUInt32();
            programSegment32[i].p_align = readUInt32();
        }

        if (isDumped) fixedProgramSegment();

        for (auto& phdr : programSegment32) {
            if (phdr.p_type == ElfConstants::PT_DYNAMIC) {
                pt_dynamic32 = &phdr;
                break;
            }
        }

        if (pt_dynamic32) {
            auto count = pt_dynamic32->p_filesz / 8;
            setPosition(pt_dynamic32->p_offset);
            dynamicSection32.resize(count);
            for (size_t i = 0; i < count; i++) {
                dynamicSection32[i].d_tag = readInt32();
                dynamicSection32[i].d_un = readUInt32();
            }
        }

        if (isDumped) fixedDynamicSection();

        readSymbol();

        if (!isDumped) {
            relocationProcessing();
            if (checkProtection()) {
                LOGI("WARNING: This file may be protected.");
                isProtected = true;
            }
        }
    }
}

bool Elf::checkSection() {
    try {
        if (is64Bit) {
            sectionTable32.clear();
            setPosition(elf64Header.e_shoff);
            sectionTable64.resize(elf64Header.e_shnum);
            for (int i = 0; i < elf64Header.e_shnum; i++) {
                sectionTable64[i].sh_name = readUInt32();
                sectionTable64[i].sh_type = readUInt32();
                sectionTable64[i].sh_flags = readUInt64();
                sectionTable64[i].sh_addr = readUInt64();
                sectionTable64[i].sh_offset = readUInt64();
                sectionTable64[i].sh_size = readUInt64();
                sectionTable64[i].sh_link = readUInt32();
                sectionTable64[i].sh_info = readUInt32();
                sectionTable64[i].sh_addralign = readUInt64();
                sectionTable64[i].sh_entsize = readUInt64();
            }
            auto shstrndx = sectionTable64[elf64Header.e_shstrndx].sh_offset;
            for (auto& section : sectionTable64) {
                auto name = readStringToNull(shstrndx + section.sh_name);
                if (name == ".text") return true;
            }
        } else {
            sectionTable64.clear();
            setPosition(elf32Header.e_shoff);
            sectionTable32.resize(elf32Header.e_shnum);
            for (int i = 0; i < elf32Header.e_shnum; i++) {
                sectionTable32[i].sh_name = readUInt32();
                sectionTable32[i].sh_type = readUInt32();
                sectionTable32[i].sh_flags = readUInt32();
                sectionTable32[i].sh_addr = readUInt32();
                sectionTable32[i].sh_offset = readUInt32();
                sectionTable32[i].sh_size = readUInt32();
                sectionTable32[i].sh_link = readUInt32();
                sectionTable32[i].sh_info = readUInt32();
                sectionTable32[i].sh_addralign = readUInt32();
                sectionTable32[i].sh_entsize = readUInt32();
            }
            auto shstrndx = sectionTable32[elf32Header.e_shstrndx].sh_offset;
            for (auto& section : sectionTable32) {
                auto name = readStringToNull(shstrndx + section.sh_name);
                if (name == ".text") return true;
            }
        }
    } catch (...) {}
    return false;
}

uint64_t Elf::mapVATR(uint64_t addr) {
    if (is64Bit) {
        for (auto& phdr : programSegment64) {
            if (addr >= phdr.p_vaddr && addr <= phdr.p_vaddr + phdr.p_memsz) {
                return addr - phdr.p_vaddr + phdr.p_offset;
            }
        }
    } else {
        for (auto& phdr : programSegment32) {
            if (addr >= phdr.p_vaddr && addr <= phdr.p_vaddr + phdr.p_memsz) {
                return addr - phdr.p_vaddr + phdr.p_offset;
            }
        }
    }
    char msg[64];
    snprintf(msg, sizeof(msg), "mapVATR: address 0x%llx not mapped", (unsigned long long)addr);
    throw std::out_of_range(msg);
}

uint64_t Elf::mapRTVA(uint64_t addr) {
    if (is64Bit) {
        for (auto& phdr : programSegment64) {
            if (addr >= phdr.p_offset && addr <= phdr.p_offset + phdr.p_filesz) {
                return addr - phdr.p_offset + phdr.p_vaddr;
            }
        }
    } else {
        for (auto& phdr : programSegment32) {
            if (addr >= phdr.p_offset && addr <= phdr.p_offset + phdr.p_filesz) {
                return addr - phdr.p_offset + phdr.p_vaddr;
            }
        }
    }
    return 0;
}

bool Elf::search(uint64_t& codeRegistrationOut, uint64_t& metadataRegistrationOut) {
    // ARM feature bytes search, only implemented for 32-bit ARM (as upstream).
    if (is64Bit || elf32Header.e_machine != ElfConstants::EM_ARM) return false;

    uint32_t GOT = 0;
    for (auto& dyn : dynamicSection32) {
        if (dyn.d_tag == ElfConstants::DT_PLTGOT) {
            GOT = dyn.d_un;
            break;
        }
    }

    std::vector<uint32_t> resultList;
    for (auto& phdr : programSegment32) {
        if (phdr.p_type == ElfConstants::PT_LOAD && (phdr.p_flags & ElfConstants::PF_X)) {
            setPosition(phdr.p_offset);
            auto buff = readBytes(static_cast<int>(phdr.p_filesz));
            for (auto r : BoyerMooreHorspool::search(buff.data(), buff.size(), ARMFeatureBytes)) {
                // Bit 20 of the first instruction (L bit, bit 4 of byte 2) marks LDR.
                if (buff[r + 2] & 0x10) {
                    resultList.push_back(static_cast<uint32_t>(r));
                }
            }
        }
    }

    if (resultList.size() != 1) return false;

    uint32_t codeRegistration = 0;
    uint32_t metadataRegistration = 0;
    auto result = resultList[0];
    if (version < 24) {
        setPosition(result + 0x14);
        codeRegistration = readUInt32() + GOT;
        setPosition(result + 0x18);
        auto ptr = readUInt32() + GOT;
        setPosition(mapVATR(ptr));
        metadataRegistration = readUInt32();
    } else {
        setPosition(result + 0x14);
        codeRegistration = readUInt32() + result + 0xcu + static_cast<uint32_t>(imageBase);
        setPosition(result + 0x10);
        auto ptr = readUInt32() + result + 0x8;
        setPosition(mapVATR(ptr + imageBase));
        metadataRegistration = readUInt32();
    }

    LOGI("CodeRegistration : %x", codeRegistration);
    LOGI("MetadataRegistration : %x", metadataRegistration);
    codeRegistrationOut = codeRegistration;
    metadataRegistrationOut = metadataRegistration;
    return true;
}

bool Elf::symbolSearch(uint64_t& codeRegistrationOut, uint64_t& metadataRegistrationOut) {
    uint64_t codeRegistration = 0;
    uint64_t metadataRegistration = 0;
    bool hasStrtab = false;
    auto strtab = findDynamic(ElfConstants::DT_STRTAB, hasStrtab);
    if (!hasStrtab) {
        LOGI("ERROR: No symbol is detected");
        return false;
    }
    uint64_t dynstrOffset = mapVATR(strtab);

    auto visit = [&](uint32_t nameOffset, uint64_t value) {
        auto name = readStringToNull(dynstrOffset + nameOffset);
        if (name == "g_CodeRegistration") codeRegistration = value;
        else if (name == "g_MetadataRegistration") metadataRegistration = value;
    };
    if (is64Bit) {
        for (auto& sym : symbolTable64) visit(sym.st_name, sym.st_value);
    } else {
        for (auto& sym : symbolTable32) visit(sym.st_name, sym.st_value);
    }

    if (codeRegistration > 0 && metadataRegistration > 0) {
        LOGI("Detected Symbol!");
        LOGI("CodeRegistration : %llx", (unsigned long long)codeRegistration);
        LOGI("MetadataRegistration : %llx", (unsigned long long)metadataRegistration);
        codeRegistrationOut = codeRegistration;
        metadataRegistrationOut = metadataRegistration;
        return true;
    }
    LOGI("ERROR: No symbol is detected");
    return false;
}

uint64_t Elf::findDynamic(int64_t tag, bool& found) const {
    found = false;
    if (is64Bit) {
        for (auto& dyn : dynamicSection64) {
            if (dyn.d_tag == tag) { found = true; return dyn.d_un; }
        }
    } else {
        for (auto& dyn : dynamicSection32) {
            if (dyn.d_tag == tag) { found = true; return dyn.d_un; }
        }
    }
    return 0;
}

SectionHelper* Elf::getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount, int64_t metadataUsagesCount) {
    auto* helper = new SectionHelper(this, methodCount, typeDefinitionsCount, metadataUsagesCount, imageCount);

    if (is64Bit) {
        std::vector<Elf64_Phdr> execList, dataList;
        for (auto& phdr : programSegment64) {
            if (phdr.p_memsz != 0) {
                switch (phdr.p_flags) {
                    case 1: case 3: case 5: case 7:
                        execList.push_back(phdr);
                        break;
                    case 2: case 4: case 6:
                        dataList.push_back(phdr);
                        break;
                }
            }
        }
        helper->setSection(SearchSectionType::Exec, execList);
        helper->setSection(SearchSectionType::Data, dataList);
        helper->setSection(SearchSectionType::Bss, dataList);
    } else {
        std::vector<Elf32_Phdr> execList, dataList;
        for (auto& phdr : programSegment32) {
            if (phdr.p_memsz != 0) {
                switch (phdr.p_flags) {
                    case 1: case 3: case 5: case 7:
                        execList.push_back(phdr);
                        break;
                    case 2: case 4: case 6:
                        dataList.push_back(phdr);
                        break;
                }
            }
        }
        helper->setSection(SearchSectionType::Exec, execList);
        helper->setSection(SearchSectionType::Data, dataList);
        helper->setSection(SearchSectionType::Bss, dataList);
    }

    return helper;
}

bool Elf::checkDump() {
    return !checkSection();
}

uint64_t Elf::getRVA(uint64_t pointer) {
    if (isDumped) return pointer - imageBase;
    return pointer;
}

void Elf::readSymbol() {
    try {
        uint64_t symbolCount = 0;
        bool found = false;
        auto hash = findDynamic(ElfConstants::DT_HASH, found);
        if (found) {
            setPosition(mapVATR(hash));
            readUInt32(); // nbucket
            symbolCount = readUInt32(); // nchain
        } else {
            auto gnuHash = findDynamic(ElfConstants::DT_GNU_HASH, found);
            if (!found) return;
            auto addr = mapVATR(gnuHash);
            setPosition(addr);
            auto nbuckets = readUInt32();
            auto symoffset = readUInt32();
            auto bloom_size = readUInt32();
            readUInt32(); // bloom_shift
            // Bloom filter words are ELFCLASS-sized: 8 bytes on 64-bit, 4 on 32-bit.
            auto buckets_address = addr + 16 + (is64Bit ? 8ull : 4ull) * bloom_size;
            auto buckets = readPrimitiveArray<uint32_t>(buckets_address, nbuckets);
            if (buckets.empty()) return;
            uint64_t last_symbol = *std::max_element(buckets.begin(), buckets.end());
            if (last_symbol < symoffset) {
                symbolCount = symoffset;
            } else {
                auto chains_base_address = buckets_address + 4ull * nbuckets;
                setPosition(chains_base_address + (last_symbol - symoffset) * 4);
                while (getPosition() + 4 <= getLength()) {
                    auto chain_entry = readUInt32();
                    ++last_symbol;
                    if ((chain_entry & 1) != 0) break;
                }
                symbolCount = last_symbol;
            }
        }

        auto symtab = findDynamic(ElfConstants::DT_SYMTAB, found);
        if (!found || symbolCount == 0) return;
        auto dynsymOffset = mapVATR(symtab);
        setPosition(dynsymOffset);
        if (is64Bit) {
            checkCount(static_cast<int64_t>(symbolCount), 24);
            symbolTable64.resize(symbolCount);
            for (auto& sym : symbolTable64) {
                sym.st_name = readUInt32();
                sym.st_info = readByte();
                sym.st_other = readByte();
                sym.st_shndx = readUInt16();
                sym.st_value = readUInt64();
                sym.st_size = readUInt64();
            }
        } else {
            checkCount(static_cast<int64_t>(symbolCount), 16);
            symbolTable32.resize(symbolCount);
            for (auto& sym : symbolTable32) {
                sym.st_name = readUInt32();
                sym.st_value = readUInt32();
                sym.st_size = readUInt32();
                sym.st_info = readByte();
                sym.st_other = readByte();
                sym.st_shndx = readUInt16();
            }
        }
    } catch (...) {
        // ignored
    }
}

void Elf::relocationProcessing() {
    LOGI("Applying relocations...");
    try {
        if (is64Bit) {
            uint64_t relaOffset = 0, relaSize = 0;
            for (auto& dyn : dynamicSection64) {
                if (dyn.d_tag == ElfConstants::DT_RELA) relaOffset = dyn.d_un;
                if (dyn.d_tag == ElfConstants::DT_RELASZ) relaSize = dyn.d_un;
            }
            if (!relaOffset || !relaSize) return;
            auto count = relaSize / 24;
            auto relaPos = mapVATR(relaOffset);
            bool isX86_64 = (elf64Header.e_machine == ElfConstants::EM_X86_64);
            bool isAArch64 = (elf64Header.e_machine == ElfConstants::EM_AARCH64);
            for (uint64_t i = 0; i < count; i++) {
                setPosition(relaPos + i * 24);
                auto r_offset = readUInt64();
                auto r_info = readUInt64();
                auto r_addend = readUInt64();
                uint32_t type = static_cast<uint32_t>(r_info & 0xffffffff);
                uint64_t sym = r_info >> 32;
                uint64_t value;
                if ((isX86_64 && type == ElfConstants::R_X86_64_RELATIVE) ||
                    (isAArch64 && type == ElfConstants::R_AARCH64_RELATIVE)) {
                    value = r_addend;
                } else if ((isX86_64 && type == ElfConstants::R_X86_64_64) ||
                           (isAArch64 && type == ElfConstants::R_AARCH64_ABS64)) {
                    if (sym >= symbolTable64.size()) continue;
                    value = symbolTable64[sym].st_value + r_addend;
                } else {
                    continue;
                }
                try {
                    setPosition(mapVATR(r_offset));
                    writeUInt64(value);
                } catch (const std::out_of_range&) {
                    // target outside any segment; skip this entry only
                }
            }
        } else {
            uint32_t relOffset = 0, relSize = 0;
            for (auto& dyn : dynamicSection32) {
                if (dyn.d_tag == ElfConstants::DT_REL) relOffset = dyn.d_un;
                if (dyn.d_tag == ElfConstants::DT_RELSZ) relSize = dyn.d_un;
            }
            if (!relOffset || !relSize) return;
            auto count = relSize / 8;
            auto relPos = mapVATR(relOffset);
            bool isx86 = (elf32Header.e_machine == ElfConstants::EM_386);
            for (uint32_t i = 0; i < count; i++) {
                // Re-seek each entry: the write below moves the stream position.
                setPosition(relPos + static_cast<uint64_t>(i) * 8);
                auto r_offset = readUInt32();
                auto r_info = readUInt32();
                auto type = r_info & 0xff;
                auto sym = r_info >> 8;
                if ((isx86 && type == ElfConstants::R_386_32) ||
                    (!isx86 && type == ElfConstants::R_ARM_ABS32)) {
                    if (sym < symbolTable32.size()) {
                        try {
                            setPosition(mapVATR(r_offset));
                            writeUInt32(symbolTable32[sym].st_value);
                        } catch (const std::out_of_range&) {}
                    }
                }
            }
        }
    } catch (...) {
        // ignored
    }
}

bool Elf::checkProtection() {
    try {
        bool found = false;
        findDynamic(ElfConstants::DT_INIT, found);
        if (found) {
            LOGI("WARNING: find .init_proc");
            return true;
        }
        auto strtab = findDynamic(ElfConstants::DT_STRTAB, found);
        if (found) {
            auto dynstrOffset = mapVATR(strtab);
            auto isJniOnLoad = [&](uint32_t nameOffset) {
                return readStringToNull(dynstrOffset + nameOffset) == "JNI_OnLoad";
            };
            bool jni = false;
            if (is64Bit) {
                for (auto& sym : symbolTable64) if (isJniOnLoad(sym.st_name)) { jni = true; break; }
            } else {
                for (auto& sym : symbolTable32) if (isJniOnLoad(sym.st_name)) { jni = true; break; }
            }
            if (jni) {
                LOGI("WARNING: find JNI_OnLoad");
                return true;
            }
        }
        bool louser = is64Bit
            ? std::any_of(sectionTable64.begin(), sectionTable64.end(), [](const Elf64_Shdr& x) { return x.sh_type == ElfConstants::SHT_LOUSER; })
            : std::any_of(sectionTable32.begin(), sectionTable32.end(), [](const Elf32_Shdr& x) { return x.sh_type == ElfConstants::SHT_LOUSER; });
        if (louser) {
            LOGI("WARNING: find SHT_LOUSER section");
            return true;
        }
    } catch (...) {}
    return false;
}

void Elf::fixedProgramSegment() {
    // For dumped ELF files, fix program segments
    if (is64Bit) {
        for (size_t i = 0; i < programSegment64.size(); i++) {
            setPosition(elf64Header.e_phoff + i * 56 + 8); // offset to p_offset field
            auto& phdr = programSegment64[i];
            phdr.p_offset = phdr.p_vaddr;
            writeUInt64(phdr.p_offset);
            phdr.p_vaddr += imageBase;
            writeUInt64(phdr.p_vaddr);
            setPosition(getPosition() + 8); // skip p_paddr
            phdr.p_filesz = phdr.p_memsz;
            writeUInt64(phdr.p_filesz);
        }
    } else {
        for (size_t i = 0; i < programSegment32.size(); i++) {
            setPosition(elf32Header.e_phoff + i * 32 + 4);
            auto& phdr = programSegment32[i];
            phdr.p_offset = phdr.p_vaddr;
            writeUInt32(phdr.p_offset);
            phdr.p_vaddr += static_cast<uint32_t>(imageBase);
            writeUInt32(phdr.p_vaddr);
            setPosition(getPosition() + 4);
            phdr.p_filesz = phdr.p_memsz;
            writeUInt32(phdr.p_filesz);
        }
    }
}

void Elf::fixedDynamicSection() {
    if (is64Bit) {
        for (size_t i = 0; i < dynamicSection64.size(); i++) {
            auto& dyn = dynamicSection64[i];
            bool needsFix = false;
            switch (dyn.d_tag) {
                case ElfConstants::DT_PLTGOT: case ElfConstants::DT_HASH:
                case ElfConstants::DT_STRTAB: case ElfConstants::DT_SYMTAB:
                case ElfConstants::DT_RELA: case ElfConstants::DT_INIT:
                case ElfConstants::DT_FINI: case ElfConstants::DT_REL:
                case ElfConstants::DT_JMPREL: case ElfConstants::DT_INIT_ARRAY:
                case ElfConstants::DT_FINI_ARRAY:
                    needsFix = true;
                    break;
            }
            if (needsFix) {
                dyn.d_un += imageBase;
                setPosition(pt_dynamic64->p_offset + i * 16 + 8);
                writeUInt64(dyn.d_un);
            }
        }
    } else {
        for (size_t i = 0; i < dynamicSection32.size(); i++) {
            auto& dyn = dynamicSection32[i];
            bool needsFix = false;
            switch (dyn.d_tag) {
                case ElfConstants::DT_PLTGOT: case ElfConstants::DT_HASH:
                case ElfConstants::DT_STRTAB: case ElfConstants::DT_SYMTAB:
                case ElfConstants::DT_RELA: case ElfConstants::DT_INIT:
                case ElfConstants::DT_FINI: case ElfConstants::DT_REL:
                case ElfConstants::DT_JMPREL: case ElfConstants::DT_INIT_ARRAY:
                case ElfConstants::DT_FINI_ARRAY:
                    needsFix = true;
                    break;
            }
            if (needsFix) {
                dyn.d_un += static_cast<uint32_t>(imageBase);
                setPosition(pt_dynamic32->p_offset + i * 8 + 4);
                writeUInt32(dyn.d_un);
            }
        }
    }
}

} // namespace il2cpp_dumper
