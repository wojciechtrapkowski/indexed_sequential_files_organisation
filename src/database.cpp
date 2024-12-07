#include "database.hpp"

#include <iostream>
Database::Database()
    : index_area(Settings::INDEX_FILE_PATH), main_area(Settings::MAIN_FILE_PATH), overflow_area(Settings::OVERFLOW_FILE_PATH)
{
    auto index_root = index_area.get_page(0);
    if (index_root->number_of_entries == 0)
    {
        index_root->entries[0] = {0, 0};
        index_root->number_of_entries = 1;
    }

    guardian = {main_area.get_header().overflow_page_index};
}

Database::~Database()
{
    auto &header = main_area.get_header();
    header.overflow_page_index = guardian.overflow_page_index;
}

void Database::delete_files()
{
    std::remove(Settings::INDEX_FILE_PATH.data());
    std::remove(Settings::MAIN_FILE_PATH.data());
    std::remove(Settings::OVERFLOW_FILE_PATH.data());
}

void Database::print()
{
    std::cout << "================================================" << std::endl;
    std::cout << "Index area" << std::endl;
    std::cout << "================================================" << std::endl;

    for (size_t i = 0; i < index_area.get_header().number_of_pages; ++i)
    {
        auto page = index_area.get_page(i);
        std::cout << "Page " << i << " number of entries: " << page->number_of_entries << std::endl;
        for (size_t j = 0; j < page->number_of_entries; ++j)
        {
            std::cout << "\tEntry " << j << "\n\t\tstart_key: " << page->entries[j].start_key
                      << "\n\t\tpage_index: " << page->entries[j].page_index << std::endl;
        }
    }

    std::cout << "================================================" << std::endl;
    std::cout << "Main area" << std::endl;
    std::cout << "================================================" << std::endl;

    std::cout << "Guardian overflow page index: " << (guardian.overflow_page_index == -1ULL ? "null" : std::to_string(guardian.overflow_page_index)) << '\n'
              << std::endl;

    for (size_t i = 0; i < main_area.get_header().number_of_pages; ++i)
    {
        auto page = main_area.get_page(i);
        std::cout << "Page " << i << " number of entries: " << page->number_of_entries << std::endl;
        for (size_t j = 0; j < page->number_of_entries; ++j)
        {
            std::cout << "\tEntry " << j << "\n\t\tkey: " << page->entries[j].key
                      << "\n\t\tvalue: " << page->entries[j].value
                      << "\n\t\toverflow_entry_index: "
                      << (page->entries[j].overflow_entry_index == -1ULL ? "null" : std::to_string(page->entries[j].overflow_entry_index))
                      << (page->entries[j].was_deleted ? "\n\t\tdeleted: true" : "") << std::endl;
        }
    }

    std::cout << "================================================" << std::endl;
    std::cout << "Overflow area" << std::endl;
    std::cout << "================================================" << std::endl;

    for (size_t i = 0; i < overflow_area.get_header().number_of_pages; ++i)
    {
        auto page = overflow_area.get_page(i);
        std::cout << "Page " << i << " number of entries: " << page->number_of_entries << std::endl;
        for (size_t j = 0; j < page->number_of_entries; ++j)
        {
            std::cout << "\tEntry " << j << "\n\t\tkey: " << page->entries[j].key
                      << "\n\t\tvalue: " << page->entries[j].value
                      << "\n\t\toverflow_entry_index: " << (page->entries[j].overflow_entry_index == -1ULL ? "null" : std::to_string(page->entries[j].overflow_entry_index))
                      << (page->entries[j].was_deleted ? "\n\t\tdeleted: true" : "") << std::endl;
        }
    }
}

// Helper function to find entry in overflow chain
std::optional<std::reference_wrapper<PageEntry>> Database::search_overflow_chain(size_t start_index, uint64_t key)
{
    size_t current_index = start_index;

    while (current_index != -1ULL)
    {
        auto page = overflow_area.get_page(current_index / Settings::PAGE_SIZE);
        auto &entry = page->entries[current_index % Settings::PAGE_SIZE];

        if (entry.was_deleted)
        {
            current_index = entry.overflow_entry_index;
            continue;
        }

        if (entry.key == key)
        {
            return entry;
        }
        current_index = entry.overflow_entry_index;
    }
    return std::nullopt;
}

// Helper function to find non-full overflow page and get next entry position
std::pair<size_t, size_t> Database::find_overflow_position()
{
    size_t overflow_page_index = 0;
    auto overflow_page = overflow_area.get_page(overflow_page_index);
    while (overflow_page->number_of_entries == Settings::PAGE_SIZE)
    {
        overflow_page_index++;
        overflow_page = overflow_area.get_page(overflow_page_index);
    }
    return {overflow_page_index, overflow_page->number_of_entries};
}

