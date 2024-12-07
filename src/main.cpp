#include <iostream>

#include "database.hpp"
#include "command_parser.hpp"
#include "debug.hpp"

int main(int argc, char *argv[])
{
    if (argc > 1 && std::string(argv[1]) == "--clean")
    {
        Database::delete_files();
        return 0;
    }

    try
    {
        Database db;
        CommandParser parser(db);
        if (argc > 1)
        {
            // Process commands from file
            parser.run_from_file(argv[1]);
        }
        else
        {
            // Interactive mode
            parser.run_interactive();
        }
    }
    catch (const std::exception &e)
    {
        DEBUG_CERR << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}