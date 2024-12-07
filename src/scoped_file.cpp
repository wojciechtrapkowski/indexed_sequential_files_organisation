#include "scoped_file.hpp"
#include "debug.hpp"

#include <iostream>
#include <filesystem>
#include <cstring>

ScopedFile::ScopedFile(const std::string_view &path)
    : path(path)
{
    // First try to create directory if it doesn't exist
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());

    if (!std::filesystem::exists(path))
    {
        // Create new file
        file.open(path.data(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to create file: " + std::string(path.data()));
        }
    }
    else
    {
        // Open existing file
        file.open(path.data(), std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + std::string(path.data()));
        }
    }
    reset_pointers();
}

ScopedFile::~ScopedFile()
{
    flush();
    file.close();
}

bool ScopedFile::read(const void *data, size_t size, size_t offset)
{
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();

    if (fileSize <= 0 || fileSize < offset + size)
    {
        DEBUG_CERR << "File is empty or too small to read " << size << " bytes at offset " << offset << std::endl;
        return false;
    }

    file.seekg(offset);
    char *char_data = static_cast<char *>(const_cast<void *>(data));
    file.read(char_data, size); // Read directly into the buffer

    return true;
}

bool ScopedFile::write(const void *data, size_t size, size_t offset)
{
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();

    // If file is too small, just return
    if (fileSize < offset + size)
    {
        resize(offset + size);
    }
    const char *char_data = static_cast<const char *>(data);

    file.seekp(offset);
    file.write(char_data, size);
    file.flush();
    return true;
}

void ScopedFile::flush()
{
    file.flush();
}

void ScopedFile::reset_pointers()
{
    file.seekg(0, std::ios::beg);
    file.seekp(0, std::ios::beg);

    file.clear();
}

void ScopedFile::resize(size_t new_size)
{
    // Get current file size
    file.seekg(0, std::ios::end);
    size_t current_size = file.tellg();

    if (new_size > current_size)
    {
        // Need to extend the file
        file.seekp(0, std::ios::end);

        // Write zeros to extend the file
        char zero_buffer[4096] = {0}; // Use 4KB chunks for efficiency

        while (current_size < new_size)
        {
            size_t bytes_to_write = std::min(sizeof(zero_buffer),
                                             new_size - current_size);
            file.write(zero_buffer, bytes_to_write);
            if (file.fail())
            {
                throw std::runtime_error("Failed to extend file");
            }
            current_size += bytes_to_write;
        }
    }

    // Reset pointers after resize
    reset_pointers();
}
