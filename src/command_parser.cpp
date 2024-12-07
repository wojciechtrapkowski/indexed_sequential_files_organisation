#include <iostream>
#include <fstream>
#include <sstream>

#include "command_parser.hpp"
#include "debug.hpp"

CommandParser::CommandParser(Database &db) : database(db) {}

void CommandParser::run_interactive()
{
    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line))
    {
        if (line == "exit" || line == "quit")
            break;
        process_command(line);
    }
}

void CommandParser::run_from_file(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string line;
    while (std::getline(file, line))
    {
        process_command(line);
    }
}

void CommandParser::process_command(const std::string &line)
{
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (command == "insert")
    {
        uint64_t key, value;
        if (iss >> key >> value)
        {
            try
            {
                database.insert(key, value);
                std::cout << std::endl;
            }
            catch (const std::exception &e)
            {
                DEBUG_CERR << "Error: " << e.what() << std::endl;
            }
        }
    }
    else if (command == "search")
    {
        uint64_t key;
        if (iss >> key)
        {
            auto result = database.search(key);
            if (result)
            {
                std::cout << *result << std::endl;
            }
            else
            {
                std::cout << "Not found: " << key << std::endl;
            }
        }
    }
    else if (command == "print")
    {
        database.print();
    }
    else if (command == "remove")
    {
        uint64_t key;
        if (iss >> key)
        {
            database.remove(key);
        }
    }
    else if (command == "help")
    {
        std::cout << "Available commands:\n"
                  << "  insert <key> <value>\n"
                  << "  search <key>\n"
                  << "  help\n"
                  << "  exit/quit\n";
    }
    else
    {
        std::cout << "Unknown command. Type 'help' for available commands.\n";
    }
}