#pragma once
#include "Engine.h"
#include <chrono>
#include <string>

constexpr int default_search_time = 5000;
constexpr int minimum_search_time = 30;

struct Flags {
	bool game_mode = false, show_help = false, quiet_mode = false;
	int fixed_depth = 0, search_time = default_search_time;
	std::string fen = "";
	int error_code = 0;
};

Flags process_flags(int argc, char* argv[]);

int game_mode(Engine& engine, Flags flags);