// Helper function to insert into overflow area and return the entry index
size_t Database::insert_overflow_entry(size_t page_index, size_t entry_pos, uint64_t key, uint64_t value)
{
    auto overflow_page = overflow_area.get_page(page_index);
    overflow_page->entries[entry_pos] = {key, value, -1ULL};
    overflow_page->number_of_entries++;
    return Settings::PAGE_SIZE * page_index + entry_pos;
}

// Helper function to find the end of an overflow chain and link new entry
void Database::link_overflow_entry(size_t start_index, size_t new_entry_index)
{
    size_t current_index = start_index;
    while (true)
    {
        auto &entry = overflow_area.get_page(current_index / Settings::PAGE_SIZE)
                          ->entries[current_index % Settings::PAGE_SIZE];
        if (entry.overflow_entry_index == -1ULL)
        {
            entry.overflow_entry_index = new_entry_index;
            break;
        }
        current_index = entry.overflow_entry_index;
    }
}

// Helper function to find index position for a key
std::pair<size_t, size_t> Database::find_index_position(uint64_t key)
{
    size_t current_index_page_index = 0;
    size_t entry_index_position = -1ULL;

    while (current_index_page_index < index_area.get_header().number_of_pages)
    {
        auto index_page = index_area.get_page(current_index_page_index);

        for (size_t i = 0; i < index_page->number_of_entries; ++i)
        {
            if (index_page->entries[i].start_key > key)
            {
                return {current_index_page_index, i > 0 ? i - 1 : -1ULL};
            }
            entry_index_position = i;
        }
        current_index_page_index++;
    }
    return {current_index_page_index - 1, entry_index_position};
}

std::optional<std::reference_wrapper<PageEntry>> Database::search_for_entry(uint64_t key)
{
    auto [page_idx, entry_pos] = find_index_position(key);

    // Check guardian if no index entry found
    if (entry_pos == -1ULL)
    {
        return guardian.overflow_page_index == -1ULL ? std::nullopt : search_overflow_chain(guardian.overflow_page_index, key);
    }

    // Search in main area page
    auto main_page = main_area.get_page(entry_pos);
    for (size_t i = 0; i < main_page->number_of_entries; ++i)
    {
        auto &entry = main_page->entries[i];
        if (entry.was_deleted)
        {
            continue;
        }

        if (entry.key == key)
        {
            return entry;
        }
        if (entry.overflow_entry_index != -1ULL)
        {
            auto result = search_overflow_chain(entry.overflow_entry_index, key);
            if (result)
                return result;
        }
        if (entry.key > key)
        {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<uint64_t> Database::search(uint64_t key)
{
    auto entry = search_for_entry(key);
    return entry ? std::make_optional(entry->get().value) : std::nullopt;
}

void Database::insert(uint64_t key, uint64_t value)
{
    if (search(key))
    {
        throw std::runtime_error("Key already exists");
    }

    auto [index_page_idx, entry_pos] = find_index_position(key);
    auto index_page = index_area.get_page(0);

    // Handle insertion into guardian (overflow area)
    if (entry_pos == -1ULL)
    {
        auto [page_idx, pos] = find_overflow_position();
        size_t new_entry_index = insert_overflow_entry(page_idx, pos, key, value);

        if (guardian.overflow_page_index == -1ULL)
        {
            guardian.overflow_page_index = new_entry_index;
        }
        else
        {
            link_overflow_entry(guardian.overflow_page_index, new_entry_index);
        }
        return;
    }

    // Handle first insert into index root
    if (index_page->entries[0].start_key == 0)
    {
        index_page->entries[0] = {key, main_area.get_header().number_of_pages};
        index_page->number_of_entries = 1;
    }

    // Insert into main area page
    auto main_page = main_area.get_page(entry_pos);
    size_t insert_pos = -1ULL;

    for (size_t i = 0; i < main_page->number_of_entries; ++i)
    {
        if (main_page->entries[i].key > key)
        {
            insert_pos = i - 1;
            break;
        }
    }

    // Insert as last record if possible
    if (main_page->number_of_entries < Settings::PAGE_SIZE)
    {
        if (insert_pos == -1ULL)
        {
            main_page->entries[main_page->number_of_entries] = {key, value, -1ULL};
            main_page->number_of_entries++;
            return;
        }
    }
    else
    {
        // Insert in overflow, when main area is full
        insert_pos = main_page->number_of_entries - 1;
    }

    // Insert into overflow area
    auto [page_idx, pos] = find_overflow_position();
    size_t new_entry_index = insert_overflow_entry(page_idx, pos, key, value);
    auto &entry = main_page->entries[insert_pos];

    if (entry.overflow_entry_index == -1ULL)
    {
        entry.overflow_entry_index = new_entry_index;
    }
    else
    {
        link_overflow_entry(entry.overflow_entry_index, new_entry_index);
    }
}

void Database::remove(uint64_t key)
{
    auto entry = search_for_entry(key);
    if (!entry)
    {
        return;
    }

    entry->get().was_deleted = 1;
}

// void Database::reorganise()
// {

// }