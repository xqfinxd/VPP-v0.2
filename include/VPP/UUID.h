#include <cassert>
#include <cstdint>

class UUID {
public:
    UUID()
        : m_Value(0) {}
    void Init(uint32_t index) {
        assert(IsNull());
        assert(index < UINT32_MAX);

        static uint32_t _MagicCounter = 0;
        if(++_MagicCounter >= UINT32_MAX) _MagicCounter = 1;

        m_Index = index;
        m_Magic = _MagicCounter;
    }
    uint32_t GetIndex() const {
        return m_Index;
    }
    uint32_t GetMagic() const {
        return m_Magic;
    }
    uint32_t GetHandle() const {
        return m_Value;
    }
    bool IsNull() const {
        return !m_Value;
    }
    operator uint64_t() const {
        return m_Value;
    }
    bool operator!=(const UUID &other) const {
        return m_Value != other.m_Value;
    }
    bool operator==(const UUID &other) const {
        return m_Value == other.m_Value;
    }
    bool operator<(const UUID &other) const {
        return m_Value < other.m_Value;
    }

private:
    union {
        struct {
            uint32_t m_Index;
            uint32_t m_Magic;
        };
        uint64_t m_Value;
    };
};