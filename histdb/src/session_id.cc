#include <iostream>
#include <vector>

#include <histdb/session_id.h>
#include <histdb/command.h>
#include <histdb/exception.h>
#include <histdb/database.h>

#include <getopt.h> // getopt_long

#include "absl/strings/str_join.h"

namespace histdb {

SessionID::SessionID() {}

constexpr std::string_view SessionID::Name()  {
	return std::string_view("session");
};

constexpr std::string_view SessionID::Short() {
	return std::string_view("session: return a new session id");
};

constexpr std::string_view SessionID::Usage() {
	return std::string_view(R"""(initiate a new histdb session

Usage:
  histdb session [flags]

Flags:
	-e, --eval    print the session id as a statment that can be evaluated by bash
	-h, --help    help for insert
)""");
};

void SessionID::parseArguments(int argc, char *const *argv) {
	const struct option session_cmd_opts[] = {
		{"help",  no_argument, nullptr, 'h'},
		{"eval",  no_argument, nullptr, 'e'}, // print evalable output
		{nullptr, 0,           nullptr, 0},   // zero pad end
	};
	int ch;
	int opt_index = 0;
	if (argc <= 0) {
		return; // TODO: error
	}
	// getopt_long is bad and this is kinda broken for --eval
	while ((ch = getopt_long(argc, argv, "he", session_cmd_opts, &opt_index)) != -1) {
		switch (ch) {
		case 'h':
			print_usage = true;
			break;
		case 'e':
			print_eval = true;
			break;
		default:
			throw histdb::ArgumentException("invalid argument: ", ch);
			break;
		}
	}
	if (print_usage) {
		return;
	}
	argc -= optind;
	argv += optind;
	if (argc > 0) {
		std::vector<absl::string_view> extra;
		for (int i = 0; i < argc; i++) {
			extra.push_back(absl::string_view(argv[i]));
		}
		throw histdb::ArgumentException(
			"unexpected arguments: ", absl::StrJoin(extra, ", ")
		);
	}
}

// TODO: consider returning void
int SessionID::Run(int argc, char *const *argv) {
	parseArguments(argc, argv);
	if (print_usage) {
		std::cout << Usage();
		return 0;
	}
	auto db = histdb::OpenDefaultDatabase();

	throw std::runtime_error("IMPLEMENT IMPLEMENT IMPLEMENT IMPLEMENT");

	return 0;
}

} // namespace histdb
