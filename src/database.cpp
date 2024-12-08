#include "database.hpp"

#include <cmath>
#include <iostream>

std::ostream &operator<<(std::ostream &os, OperationType operation)
{
    switch (operation)
    {
    case OperationType::SEARCH:
        os << "SEARCH";
        break;
    case OperationType::INSERT:
        os << "INSERT";
        break;
    case OperationType::UPDATE:
        os << "UPDATE";
        break;
    case OperationType::REMOVE:
        os << "REMOVE";
        break;
    case OperationType::REORGANISE:
        os << "REORGANISE";
        break;
    case OperationType::PRINT:
        os << "PRINT";
        break;
    }
    return os;
}

Database::Database()
    : index_area(Settings::INDEX_FILE_PATH), main_area(Settings::MAIN_FILE_PATH), overflow_area(Settings::OVERFLOW_FILE_PATH)
{
    auto index_root = index_area.get_page(0);
    if (index_root->number_of_entries == 0)
    {
        index_root->entries[0] = {0, 0};
        index_root->number_of_entries = 1;
    }

    for (size_t i = 1; i < Settings::INITIAL_NUMBER_OF_PAGES_IN_OVERFLOW_AREA; ++i)
    {
        overflow_area.create_page();
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
    clear_counters();
    print_wrapper();
    print_stats_after_operation(OperationType::PRINT);
}

void Database::print_stats()
{
    std::cout << "Disk operations statistics:\n";
    std::cout << "Index area reads: " << PageBuffer<IndexPage, Header>::get_all_read_count() << "\n";
    std::cout << "Index area writes: " << PageBuffer<IndexPage, Header>::get_all_write_count() << "\n";

    std::cout << "Main area reads: " << PageBuffer<Page, MainAreaHeader>::get_all_read_count() << "\n";
    std::cout << "Main area writes: " << PageBuffer<Page, MainAreaHeader>::get_all_write_count() << "\n";

    std::cout << "Overflow area reads: " << PageBuffer<Page, Header>::get_all_read_count() << "\n";
    std::cout << "Overflow area writes: " << PageBuffer<Page, Header>::get_all_write_count() << "\n";

    std::cout << "Combined reads: " << PageBuffer<IndexPage, Header>::get_all_read_count() + PageBuffer<Page, MainAreaHeader>::get_all_read_count() + PageBuffer<Page, Header>::get_all_read_count() << "\n";
    std::cout << "Combined writes: " << PageBuffer<IndexPage, Header>::get_all_write_count() + PageBuffer<Page, MainAreaHeader>::get_all_write_count() + PageBuffer<Page, Header>::get_all_write_count() << std::endl;
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

std::tuple<std::optional<std::pair<size_t, size_t>>, double> Database::find_overflow_position()
{
    std::optional<std::pair<size_t, size_t>> result = std::nullopt;
    size_t overflow_area_number_of_entries = 0;

    for (size_t i = 0; i < overflow_area.get_header().number_of_pages; ++i)
    {
        auto overflow_page = overflow_area.get_page(i);
        if (result == std::nullopt && overflow_page->number_of_entries < Settings::PAGE_SIZE)
        {
            result = std::make_pair(i, overflow_page->number_of_entries);
        }
        overflow_area_number_of_entries += overflow_page->number_of_entries;
    }
    return std::make_tuple(result, static_cast<double>(overflow_area_number_of_entries) / (overflow_area.get_header().number_of_pages * Settings::PAGE_SIZE));
}

// Helper function to insert into overflow area and return the entry index
size_t Database::insert_overflow_entry(size_t page_index, size_t entry_pos, uint64_t key, uint64_t value)
{
    auto overflow_page = overflow_area.get_page(page_index);
    overflow_page->entries[entry_pos] = {key, value, -1ULL};
    overflow_page->number_of_entries++;
    return Settings::PAGE_SIZE * page_index + entry_pos;
}

// Helper function to find the proper position in overflow chain for new entry
void Database::link_overflow_entry(uint64_t &start_index, size_t new_entry_index)
{
    auto &new_entry = overflow_area.get_page(new_entry_index / Settings::PAGE_SIZE)
                          ->entries[new_entry_index % Settings::PAGE_SIZE];
    uint64_t new_key = new_entry.key;

    size_t current_index = start_index;
    size_t prev_index = -1ULL;

    // Traverse the chain to find proper position
    while (current_index != -1ULL)
    {
        auto &current_entry = overflow_area.get_page(current_index / Settings::PAGE_SIZE)
                                  ->entries[current_index % Settings::PAGE_SIZE];

        // Found position where new key should be inserted
        if (current_entry.key > new_key)
        {
            // Insert between previous and current
            if (prev_index != -1ULL)
            {
                auto &prev_entry = overflow_area.get_page(prev_index / Settings::PAGE_SIZE)
                                       ->entries[prev_index % Settings::PAGE_SIZE];
                new_entry.overflow_entry_index = current_index;
                prev_entry.overflow_entry_index = new_entry_index;
            }
            else
            {
                // Insert at start
                new_entry.overflow_entry_index = current_index;
                start_index = new_entry_index;
            }
            return;
        }

        // Move to next entry
        if (current_entry.overflow_entry_index == -1ULL)
        {
            // Append at end if we reached the end
            current_entry.overflow_entry_index = new_entry_index;
            new_entry.overflow_entry_index = -1ULL;
            return;
        }

        prev_index = current_index;
        current_index = current_entry.overflow_entry_index;
    }
}

// Helper function to find index position for a key
size_t Database::find_index_position(uint64_t key)
{
    if (index_area.get_header().number_of_pages == 0)
    {
        return -1ULL;
    }

    // If key is smaller than first key in first page
    auto first_page = index_area.get_page(0);
    if (first_page->number_of_entries == 0)
    {
        return -1ULL;
    }

    if (key < first_page->entries[0].start_key)
    {
        return -1ULL;
    }

    // Iterate through all index pages
    for (size_t page_idx = 0; page_idx < index_area.get_header().number_of_pages; page_idx++)
    {
        auto index_page = index_area.get_page(page_idx);

        // Skip empty pages
        if (index_page->number_of_entries <= 1)
        {
            continue;
        }

        // Check all entries in this page
        for (size_t i = 0; i < index_page->number_of_entries - 1; i++)
        {
            // Validate page_index before returning
            if (index_page->entries[i].page_index >= main_area.get_header().number_of_pages)
            {
                throw std::runtime_error("Invalid page index in index entry");
            }

            if (key >= index_page->entries[i].start_key &&
                key < index_page->entries[i + 1].start_key)
            {
                return index_page->entries[i].page_index;
            }
        }

        // Check if it's in the last entry's range
        if (key >= index_page->entries[index_page->number_of_entries - 1].start_key)
        {
            // If this is not the last page, check if key is smaller than next page's first key
            if (page_idx < index_area.get_header().number_of_pages - 1)
            {
                auto next_page = index_area.get_page(page_idx + 1);
                if (next_page->number_of_entries > 0 && key < next_page->entries[0].start_key)
                {
                    return index_page->entries[index_page->number_of_entries - 1].page_index;
                }
                // If key is >= next page's first key, continue to next page
                continue;
            }
            else
            {
                // This is the last page, return its last entry
                size_t last_idx = index_page->number_of_entries - 1;
                if (index_page->entries[last_idx].page_index >= main_area.get_header().number_of_pages)
                {
                    throw std::runtime_error("Invalid last page index");
                }
                return index_page->entries[last_idx].page_index;
            }
        }
    }

    // If we get here, use the last entry of the last page
    auto last_page = index_area.get_page(index_area.get_header().number_of_pages - 1);
    size_t last_idx = last_page->number_of_entries - 1;

    // Validate final page_index
    if (last_page->entries[last_idx].page_index >= main_area.get_header().number_of_pages)
    {
        throw std::runtime_error("Invalid final page index");
    }

    return last_page->entries[last_idx].page_index;
}

std::optional<std::reference_wrapper<PageEntry>> Database::search_for_entry(uint64_t key)
{
    auto entry_pos = find_index_position(key);

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

std::vector<PageEntry> Database::gather_overflow_entries(size_t start_index)
{
    std::vector<PageEntry> entries;

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

        entries.push_back(entry);

        current_index = entry.overflow_entry_index;
    }
    return entries;
}

std::optional<uint64_t> Database::search_wrapper(uint64_t key)
{
    auto entry = search_for_entry(key);
    return entry ? std::make_optional(entry->get().value) : std::nullopt;
}

void Database::print_wrapper()
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

void Database::insert_wrapper(uint64_t key, uint64_t value)
{
    if (search_wrapper(key))
    {
        throw std::runtime_error("Key already exists");
    }

    auto entry_pos = find_index_position(key);
    auto index_page = index_area.get_page(0);

    // Handle insertion into guardian (overflow area)
    if (entry_pos == -1ULL)
    {
        auto [overflow_pos, overflow_area_fill] = find_overflow_position();

        // If overflow area is full, reorganise and try again
        if (!overflow_pos || overflow_area_fill >= Settings::GAMMA)
        {
            std::cout << "Overflow area is full, reorganising" << std::endl;
            reorganise();
            return insert(key, value);
        }

        auto [page_idx, pos] = *overflow_pos;
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
        index_page->entries[0] = {key, 0};
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
    if (insert_pos == -1ULL)
    {
        if (main_page->number_of_entries < Settings::PAGE_SIZE)
        {
            main_page->entries[main_page->number_of_entries] = {key, value, -1ULL};
            main_page->number_of_entries++;
            return;
        }
        else
        {
            // Insert in overflow, when main area is full
            insert_pos = main_page->number_of_entries - 1;
        }
    }

    // Insert into overflow area
    auto [overflow_pos, overflow_area_fill] = find_overflow_position();
    if (!overflow_pos || overflow_area_fill >= Settings::GAMMA)
    {
        std::cout << "Overflow area is full, reorganising" << std::endl;
        reorganise();
        return insert(key, value);
    }
    auto [page_idx, pos] = *overflow_pos;

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

void Database::update_wrapper(uint64_t key, uint64_t value)
{
    auto entry = search_for_entry(key);
    if (!entry)
    {
        return;
    }
    entry->get().value = value;
}

void Database::remove_wrapper(uint64_t key)
{
    auto entry = search_for_entry(key);
    if (!entry)
    {
        return;
    }

    entry->get().was_deleted = 1;
}

void Database::reorganise_wrapper()
{
    PageBuffer<IndexPage, Header> new_index_area(Settings::TEMP_INDEX_FILE_PATH, true);
    PageBuffer<Page, MainAreaHeader> new_main_area(Settings::TEMP_MAIN_FILE_PATH, true);
    PageBuffer<Page, Header> new_overflow_area(Settings::TEMP_OVERFLOW_FILE_PATH, true);

    size_t index_page_counter = 0;
    size_t main_page_counter = 0;

    auto current_main_page = new_main_area.get_page(0);
    auto current_index_page = new_index_area.get_page(0);

    auto create_new_main_page = [&]()
    {
        if (current_main_page->number_of_entries == Settings::NUMBER_OF_ENTRIES_AFTER_REORGANISATION)
        {
            main_page_counter++;
            current_main_page = new_main_area.create_page();
        }
    };

    // Setup main area

    for (size_t i = 0; i < main_area.get_header().number_of_pages; ++i)
    {
        size_t entries_per_page_counter = 0;
        auto page = main_area.get_page(i);
        for (size_t j = 0; j < page->number_of_entries; ++j)
        {
            auto entry = page->entries[j];
            if (entry.was_deleted)
            {
                continue;
            }
            create_new_main_page();

            // Gather all entries in overflow area from this entry
            auto all_entries = gather_overflow_entries(entry.overflow_entry_index);
            all_entries.insert(all_entries.begin(), entry);

            // Insert guardian entries first
            if (i == 0 && j == 0)
            {
                auto guardian_entries = gather_overflow_entries(guardian.overflow_page_index);
                all_entries.insert(all_entries.begin(), std::make_move_iterator(guardian_entries.begin()), std::make_move_iterator(guardian_entries.end()));
            }

            while (entries_per_page_counter < all_entries.size())
            {
                current_main_page->entries[current_main_page->number_of_entries] = all_entries[entries_per_page_counter];
                current_main_page->entries[current_main_page->number_of_entries].overflow_entry_index = -1ULL;
                current_main_page->number_of_entries++;
                entries_per_page_counter++;
                if (current_main_page->number_of_entries == Settings::NUMBER_OF_ENTRIES_AFTER_REORGANISATION && entries_per_page_counter < all_entries.size())
                {
                    create_new_main_page();
                }
            }

            entries_per_page_counter = 0;
        }
    }

    // Setup index area
    for (size_t i = 0; i < new_main_area.get_header().number_of_pages; ++i)
    {
        auto page = new_main_area.get_page(i);
        current_index_page->entries[current_index_page->number_of_entries] = {page->entries[0].key, page->index};
        current_index_page->number_of_entries++;

        if (current_index_page->number_of_entries == Settings::PAGE_SIZE && i < new_main_area.get_header().number_of_pages - 1)
        {
            current_index_page = new_index_area.create_page();
        }
    }

    // Create pages for overflow area
    for (size_t i = 1; i < std::ceil(new_main_area.get_header().number_of_pages * Settings::BETA); ++i)
    {
        new_overflow_area.create_page();
    }

    guardian.overflow_page_index = -1ULL;

    index_area = std::move(new_index_area);
    main_area = std::move(new_main_area);
    overflow_area = std::move(new_overflow_area);
}

void Database::print_stats_after_operation(OperationType operation)
{
    std::cout << "Operation: " << operation << std::endl;
    std::cout << "Index area reads: " << index_area.get_read_count() << "\n";
    std::cout << "Index area writes: " << index_area.get_write_count() << "\n";

    std::cout << "Main area reads: " << main_area.get_read_count() << "\n";
    std::cout << "Main area writes: " << main_area.get_write_count() << "\n";

    std::cout << "Overflow area reads: " << overflow_area.get_read_count() << "\n";
    std::cout << "Overflow area writes: " << overflow_area.get_write_count() << "\n";
}

void Database::clear_counters()
{
    main_area.clear_counters();
    index_area.clear_counters();
    overflow_area.clear_counters();
}

std::optional<uint64_t> Database::search(uint64_t key)
{
    clear_counters();
    auto result = search_wrapper(key);
    print_stats_after_operation(OperationType::SEARCH);

    return result;
}

void Database::insert(uint64_t key, uint64_t value)
{
    clear_counters();
    insert_wrapper(key, value);
    print_stats_after_operation(OperationType::INSERT);
}

void Database::update(uint64_t key, uint64_t value)
{
    clear_counters();
    update_wrapper(key, value);
    print_stats_after_operation(OperationType::UPDATE);
}

void Database::remove(uint64_t key)
{
    clear_counters();

    remove_wrapper(key);
    print_stats_after_operation(OperationType::REMOVE);
}

void Database::reorganise()
{
    clear_counters();

    reorganise_wrapper();
    print_stats_after_operation(OperationType::REORGANISE);
}

void Database::flush()
{
    index_area.flush();
    main_area.flush();
    overflow_area.flush();
}
