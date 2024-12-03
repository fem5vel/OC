#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "build/libs/json.hpp"
using json = nlohmann::json;

const size_t BLOCK_SIZE = 32;

struct Block
{
    size_t freeSpace;
    int nextBlock;
    std::string data;

    Block() : freeSpace(BLOCK_SIZE), nextBlock(-1), data("") {}

    json toJSON() const
    {
        return {
            {"freeSpace", freeSpace},
            {"nextBlock", nextBlock},
            {"data", data}};
    }

    static Block fromJSON(const json &j)
    {
        Block block;
        block.freeSpace = j.at("freeSpace").get<size_t>();
        block.nextBlock = j.at("nextBlock").get<int>();
        block.data = j.at("data").get<std::string>();
        return block;
    }
};

struct FATEntry
{
    int blockNumber;
    std::string fileName;
    size_t fileSize;
    time_t creationDate;
    int nextBlock;

    FATEntry()
        : blockNumber(-1), fileName(""), fileSize(0), creationDate(0), nextBlock(-1) {}

    FATEntry(int blockNum, const std::string &fName, size_t fSize, time_t cDate, int next)
        : blockNumber(blockNum), fileName(fName), fileSize(fSize), creationDate(cDate), nextBlock(next) {}

    json toJSON() const
    {
        return {
            {"blockNumber", blockNumber},
            {"fileName", fileName},
            {"fileSize", fileSize},
            {"creationDate", creationDate},
            {"nextBlock", nextBlock}};
    }

    static FATEntry fromJSON(const json &j)
    {
        FATEntry entry;
        entry.blockNumber = j.at("blockNumber").get<int>();
        entry.fileName = j.at("fileName").get<std::string>();
        entry.fileSize = j.at("fileSize").get<size_t>();
        entry.creationDate = j.at("creationDate").get<time_t>();
        entry.nextBlock = j.at("nextBlock").get<int>();
        return entry;
    }
};

class File
{
public:
    std::string name;
    size_t size;

    File() : name(""), size(0) {}
    File(const std::string &name) : name(name), size(0) {}

    void writeData(const std::string &data, std::vector<Block> &blocks, std::vector<FATEntry> &fat)
    {
        size_t dataSize = data.size();
        int previousBlock = -1;
        int freeBlock = -1;

        for (int i = 0; i < blocks.size(); ++i)
        {
            if (blocks[i].freeSpace == BLOCK_SIZE)
            {
                freeBlock = i;
                break;
            }
        }

        if (freeBlock == -1)
        {
            blocks.push_back(Block());
            freeBlock = blocks.size() - 1;
        }

        while (dataSize > 0)
        {
            Block &newBlock = blocks[freeBlock];
            size_t toWrite = std::min(dataSize, newBlock.freeSpace);
            newBlock.data = data.substr(data.size() - dataSize, toWrite);
            newBlock.freeSpace -= toWrite;
            dataSize -= toWrite;

            FATEntry entry(freeBlock, name, size, std::time(nullptr), -1);
            if (previousBlock != -1)
            {
                blocks[previousBlock].nextBlock = freeBlock;
                fat[previousBlock].nextBlock = freeBlock;
            }
            fat.push_back(entry);
            previousBlock = freeBlock;

            size += toWrite;

            freeBlock = -1;
            for (int i = 0; i < blocks.size(); ++i)
            {
                if (blocks[i].freeSpace == BLOCK_SIZE)
                {
                    freeBlock = i;
                    break;
                }
            }

            if (freeBlock == -1)
            {
                blocks.push_back(Block());
                freeBlock = blocks.size() - 1;
            }
        }
    }

    std::string readData(const std::vector<Block> &blocks, const std::vector<FATEntry> &fat) const
    {
        std::string result;
        int currentBlock = -1;

        for (const auto &entry : fat)
        {
            if (entry.fileName == name)
            {
                currentBlock = entry.blockNumber;
                break;
            }
        }

        if (currentBlock == -1)
        {
            std::cerr << "Файл " << name << " не найден в FAT!" << std::endl;
            return "";
        }

        while (currentBlock != -1)
        {
            result += blocks[currentBlock].data;
            currentBlock = blocks[currentBlock].nextBlock;
        }

        return result;
    }
};

class Directory
{
public:
    std::string name;
    std::map<std::string, File> files;
    std::map<std::string, Directory> subdirectories;
    Directory *parent;

    Directory() : name("root"), parent(nullptr) {}
    Directory(const std::string &name, Directory *parent = nullptr)
        : name(name), parent(parent) {}

