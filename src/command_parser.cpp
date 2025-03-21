#include <iostream>
#include <fstream>
#include <sstream>

#include "command_parser.hpp"
#include "debug.hpp"
#include "utils.hpp"

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
        else
        {
            std::cout << "Invalid command. Type 'help' for available commands.\n";
        }
    }
    else if (command == "update")
    {
        uint64_t key, value;
        if (iss >> key >> value)
        {
            database.update(key, value);
        }
        else
        {
            std::cout << "Invalid command. Type 'help' for available commands.\n";
        }
    }
    else if (command == "flush")
    {
        database.flush();
    }
    else if (command == "remove")
    {
        uint64_t key;
        if (iss >> key)
        {
            database.remove(key);
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
    else if (command == "print_stats")
    {
        database.print_stats();
    }
    else if (command == "generate")
    {
        size_t number_of_keys;
        if (iss >> number_of_keys)
        {
            // Number of keys should be checked before generating
            auto [keys, values] = generate_keys_and_values(number_of_keys);
            for (size_t i = 0; i < number_of_keys; i++)
            {
                database.insert(keys.extract(keys.begin()).value(), values.extract(values.begin()).value());
            }
        }
    }
    else if (command == "reorganise")
    {
        database.reorganise();
    }
    else if (command == "help")
    {
        std::cout << "Available commands:\n"
                  << "  insert <key> <value>\n"
                  << "  update <key> <value>\n"
                  << "  search <key>\n"
                  << "  print\n"
                  << "  print_stats\n"
                  << "  remove <key>\n"
                  << "  generate <number_of_keys>\n"
                  << "  reorganise\n"
                  << "  help\n"
                  << "  exit/quit\n";
    }
    else
    {
        std::cout << "Unknown command. Type 'help' for available commands.\n";
    }
}