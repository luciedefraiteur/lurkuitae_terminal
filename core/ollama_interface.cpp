#include "ollama_interface.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <fstream>

// Fonction d’échappement JSON simple
std::string escape_json(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '\"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\n': ss << "\\n"; break;
            default: ss << c;
        }
    }
    return ss.str();
}

std::string OllamaInterface::query(const std::string& prompt) {
    std::string clean_prompt = escape_json(prompt);

    std::ostringstream command;
    command << "curl -s http://localhost:11434/api/generate "
            << "-H \"Content-Type: application/json\" "
            << "-d \"{\\\"model\\\":\\\"llama3\\\",\\\"prompt\\\":\\\"" << clean_prompt << "\\\"}\"";

    FILE* pipe = popen(command.str().c_str(), "r");
    if (!pipe) return "[Erreur: impossible d'exécuter la commande]";

    char buffer[4096];
    std::ostringstream fullResponse;

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        try {
            auto json_line = nlohmann::json::parse(buffer);
            if (json_line.contains("response")) {
                fullResponse << json_line["response"].get<std::string>();
            }
        } catch (...) {
            // erreurs de parsing ignorées
        }
    }

    pclose(pipe);
    std::string result = fullResponse.str();
    if (result.empty()) return "[Erreur : réponse vide]";
    return result;
}
