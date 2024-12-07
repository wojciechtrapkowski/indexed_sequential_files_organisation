#include <iostream>
#include "database.hpp"

int main()
{
    Database db;
    Database::delete_files();
    db.insert(3, 3);
    db.insert(6, 6);
    db.insert(4, 4);
    db.insert(5, 5);
    db.insert(2, 2);
    db.insert(1, 1);
    db.insert(0, 0);
    for (size_t i = 0; i <= 10; ++i)
    {
        std::cout << i << " " << (db.search(i).has_value() ? db.search(i).value() : -1) << std::endl;
    }
    db.insert(10, 10);
    db.insert(7, 7);
    db.insert(8, 8);
    db.insert(9, 9);
    for (size_t i = 0; i <= 10; ++i)
    {
        std::cout << i << " " << (db.search(i).has_value() ? db.search(i).value() : -1) << std::endl;
    }
    return 0;
}
