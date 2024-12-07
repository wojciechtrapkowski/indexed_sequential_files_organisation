#pragma once

#include <cstddef>
#include <string_view>

namespace Settings
{
    constexpr size_t PAGE_SIZE = 8;
    constexpr size_t DEFAULT_PAGE_BUFFER_SIZE = 3;

    constexpr std::string_view INDEX_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/index.db";
    constexpr std::string_view MAIN_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/main.db";
    constexpr std::string_view OVERFLOW_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/overflow.db";
}