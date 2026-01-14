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
    /**
     * Verify the program is well used
     */
    static bool isUsageCorrect(int argc, char **argv);
    /**
     * Parse the option of the programs, verifying their usage is respected
     */
    bool parseArgs(int argc, char **argv);
    /**
     * Permit to set new inital facts using the stdinput
     */
    int runInteractive(Parser &parser, Resolver &resolver);
    /**
     * Parse an interactive facts line into a set of symbols. 
     */ 
    static bool parseInteractiveFacts(const std::string &line, std::set<char> &facts);
    // file path to manage (file containing the facts and logics links)
    std::string input_path;
    // debug mode activation
    bool print_trace = false;
    // interactive mode activation
    bool interactive_mode = false;
};
