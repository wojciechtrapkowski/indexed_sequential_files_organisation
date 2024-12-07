#pragma once

#include <array>
#include <concepts>
#include <memory>
#include <fstream>

#include "scoped_file.hpp"
#include "settings.hpp"

template <typename T>
concept HasIndex = requires(T t) {
    { t.index } -> std::convertible_to<size_t>;
};

template <typename T>
concept HasNumberOfPages = requires(T t) {
    { t.number_of_pages } -> std::convertible_to<size_t>;
};

template <typename T>
concept HasNumberOfEntries = requires(T t) {
    { t.number_of_entries } -> std::convertible_to<size_t>;
};

template <typename Page, typename Header>
    requires HasIndex<Page> && HasNumberOfPages<Header> && HasNumberOfEntries<Page>
class PageBuffer
{
public:
    using PagePtr = std::shared_ptr<Page>;

private:
    size_t evict_page()
    {
        for (size_t i = 0; i < pages.size(); ++i)
        {
            if (pages[i] && pages[i].use_count() == 1)
            {
                // Save this page to disk
                write_page_to_disk(*pages[i]);
                return i;
            }
        }

        throw std::runtime_error("No page to evict");
    }

    Page get_page_from_disk(size_t index)
    {
        Page page;
        if (!file.read(reinterpret_cast<char *>(&page), sizeof(Page), sizeof(Header) + index * sizeof(Page)))
        {
            throw std::runtime_error("Failed to read page from disk");
        }
        return page;
    }

    void write_page_to_disk(const Page &page)
    {
        file.write(reinterpret_cast<const char *>(&page), sizeof(Page), sizeof(Header) + page.index * sizeof(Page));
    }

    Header header;
    std::array<PagePtr, Settings::DEFAULT_PAGE_BUFFER_SIZE> pages;
    ScopedFile file;
    std::string file_path;

public:
    PageBuffer(std::string_view file_path, bool truncate = false) : file(file_path, truncate), file_path(file_path)
    {
        pages.fill(nullptr);

        // Try to read header from disk
        // If header is not found, create a new one along with a new root page
        if (!file.read(reinterpret_cast<char *>(&header), sizeof(Header), 0))
        {
            // Header not found, create a new one
            header = Header();
            pages[0] = create_page();
        }
        else
        {
            // Load root page from disk
            pages[0] = std::make_shared<Page>(get_page_from_disk(0));
        }
    }

    ~PageBuffer()
    {
        flush();
    }

    PageBuffer(const PageBuffer &) = delete;
    PageBuffer &operator=(const PageBuffer &) = delete;

    PageBuffer(PageBuffer &&) = delete;

    PageBuffer &operator=(PageBuffer &&other)
    {
        if (this == &other)
        {
            return *this;
        }

        // Swap members
        std::swap(header, other.header);
        std::swap(pages, other.pages);

        // Copy file contents from other's file to original file
        std::ifstream src(other.file_path, std::ios::binary);
        std::ofstream dst(file_path, std::ios::binary | std::ios::trunc);

        if (!src || !dst)
        {
            throw std::runtime_error("Failed to copy file contents");
        }

        dst << src.rdbuf();

        // Clear other's in-memory state but keep its file path
        other.pages.fill(nullptr);
        other.header = {};

        return *this;
    }

    Header &get_header()
    {
        return header;
    }

    PagePtr get_page(size_t index)
    {
        // If page is in buffer, return it
        size_t index_in_buffer = -1;
        for (size_t i = 0; i < pages.size(); ++i)
        {
            if (pages[i] && pages[i]->index == index)
            {
                return pages[i];
            }
            if (index_in_buffer == -1 && !pages[i])
            {
                index_in_buffer = i;
            }
        }

        // If page is not in buffer

        // Get page from disk
        auto page = get_page_from_disk(index);

        if (index_in_buffer != -1)
        {
            pages[index_in_buffer] = std::make_shared<Page>(page);
            return pages[index_in_buffer];
        }

        // If there is no space in buffer, evict a page
        if (index_in_buffer == -1)
        {
            size_t evicted_page_index = evict_page();
            pages[evicted_page_index] = std::make_shared<Page>(page);
            return pages[evicted_page_index];
        }

        throw std::runtime_error("No space to load a new page");
    }

    PagePtr create_page()
    {
        auto page = std::make_shared<Page>();
        page->index = header.number_of_pages;
        header.number_of_pages++;
        for (size_t i = 0; i < pages.size(); ++i)
        {
            if (!pages[i])
            {
                pages[i] = page;
                return page;
            }
        }

        // No space to create a new page, evict a page
        size_t evicted_page_index = evict_page();
        pages[evicted_page_index] = page;
        return page;
    }

    void flush()
    {
        file.write(reinterpret_cast<char *>(&header), sizeof(Header), 0);
        for (size_t i = 0; i < pages.size(); ++i)
        {
            if (pages[i])
            {
                write_page_to_disk(*pages[i]);
            }
        }
    }
};