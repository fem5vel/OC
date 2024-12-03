#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <algorithm>

class Page
{
public:
    int page_id;
    size_t offset_in_pool;
    size_t data_length;

    Page(int id, size_t offset, size_t length)
        : page_id(id), offset_in_pool(offset), data_length(length) {}
};

class Segment
{
public:
    std::unordered_map<int, Page> pages;
};

struct FreeBlock
{
    size_t offset;
    size_t length;

    FreeBlock(size_t offset, size_t length) : offset(offset), length(length) {}
};

class MemoryManager
{
private:
    Segment segment;
    std::vector<char> memory_pool;
    std::list<FreeBlock> free_blocks;
    size_t pool_offset = 0;
    int next_page_id = 0;

public:
    MemoryManager(const std::vector<std::string> &words);
    bool write_data(const std::string &word);
    bool delete_page(int page_id);
    void print_memory();
};

MemoryManager::MemoryManager(const std::vector<std::string> &words)
{
    size_t total_word_size = 0;
    for (const auto &word : words)
    {
        total_word_size += word.size();
    }
    memory_pool.reserve(total_word_size * 2);
    for (const auto &word : words)
    {
        write_data(word);
    }
}

bool MemoryManager::write_data(const std::string &word)
{
    size_t word_size = word.size();
    size_t write_offset = pool_offset;

    auto free_it = std::find_if(free_blocks.begin(), free_blocks.end(),
                                [word_size](const FreeBlock &block)
                                { return block.length >= word_size; });

    if (free_it != free_blocks.end())
    {
        write_offset = free_it->offset;
        if (free_it->length > word_size)
        {
            free_it->offset += word_size;
            free_it->length -= word_size;
        }
        else
        {
            free_blocks.erase(free_it);
        }
    }
    else
    {
        pool_offset += word_size;
        if (pool_offset > memory_pool.capacity())
        {
            memory_pool.resize(pool_offset);
        }
    }

    if (write_offset + word_size > memory_pool.size())
    {
        memory_pool.resize(write_offset + word_size);
    }
    std::copy(word.begin(), word.end(), memory_pool.begin() + write_offset);

    int current_page_id = next_page_id++;
    segment.pages.emplace(current_page_id, Page(current_page_id, write_offset, word_size));
    return true;
}

bool MemoryManager::delete_page(int page_id)
{
    auto it = segment.pages.find(page_id);
    if (it == segment.pages.end())
    {
        std::cerr << "Страница с ID " << page_id << " не найдена.\n";
        return false;
    }

    size_t deleted_offset = it->second.offset_in_pool;
    size_t deleted_length = it->second.data_length;
    std::fill(memory_pool.begin() + deleted_offset, memory_pool.begin() + deleted_offset + deleted_length, 0);
    free_blocks.emplace_back(deleted_offset, deleted_length);
    segment.pages.erase(it);
    return true;
}

void MemoryManager::print_memory()
{
    std::cout << "Общий фрагмент данных:\n";
    for (char c : memory_pool)
    {
        std::cout << c;
    }
    std::cout << "\n----------------------------------------\n";
    std::cout << "Сегмент:\n";
    for (auto it = segment.pages.begin(); it != segment.pages.end(); ++it)
    {
        int id = it->first;
        const Page &page = it->second;
        std::cout << "  Страница ID: " << id << " | Смещение: " << page.offset_in_pool
                  << " | Данные: " << std::string(memory_pool.begin() + page.offset_in_pool, memory_pool.begin() + page.offset_in_pool + page.data_length) << "\n";
    }
}

int main()
{
    std::vector<std::string> words = {
        "Пример", "данных", "разной", "длины", "для",
        "демонстрации", "работы", "системы", "управления",
        "памятью", "и", "разделения", "на", "страницы", "и",
        "сегменты", "оченьдлинноесловокотороепревышаетразмерсегмента"};

    MemoryManager manager(words);

    std::cout << "\nСодержимое памяти до удаления:\n";
    manager.print_memory();

    std::cout << "\nУдаление страницы с ID 3:\n";
    manager.delete_page(3);

    std::cout << "\nЗапись нового слова 'новое':\n";
    manager.write_data("новое");

    std::cout << "\nСодержимое памяти после записи нового слова:\n";
    manager.print_memory();

    return 0;
}
