#pragma once

#include <set>

std::pair<std::set<uint64_t>, std::set<uint64_t>> generate_keys_and_values(size_t number_of_keys);

uint64_t generate_key();

uint64_t generate_pesel();