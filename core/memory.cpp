#include "memory.h"
#include <string>
#include <vector>

static std::vector<std::string> memory_log;

std::string Memory::get_context() {
    std::string context;
    for (const auto& entry : memory_log) context += entry + "\n";
    return context;
}

void Memory::append(const std::string& entry) {
    if (memory_log.size() > 10) memory_log.erase(memory_log.begin());
    memory_log.push_back(entry);
}
