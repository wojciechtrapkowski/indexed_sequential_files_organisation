#include "utils.hpp"

#include <random>
#include <sstream>
#include <iomanip>

std::pair<std::set<uint64_t>, std::set<uint64_t>> generate_keys_and_values(size_t number_of_keys)
{
    std::set<uint64_t> keys;
    std::set<uint64_t> values;

    while (keys.size() < number_of_keys)
    {
        keys.insert(generate_key());
    }

    while (values.size() < number_of_keys)
    {
        values.insert(generate_pesel());
    }

    return {keys, values};
}
uint64_t generate_pesel()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> year(0, 99);
    std::uniform_int_distribution<> month(1, 12);
    std::uniform_int_distribution<> day(1, 28);
    std::uniform_int_distribution<> serial(0, 9999);

    std::stringstream pesel;
    pesel << std::setfill('0')
          << std::setw(2) << year(gen)
          << std::setw(2) << month(gen)
          << std::setw(2) << day(gen)
          << std::setw(4) << serial(gen)
          << std::setw(1) << (gen() % 10);

    return std::stoull(pesel.str());
}

uint64_t generate_key()
{
    static std::mt19937 gen(std::random_device{}());
    return gen();
}