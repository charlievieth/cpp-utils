#pragma once

#include <string>

namespace histdb {

class Command {
public:
	virtual ~Command() = default;

	// Command name
	virtual constexpr std::string_view Name();

	// Short usage
	virtual constexpr std::string_view Short();

	virtual int Run(int argc, char * const argv[]);
};

} // namespace histdb
