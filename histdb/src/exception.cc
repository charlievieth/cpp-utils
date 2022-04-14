#include <histdb/exception.h>

#include <stdexcept>
#include <string>

namespace histdb {

ErrnoException::ErrnoException(const std::string& message) :
    std::runtime_error(message) {}

ArgumentException::ArgumentException(const std::string& message) :
    std::invalid_argument(message) {}

ArgumentException::ArgumentException(const absl::AlphaNum& a, const absl::AlphaNum& b) :
    std::invalid_argument(absl::StrCat(a, b)) {}

} // namespace histdb
