#ifndef OLLAMA_INTERFACE_H
#define OLLAMA_INTERFACE_H
#include <string>
namespace OllamaInterface {
    std::string query(const std::string& prompt);
}
#endif
