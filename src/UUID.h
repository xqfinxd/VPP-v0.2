#pragma once

#include <cassert>
#include <cstdint>
#include <ctime>

namespace VPP {

typedef uint64_t UUID;

class UUIDGenerator {
public:
    UUIDGenerator() {}
    ~UUIDGenerator() = default;

    UUID Generate(uint16_t magicNumber) {
        uint64_t nowTime = time(nullptr);
        if(m_CurTime != nowTime) {
            m_CurTime = nowTime;
            m_Counter = 0;
        } else {
            m_Counter++;
        }
        return m_CurTime << 32 | (magicNumber << 16 | m_Counter);
    }

private:
    uint64_t m_CurTime = 0;
    uint16_t m_Counter = 0;
};

UUID GenerateUUID();

} // namespace VPP