#pragma once

struct CommandLineArguments {
	std::vector<std::string> parameters;
	std::vector<std::string> options;

	CommandLineArguments(int argc, char **argv);
	~CommandLineArguments();
	bool isOption(std::string option);
	std::string getOptionValue(std::string option);
	std::vector<int> getOptionValueAsVector(std::string option);
};

