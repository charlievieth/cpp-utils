#pragma once

#include <stdexcept>
#include <string>

#include "absl/strings/str_cat.h"

namespace histdb {

// TODO: add a constructor that takes errno as its argument
class ErrnoException : public std::runtime_error {
public:
    ErrnoException(const std::string& message);
};

class ArgumentException : public std::invalid_argument {
public:
    ArgumentException(const std::string& message);

    ArgumentException(const absl::AlphaNum& a, const absl::AlphaNum& b);
};

} // namespace histdb