    void createFile(const std::string &fileName)
    {
        if (files.find(fileName) != files.end())
        {
            std::cerr << "Файл с именем " << fileName << " уже существует!" << std::endl;
            return;
        }
        files[fileName] = File(fileName);
    }

    void createDirectory(const std::string &dirName)
    {
        if (subdirectories.find(dirName) != subdirectories.end())
        {
            std::cerr << "Директория с именем " << dirName << " уже существует!" << std::endl;
            return;
        }
        subdirectories[dirName] = Directory(dirName, this);
    }
    void deleteFile(const std::string &fileName, std::vector<FATEntry> &fat, std::vector<Block> &blocks)
    {
        auto it = files.find(fileName);
        if (it == files.end())
        {
            std::cerr << "Файл " << fileName << " не найден!" << std::endl;
            return;
        }

        int currentBlock = -1;
        for (auto &entry : fat)
        {
            if (entry.fileName == fileName)
            {
                currentBlock = entry.blockNumber;
                entry.fileName = "";
                entry.fileSize = 0;
                entry.nextBlock = -1;
            }
        }

        while (currentBlock != -1)
        {
            int nextBlock = blocks[currentBlock].nextBlock;
            blocks[currentBlock].data = "";
            blocks[currentBlock].freeSpace = BLOCK_SIZE;
            blocks[currentBlock].nextBlock = -1;
            currentBlock = nextBlock;
        }

        files.erase(it);
        std::cout << "Файл " << fileName << " успешно удалён." << std::endl;
    }

    void deleteDirectory(const std::string &dirName, std::vector<FATEntry> &fat, std::vector<Block> &blocks)
    {
        auto it = subdirectories.find(dirName);
        if (it == subdirectories.end())
        {
            std::cerr << "Директория " << dirName << " не найдена!" << std::endl;
            return;
        }

        it->second.deleteAll(fat, blocks);

        subdirectories.erase(it);

        std::cout << "Директория " << dirName << " успешно удалена." << std::endl;
    }

    void deleteAll(std::vector<FATEntry> &fat, std::vector<Block> &blocks)
    {
        for (auto it = files.begin(); it != files.end();)
        {
            deleteFile(it->first, fat, blocks);
            it = files.begin();
        }

        for (auto it = subdirectories.begin(); it != subdirectories.end();)
        {
            it->second.deleteAll(fat, blocks);
            it = subdirectories.erase(it);
        }

        std::cout << "Содержимое директории " << name << " успешно удалено." << std::endl;
    }

    void copyFile(const std::string &fileName, Directory &destination)
    {
        auto it = files.find(fileName);
        if (it != files.end())
        {
            destination.files[fileName] = it->second;
        }
        else
        {
            std::cerr << "Файл " << fileName << " не найден!" << std::endl;
        }
    }

    json toJSON() const
    {
        json j;
        j["name"] = name;

        for (const auto &[fileName, file] : files)
        {
            j["files"].push_back({{"name", file.name}, {"size", file.size}});
        }

        for (const auto &[dirName, dir] : subdirectories)
        {
            j["subdirectories"].push_back(dir.toJSON());
        }

        return j;
    }

    static Directory fromJSON(const json &j, Directory *parent = nullptr)
    {
        Directory dir(j.at("name").get<std::string>(), parent);

        if (j.contains("files"))
        {
            for (const auto &fileJSON : j.at("files"))
            {
                File file(fileJSON.at("name").get<std::string>());
                file.size = fileJSON.at("size").get<size_t>();
                dir.files[file.name] = file;
            }
        }

        if (j.contains("subdirectories"))
        {
            for (const auto &subDirJSON : j.at("subdirectories"))
            {
                dir.subdirectories[subDirJSON.at("name").get<std::string>()] = Directory::fromJSON(subDirJSON, &dir);
            }
        }

        return dir;
    }
};

