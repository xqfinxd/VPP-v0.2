#include "UUID.h"
#include <random>

namespace VPP {

UUID GenerateUUID() {
    static UUIDGenerator s_Generator;
    static std::random_device s_RandomDevice;
    static std::mt19937 s_Engine(s_RandomDevice());
    static std::uniform_int_distribution<uint16_t> s_UniformDistribution;
    return s_Generator.Generate(s_UniformDistribution(s_Engine));
}

} // namespace VPP