#pragma once

#include <optional>
#include <vector>
#include <tuple>
#include "page_buffer.hpp"
#include "structures.hpp"
#include "settings.hpp"

enum class OperationType
{
    SEARCH,
    INSERT,
    UPDATE,
    REMOVE,
    REORGANISE,
    PRINT
};

std::ostream &operator<<(std::ostream &os, OperationType operation);

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

    void print_stats();

    std::optional<uint64_t> search(uint64_t key);

    void insert(uint64_t key, uint64_t value);

    void update(uint64_t key, uint64_t value);

    void remove(uint64_t key);

    void reorganise();

    void flush();

private:
    // Helper methods
    std::optional<std::reference_wrapper<PageEntry>> search_for_entry(uint64_t key);
    std::optional<std::reference_wrapper<PageEntry>> search_overflow_chain(size_t start_index, uint64_t key);
    std::tuple<std::optional<std::pair<size_t, size_t>>, double> find_overflow_position();
    size_t insert_overflow_entry(size_t page_index, size_t entry_pos, uint64_t key, uint64_t value);
    void link_overflow_entry(uint64_t &start_index, size_t new_entry_index);
    size_t find_index_position(uint64_t key);
    std::vector<PageEntry> gather_overflow_entries(size_t start_index);

    std::optional<uint64_t> search_wrapper(uint64_t key);

    void print_wrapper();

    void insert_wrapper(uint64_t key, uint64_t value);

    void update_wrapper(uint64_t key, uint64_t value);

    void remove_wrapper(uint64_t key);

    void reorganise_wrapper();

    void print_stats_after_operation(OperationType operation);

    void clear_counters();

    Guardian guardian;

    PageBuffer<IndexPage, Header> index_area;
    PageBuffer<Page, MainAreaHeader> main_area;
    PageBuffer<Page, Header> overflow_area;
};
