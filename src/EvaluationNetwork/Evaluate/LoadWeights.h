#pragma once
#include <filesystem>
#include <fstream>
#include <string>

template <typename T>
inline void loadFromFile(T* arr, int size, std::filesystem::path file_path) {
    std::ifstream file(file_path, std::ios::binary);
    file.read((char*) arr, sizeof(T) * size);
    file.close();
}
