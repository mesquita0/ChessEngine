#pragma once
#include <filesystem>
#include <fstream>
#include <string>

template <typename T>
inline bool loadFromFile(T* arr, int size, std::filesystem::path file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) return false;
    file.read((char*) arr, sizeof(T) * size);
    return true;
}
