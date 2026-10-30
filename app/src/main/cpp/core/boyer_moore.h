#pragma once

#include <cstdint>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace il2cpp_dumper {

class BoyerMooreHorspool {
public:
    // Search for exact byte pattern
    static std::vector<int> search(const uint8_t* source, size_t sourceLen,
                                   const uint8_t* pattern, size_t patternLen) {
        std::vector<int> results;
        if (!source || !pattern || sourceLen == 0 || patternLen == 0 || patternLen > sourceLen)
            return results;

        int badCharacters[256];
        for (int i = 0; i < 256; i++)
            badCharacters[i] = static_cast<int>(patternLen);

        auto lastPatternByte = static_cast<int>(patternLen - 1);
        for (int i = 0; i < lastPatternByte; i++)
            badCharacters[pattern[i]] = lastPatternByte - i;

        size_t index = 0;
        while (index <= sourceLen - patternLen) {
            int i = lastPatternByte;
            while (source[index + i] == pattern[i]) {
                if (i == 0) {
                    results.push_back(static_cast<int>(index));
                    break;
                }
                i--;
            }
            index += badCharacters[source[index + lastPatternByte]];
        }
        return results;
    }

    // Search with wildcard pattern string (space-separated hex bytes, "?" for wildcards)
    static std::vector<int> search(const uint8_t* source, size_t sourceLen,
                                   const std::string& stringPattern) {
        std::vector<int> results;
        if (!source || sourceLen == 0 || stringPattern.empty())
            return results;

        // Parse once: -1 marks a wildcard.
        std::vector<int> pattern;
        std::string token;
        auto flush = [&]() {
            if (token.empty()) return;
            pattern.push_back(token == "?" ? -1 : static_cast<int>(std::stoul(token, nullptr, 16) & 0xFF));
            token.clear();
        };
        for (char c : stringPattern) {
            if (c == ' ') flush();
            else token += c;
        }
        flush();

        size_t patternLen = pattern.size();
        if (patternLen == 0 || patternLen > sourceLen)
            return results;

        auto lastPatternByte = static_cast<int>(patternLen - 1);
        // A wildcard matches any byte, so no shift may jump past the last
        // wildcard before the final position.
        int maxShift = static_cast<int>(patternLen);
        for (int i = 0; i < lastPatternByte; i++) {
            if (pattern[i] < 0) maxShift = lastPatternByte - i;
        }
        int badCharacters[256];
        for (int i = 0; i < 256; i++)
            badCharacters[i] = maxShift;
        // In-order assignment lets later occurrences win (smallest shift).
        for (int i = 0; i < lastPatternByte; i++) {
            if (pattern[i] >= 0) badCharacters[pattern[i]] = std::min(maxShift, lastPatternByte - i);
        }

        size_t index = 0;
        while (index <= sourceLen - patternLen) {
            int i = lastPatternByte;
            while (pattern[i] < 0 || source[index + i] == pattern[i]) {
                if (i == 0) {
                    results.push_back(static_cast<int>(index));
                    break;
                }
                i--;
            }
            index += badCharacters[source[index + lastPatternByte]];
        }
        return results;
    }
};

} // namespace il2cpp_dumper
