#pragma once

#include <fstream>

struct ScopedFile
{
    std::fstream file;

    ScopedFile(const std::string_view &path, bool truncate = false);
    ~ScopedFile();

    ScopedFile(const ScopedFile &) = delete;
    ScopedFile &operator=(const ScopedFile &) = delete;

    ScopedFile(ScopedFile &&) = delete;
    ScopedFile &operator=(ScopedFile &&) = delete;

    bool read(const void *data, size_t size, size_t offset = 0);
    bool write(const void *data, size_t size, size_t offset = 0);
    void flush();

private:
    void reset_pointers();
    void resize(size_t new_size);

    std::string path;
};