void printFAT(const std::vector<FATEntry> &fat, const std::vector<Block> &blocks)
{
    std::cout << "Содержимое FAT таблицы:\n";
    std::cout << "Блок\tФайл\t\tЗанятый объем\tДата\t\t\tСледующий блок\n";

    for (const auto &entry : fat)
    {
        if (!entry.fileName.empty())
        {
            int usedSpace = BLOCK_SIZE - blocks[entry.blockNumber].freeSpace;
            std::cout << entry.blockNumber << "\t"
                      << entry.fileName << "\t\t"
                      << usedSpace << "\t\t"
                      << std::asctime(std::localtime(&entry.creationDate))
                      << "\t" << entry.nextBlock << "\n";
        }
    }
}
void saveFileSystem(const Directory &root, const std::vector<Block> &blocks, const std::vector<FATEntry> &fat)
{
    json fsJSON;

    fsJSON["root"] = root.toJSON();

    for (const auto &block : blocks)
    {
        fsJSON["blocks"].push_back(block.toJSON());
    }

    for (const auto &entry : fat)
    {
        fsJSON["fat"].push_back(entry.toJSON());
    }

    std::ofstream outFile("filesystem.json");
    outFile << fsJSON.dump(4);
    outFile.close();
}

void loadFileSystem(Directory &root, std::vector<Block> &blocks, std::vector<FATEntry> &fat)
{
    std::ifstream inFile("filesystem.json");
    if (!inFile.is_open())
    {
        std::cerr << "Не удалось открыть файл файловой системы!" << std::endl;
        return;
    }

    json fsJSON;
    inFile >> fsJSON;
    inFile.close();

    root = Directory::fromJSON(fsJSON["root"]);

    blocks.clear();
    for (const auto &blockJSON : fsJSON["blocks"])
    {
        blocks.push_back(Block::fromJSON(blockJSON));
    }

    fat.clear();
    for (const auto &fatJSON : fsJSON["fat"])
    {
        fat.push_back(FATEntry::fromJSON(fatJSON));
    }
}

void runFileSystem()
{
    Directory root;
    Directory *currentDir = &root;
    std::vector<Block> blocks;
    std::vector<FATEntry> fat;

    loadFileSystem(root, blocks, fat);

    std::string command;
    while (true)
    {
        std::cout << currentDir->name << "> ";
        std::getline(std::cin, command);

        std::istringstream iss(command);
        std::string cmd, arg1, arg2;
        iss >> cmd >> arg1;
        std::getline(iss, arg2);
        arg2.erase(0, arg2.find_first_not_of(" "));

        if (cmd == "exit")
        {
            saveFileSystem(root, blocks, fat);
            break;
        }
        else if (cmd == "ls")
        {
            for (const auto &[name, dir] : currentDir->subdirectories)
                std::cout << name << "/\n";
            for (const auto &[name, file] : currentDir->files)
                std::cout << name << "\n";
        }
        else if (cmd == "cd")
        {
            if (arg1 == "..")
            {
                if (currentDir->parent)
                    currentDir = currentDir->parent;
                else
                    std::cerr << "Вы уже в корневой директории!" << std::endl;
            }
            else if (currentDir->subdirectories.find(arg1) != currentDir->subdirectories.end())
            {
                currentDir = &currentDir->subdirectories[arg1];
            }
            else
            {
                std::cerr << "Директория " << arg1 << " не найдена!" << std::endl;
            }
        }
        else if (cmd == "mkdir")
        {
            currentDir->createDirectory(arg1);
        }
        else if (cmd == "touch")
        {
            currentDir->createFile(arg1);
        }
        else if (cmd == "write")
        {
            if (currentDir->files.find(arg1) != currentDir->files.end())
            {
                currentDir->files[arg1].writeData(arg2, blocks, fat);
            }
            else
            {
                std::cerr << "Файл " << arg1 << " не найден!" << std::endl;
            }
        }
        else if (cmd == "read")
        {
            if (currentDir->files.find(arg1) != currentDir->files.end())
            {
                std::string fileData = currentDir->files[arg1].readData(blocks, fat);
                std::cout << fileData << std::endl;
            }
            else
            {
                std::cerr << "Файл " << arg1 << " не найден!" << std::endl;
            }
        }
        else if (cmd == "rm")
        {
            currentDir->deleteFile(arg1, fat, blocks);
        }
        else if (cmd == "rmdir")
        {
            currentDir->deleteDirectory(arg1, fat, blocks);
        }
        else if (cmd == "cp")
        {
            auto it = currentDir->subdirectories.find(arg2);
            if (it != currentDir->subdirectories.end())
            {
                currentDir->copyFile(arg1, it->second);
            }
            else
            {
                std::cerr << "Директория назначения не найдена!" << std::endl;
            }
        }
        else if (cmd == "fat")
        {
            printFAT(fat, blocks);
        }
        else
        {
            std::cerr << "Неизвестная команда: " << cmd << std::endl;
        }
    }
}

int main()
{
    runFileSystem();
    return 0;
}