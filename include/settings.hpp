#pragma once

#include <cstddef>
#include <string_view>

namespace Settings
{
    // Test are written with PAGE_SIZE = 8
    // constexpr size_t PAGE_SIZE = 4;
    constexpr size_t PAGE_SIZE = 4;
    constexpr size_t DEFAULT_PAGE_BUFFER_SIZE = 4;

    constexpr std::string_view INDEX_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/index.db";
    constexpr std::string_view MAIN_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/main.db";
    constexpr std::string_view OVERFLOW_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/overflow.db";

    constexpr std::string_view TEMP_INDEX_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/temp_index.db";
    constexpr std::string_view TEMP_MAIN_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/temp_main.db";
    constexpr std::string_view TEMP_OVERFLOW_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/temp_overflow.db";

    // TODO: Implement
    constexpr size_t INITIAL_NUMBER_OF_PAGES_IN_OVERFLOW_AREA = 1;

    // How many pages should be in overflow area after reorganisation
    // Test are written with BETA = 0.5
    constexpr double BETA = 1 / 8;
    // constexpr double BETA = 0.5;
    // How many entries should be in a page after reorganisation
    constexpr double ALPHA = 0.5;
    constexpr size_t NUMBER_OF_ENTRIES_AFTER_REORGANISATION = ALPHA * PAGE_SIZE;
}
