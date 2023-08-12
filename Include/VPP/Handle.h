#pragma once

#include <map>
#include <memory>
#include <vector>
#include <string>

// clang-format off

namespace VPP {

template <class TAG>
class Handle {
public:
  Handle() : m_Handle(0) {}
  void Init(uint32_t index) {
    assert(IsNull());
    assert(index < UINT32_MAX);

    static uint32_t _MagicCounter = 0;
    if (++_MagicCounter >= UINT32_MAX) _MagicCounter = 1;

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
    return m_Handle;
  }
  bool IsNull() const {
    return !m_Handle;
  }
  operator uint64_t() const {
    return m_Handle;
  }
  bool operator!=(const Handle& other) const {
    return m_Handle != other.m_Handle;
  }
  bool operator==(const Handle& other) const {
    return m_Handle == other.m_Handle;
  }
  bool operator<(const Handle& other) const {
    return m_Handle < other.m_Handle;
  }

private:
  union {
    struct {
      uint32_t  m_Index;
      uint32_t  m_Magic;
    };
    uint64_t    m_Handle;
  };
};

template <class TData, class THandle>
class HandleManager {
public:
  HandleManager() {}
  ~HandleManager() {}

  TData* Acquire(THandle& handle) {
    uint32_t index = UINT32_MAX;
    if (m_Free.empty()) {
      index = (uint32_t)m_Magics.size();
      handle.Init(index);
      m_Datas.emplace_back();
      m_Magics.push_back(handle.GetMagic());
    } else {
      index = m_Free.back();
      handle.Init(index);
      m_Free.pop_back();
      m_Magics[index] = handle.GetMagic();
    }

    return &m_Datas[index];
  }

  void Release(THandle handle) {
    auto index = handle.GetIndex();

    assert(index < m_Datas.size());
    assert(m_Magics[index] == handle.GetMagic());

    m_Magics[index] = 0;
    m_Free.push_back(index);
  }

  TData* Dereference(THandle handle) {
    if (handle.IsNull()) return nullptr;

    auto index = handle.GetIndex();
    if (index >= m_Datas.size() || m_Magics[index] != handle.GetMagic()) {
      return nullptr;
    }

    return &m_Datas[index];
  }

  const TData* Dereference(THandle handle) const {
    using ThisType = HandleManager<TData, THandle>;
    return (const_cast<ThisType*>(this)->Dereference(handle));
  }

private:
  using DataVec   = std::vector<TData>;
  using MagicVec  = std::vector<uint32_t>;
  using FreeVec   = std::vector<uint32_t>;

  DataVec   m_Datas;
  MagicVec  m_Magics;
  FreeVec   m_Free;
};

} // namespace VPP

// clang-format on
