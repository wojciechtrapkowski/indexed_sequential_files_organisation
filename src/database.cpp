#include "database.hpp"

Database::Database()
    : index_area(INDEX_FILE_PATH), main_area(MAIN_FILE_PATH), overflow_area(OVERFLOW_FILE_PATH)
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
    std::remove(INDEX_FILE_PATH.data());
    std::remove(MAIN_FILE_PATH.data());
    std::remove(OVERFLOW_FILE_PATH.data());
}

std::optional<uint64_t> Database::search(uint64_t key)
{
    // Iterate over the indexes, to find page that we are interested in
    size_t current_index_page_index = 0;
    size_t entry_index_position = 0;
    auto index_page = index_area.get_page(current_index_page_index);

    while (current_index_page_index < index_area.get_header().number_of_pages)
    {
        index_page = index_area.get_page(current_index_page_index);
        current_index_page_index++;

        size_t number_of_entries = index_page->number_of_entries;
        for (size_t i = 0; i < number_of_entries; ++i)
        {
            auto entry = index_page->entries[i];
            if (entry.start_key > key)
            {
                entry_index_position = i - 1;
                break;
            }
            if (entry.start_key == key)
            {
                entry_index_position = i;
                break;
            }
        }

        if (entry_index_position != -1ULL)
        {
            break;
        }
    }

    if (entry_index_position == -1ULL)
    {
        // If we did not find any page, we check guardian
        if (guardian.overflow_page_index == -1ULL)
        {
            return std::nullopt;
        }

        // If we found guardian, we need to search in overflow
        auto overflow_page = overflow_area.get_page(guardian.overflow_page_index / PAGE_SIZE);
        auto overflow_entry = overflow_page->entries[guardian.overflow_page_index % PAGE_SIZE];
        if (overflow_entry.key == key)
        {
            return overflow_entry.value;
        }

        while (overflow_entry.overflow_entry_index != -1ULL)
        {
            overflow_entry = overflow_area.get_page(overflow_entry.overflow_entry_index / PAGE_SIZE)->entries[overflow_entry.overflow_entry_index % PAGE_SIZE];
            if (overflow_entry.key == key)
            {
                return overflow_entry.value;
            }
        }

        return std::nullopt;
    }

    // We are searching in page
    auto main_area_page = main_area.get_page(entry_index_position);

    for (size_t i = 0; i < main_area_page->number_of_entries; ++i)
    {
        if (main_area_page->entries[i].key == key)
        {
            return main_area_page->entries[i].value;
        }
        if (main_area_page->entries[i].overflow_entry_index != -1ULL)
        {
            auto overflow_page = overflow_area.get_page(main_area_page->entries[i].overflow_entry_index / PAGE_SIZE);
            auto overflow_entry = overflow_page->entries[main_area_page->entries[i].overflow_entry_index % PAGE_SIZE];

            if (overflow_entry.key == key)
            {
                return overflow_entry.value;
            }

            while (overflow_entry.overflow_entry_index != -1ULL)
            {
                overflow_entry = overflow_area.get_page(overflow_entry.overflow_entry_index / PAGE_SIZE)->entries[overflow_entry.overflow_entry_index % PAGE_SIZE];
                if (overflow_entry.key == key)
                {
                    return overflow_entry.value;
                }
            }
        }
        if (main_area_page->entries[i].key > key)
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

void Database::insert(uint64_t key, uint64_t value)
{
    if (search(key))
    {
        throw std::runtime_error("Key already exists");
    }

    // Iterate over the indexes, to find page that we are interested in
    size_t current_index_page_index = 0;
    size_t entry_index_position = -1;
    auto index_page = index_area.get_page(current_index_page_index);

    // Find the page that we are interested in
    while (current_index_page_index < index_area.get_header().number_of_pages)
    {
        index_page = index_area.get_page(current_index_page_index);
        current_index_page_index++;

        size_t number_of_entries = index_page->number_of_entries;
        for (size_t i = 0; i < number_of_entries; ++i)
        {
            if (key > index_page->entries[i].start_key)
            {
                entry_index_position = i;
            }
            if (index_page->entries[i].start_key > key)
            {
                break;
            }
        }
    }

    // If we did not find any page, we need to insert into guardian
    if (entry_index_position == -1ULL)
    {
        // Insert into overflow

        // Find non full overflow page
        size_t overflow_page_index = 0;
        auto overflow_page = overflow_area.get_page(overflow_page_index);
        while (overflow_page->number_of_entries == PAGE_SIZE)
        {
            overflow_page_index++;
            overflow_page = overflow_area.get_page(overflow_page_index);
        }

        // If overflow entry is null, we can insert new entry into overflow page
        if (guardian.overflow_page_index == -1ULL)
        {
            overflow_page->entries[overflow_page->number_of_entries] = {key, value, -1ULL};
            guardian.overflow_page_index = PAGE_SIZE * overflow_page_index + overflow_page->number_of_entries;
            overflow_page->number_of_entries++;
            return;
        }

        // If overflow entry is not null, we need to find the end of the overflow chain

        auto overflow_entry_index = guardian.overflow_page_index;
        while (true)
        {
            auto &overflow_entry = overflow_area.get_page(overflow_entry_index / PAGE_SIZE)->entries[overflow_entry_index % PAGE_SIZE];
            if (overflow_entry.overflow_entry_index == -1ULL)
            {
                overflow_entry.overflow_entry_index = PAGE_SIZE * overflow_page_index + overflow_page->number_of_entries;
                break;
            }
            overflow_entry_index = overflow_entry.overflow_entry_index;
        }

        // Insert new entry into page
        overflow_page->entries[overflow_page->number_of_entries] = {key, value, -1ULL};
        overflow_page->number_of_entries++;
        return;
    }

    // If this is first insert, we need to insert into index root
    if (index_page->entries[0].start_key == 0)
    {
        index_page->entries[0].start_key = key;
        index_page->entries[0].page_index = main_area.get_header().number_of_pages;
        index_page->number_of_entries = 1;
    }

    // We are inserting into page
    auto main_area_page = main_area.get_page(entry_index_position);

    size_t place_to_insert_index = -1ULL;
    for (size_t i = 0; i < main_area_page->number_of_entries; ++i)
    {
        if (main_area_page->entries[i].key > key)
        {
            place_to_insert_index = i - 1;
            break;
        }
    }

    // We can insert this record as the last record
    if (place_to_insert_index == -1ULL)
    {
        main_area_page->entries[main_area_page->number_of_entries] = {key, value, -1ULL};
        main_area_page->number_of_entries++;
        return;
    }

    // We need to insert this record in the overflow area
    auto &entry = main_area_page->entries[place_to_insert_index];

    // Find non full overflow page
    size_t overflow_page_index = 0;
    auto overflow_page = overflow_area.get_page(overflow_page_index);
    while (overflow_page->number_of_entries == PAGE_SIZE)
    {
        overflow_page_index++;
        overflow_page = overflow_area.get_page(overflow_page_index);
    }

    // If overflow entry is null, we can insert new entry into overflow page
    if (entry.overflow_entry_index == -1ULL)
    {
        overflow_page->entries[overflow_page->number_of_entries] = {key, value, -1ULL};
        entry.overflow_entry_index = PAGE_SIZE * overflow_page_index + overflow_page->number_of_entries;
        overflow_page->number_of_entries++;
        return;
    }

    // If overflow entry is not null, we need to find the end of the overflow chain
    size_t overflow_entry_index = entry.overflow_entry_index;
    while (true)
    {
        auto &overflow_entry = overflow_area.get_page(overflow_entry_index / PAGE_SIZE)->entries[overflow_entry_index % PAGE_SIZE];
        if (overflow_entry.overflow_entry_index == -1ULL)
        {
            overflow_entry.overflow_entry_index = PAGE_SIZE * overflow_page_index + overflow_page->number_of_entries;
            break;
        }
        overflow_entry_index = overflow_entry.overflow_entry_index;
    }

    // Insert new entry into page
    overflow_page->entries[overflow_page->number_of_entries] = {key, value, -1ULL};
    overflow_page->number_of_entries++;
}
