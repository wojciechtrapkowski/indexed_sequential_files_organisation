#pragma once

#include <cstdint>
#include <array>

#include "settings.hpp"
struct Guardian
{
    uint64_t overflow_page_index = -1;
};

struct Header
{
    uint64_t number_of_pages = 0;
};

struct MainAreaHeader
{
    uint64_t number_of_pages = 0;
    uint64_t overflow_page_index = -1; // our guardian
};

struct PageEntry
{
    uint64_t key = -1;
    uint64_t value = -1; // in our case PESEL
    uint64_t overflow_entry_index = -1;
    uint64_t was_deleted = 0;
};

struct Page
{
    uint64_t index = -1;
    uint64_t number_of_entries = 0;
    std::array<PageEntry, Settings::PAGE_SIZE> entries;
};

struct IndexEntry
{
    uint64_t start_key = -1;
    uint64_t page_index = -1;
};

struct IndexPage
{
    uint64_t index = -1;
    uint64_t number_of_entries = 0;
    std::array<IndexEntry, Settings::PAGE_SIZE> entries;
};