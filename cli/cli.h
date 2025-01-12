#pragma once

#include <string>
#include "View.h"
#include "Illustrace.h"
#include "Editor.h"

namespace illustrace {

class CLI {
public:
    static int main(int argc, char **argv);
    static const std::string VERSION;

    CLI();
    ~CLI();

    void help();
    void usage();
    void version();
    bool execute(const char *inputFilePath);
    void executeCommand(char *commandLine, int line);

    Document *document;
    View view;
    Illustrace illustrace;
    Editor *editor;
    const char *editFilePath;
    const char *outputFilepath;
};

} // namespace illustrace
