#pragma once

#include <optional>

#include "page_buffer.hpp"
#include "structures.hpp"

struct Database
{
public:
    Database();
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    Database(Database &&) = delete;
    Database &operator=(Database &&) = delete;

    static void delete_files();

    void print();

    std::optional<uint64_t> search(uint64_t key);

    void insert(uint64_t key, uint64_t value);

    void remove(uint64_t key);

    // void reorganise();

private:
    // Helper methods
    std::optional<std::reference_wrapper<PageEntry>> search_for_entry(uint64_t key);
    std::optional<std::reference_wrapper<PageEntry>> search_overflow_chain(size_t start_index, uint64_t key);
    std::pair<size_t, size_t> find_overflow_position();
    size_t insert_overflow_entry(size_t page_index, size_t entry_pos, uint64_t key, uint64_t value);
    void link_overflow_entry(size_t start_index, size_t new_entry_index);
    std::pair<size_t, size_t> find_index_position(uint64_t key);

    static constexpr std::string_view INDEX_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/index.db";
    static constexpr std::string_view MAIN_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/main.db";
    static constexpr std::string_view OVERFLOW_FILE_PATH = "/Users/wojtektrapkowski/studia/semestr_5/struktury_baz_danych/projekt_2_indeksowo_sekwencyjne/data/overflow.db";

    Guardian guardian;

    PageBuffer<IndexPage, Header> index_area;
    PageBuffer<Page, MainAreaHeader> main_area;
    PageBuffer<Page, Header> overflow_area;
};
