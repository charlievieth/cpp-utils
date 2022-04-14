#pragma once

#include <histdb/command.h>
#include <string>

namespace histdb {

class SessionID : public histdb::Command {
public:
	~SessionID();

	SessionID();

	constexpr std::string_view Name();
	constexpr std::string_view Short();
	constexpr std::string_view Usage();

	int Run(int argc, char * const argv[]);

private:
	bool print_eval = false;
	bool print_usage = false;

	void parseArguments(int argc, char * const argv[]);

	// TODO: use static fields for the Name and things.
	// static std::string _Name = "SessionID";
	//
	// inline static const std::string _Name = "SessionID";
};

} // namespace histdb
