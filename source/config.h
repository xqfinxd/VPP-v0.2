#pragma once

#include <memory>
#include <string>

namespace cfg {

class ISection {
 public:
  virtual std::string getString(const char* name) const = 0;
  virtual double getNumber(const char* name) const = 0;
  virtual int64_t getInteger(const char* name) const = 0;
  virtual ~ISection() {}
};

using Section = std::unique_ptr<ISection>;

void Load(const char* fn);
void Unload();
Section Find(const char* name);

}  // namespace cfg
