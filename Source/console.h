#pragma once

namespace Console {
inline std::ostream& clear(std::ostream& o) {
    o << "\033[0m";
    return o;
}

inline std::ostream& log(std::ostream& o) {
    o << "\033[97m";
    return o;
}

inline std::ostream& warn(std::ostream& o) {
    o << "\033[1;93m";
    return o;
}

inline std::ostream& error(std::ostream& o) {
    o << "\033[1;91m";
    return o;
}
}  // namespace Console