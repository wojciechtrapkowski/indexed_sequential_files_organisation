#include <iostream>
#include "database.hpp"
#include "command_parser.hpp"

int main(int argc, char *argv[])
{
    Database::delete_files();
    Database db;

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
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}