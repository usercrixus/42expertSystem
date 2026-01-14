#pragma once
#include <set>
#include <string>

class Parser;
class Resolver;

class App
{
public:
    int run(int argc, char **argv);

private:
    static bool parseInteractiveFacts(const std::string &line, std::set<char> &facts);
    static bool isUsageCorrect(int argc, char **argv);
    bool parseArgs(int argc, char **argv);
    int runInteractive(Parser &parser, Resolver &resolver);

    std::string input_path;
    bool print_trace = false;
    bool interactive_mode = false;
};
