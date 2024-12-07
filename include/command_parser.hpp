#pragma once
#include <string>
#include "database.hpp"

class CommandParser
{
public:
    explicit CommandParser(Database &db);

    // Process commands from console
    void run_interactive();

    // Process commands from file
    void run_from_file(const std::string &filename);

private:
    Database &database;
    void process_command(const std::string &line);
};