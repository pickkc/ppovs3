#include <iostream>
#include <Windows.h>
#include <filesystem>
#include <thread>
#include <vector>
#include <mutex>
#include <locale.h>
#include <fstream>   
#include <regex> 
namespace fs = std::filesystem;

std::mutex outputMutex;

struct FileData {
    std::string filename;
    std::vector<std::string> headers;
    std::vector<std::string> paragraphs;
    std::vector<std::string> orderedList;
};

void parseFile(const std::string& filename, FileData& data) {
    try {
        std::ifstream file(filename);
        std::string line;
        std::regex headerRegex(R"(^=+\s+(.*)$)");
        std::regex paragraphRegex(R"(^[^=\d].+$)");
        std::regex listItemRegex(R"(^\s*\d+\.\s+(.+)$)");

        while (std::getline(file, line)) {
            std::smatch match;

            if (std::regex_match(line, match, headerRegex)) {
                data.headers.push_back(match[1].str());
            }
            else if (std::regex_match(line, match, paragraphRegex)) {
                data.paragraphs.push_back(line);
            }
            else if (std::regex_match(line, match, listItemRegex)) {
                data.orderedList.push_back(match[1].str());
            }
        }

        file.close();
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка при парсинге файла " << filename << ": " << e.what() << std::endl;
    }
}

void displayFileElements(const FileData& data) {
    std::lock_guard<std::mutex> lock(outputMutex);

    std::cout << "Файл: " << data.filename << std::endl;

    std::cout << "Заголовки:" << std::endl;
    for (const auto& header : data.headers) {
        std::cout << header << std::endl;
    }

    std::cout << "\nАбзацы:" << std::endl;
    for (const auto& paragraph : data.paragraphs) {
        std::cout << paragraph << std::endl;
    }

    std::cout << "\nУпорядоченные списки:" << std::endl;
    for (const auto& item : data.orderedList) {
        std::cout << item << std::endl;
    }

    std::cout << "---------------------------------------------\n";
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Rus");

    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <путь_к_директории>" << std::endl;
        return 1;
    }

    std::string directoryPath = argv[1];

    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        std::cerr << "Указанный путь не является директорией или не существует.\n";
        return 1;
    }

    std::vector<std::string> filenames;
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (fs::is_regular_file(entry) && entry.path().extension() == ".adoc") {
            filenames.push_back(entry.path().string());
        }
    }

    std::sort(filenames.begin(), filenames.end());

    std::vector<FileData> filesData(filenames.size());

    std::vector<std::thread> threads;

    for (size_t i = 0; i < filenames.size(); ++i) {
        filesData[i].filename = filenames[i];

        threads.emplace_back([&, i]() {
            parseFile(filesData[i].filename, filesData[i]);
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    for (size_t i = 0; i < filesData.size(); ++i) {
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Поток #" << i + 1 << ": Файл " << filesData[i].filename << std::endl;
        }
        displayFileElements(filesData[i]);
    }

    return 0;
}

