#include <iostream>

#include "page_buffer.hpp"
struct Page
{
    size_t index;
    uint64_t data; // in our case PESEL
};

struct Header
{
    size_t number_of_pages;
};

void create_pages(PageBuffer<Page, Header> &page_buffer)
{
    page_buffer.create_page();
    page_buffer.create_page();
    auto page = page_buffer.create_page();
    page->data = 1234567890;
}

void load_pages(PageBuffer<Page, Header> &page_buffer)
{
    std::cout << "Loaded pages:" << std::endl;
    std::cout << page_buffer.get_header().number_of_pages << std::endl;
}

int main()
{
    PageBuffer<Page, Header> page_buffer("../data/test.txt");
    create_pages(page_buffer);
    // load_pages(page_buffer);
    for (size_t i = 0; i < page_buffer.get_header().number_of_pages; ++i)
    {
        auto page = page_buffer.get_page(i);
        std::cout << "Page index: " << page->index << " Page data: " << page->data << std::endl;
    }
    return 0;
}
