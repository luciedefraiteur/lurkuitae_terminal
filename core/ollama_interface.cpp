#include "ollama_interface.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <fstream>
#include <regex>

// Fonction d’échappement JSON simple
std::string escape_json(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << c;
        }
    }
    return ss.str();
}

// Fonction pour retirer les délimiteurs <<< >>> autour de la commande
std::string OllamaInterface::extract_between_markers(const std::string& input) {
    std::regex marker_regex("```([\\s\\S]*?)```"); // [\s\S] pour inclure \n (pas de regex::dotall en C++)
    std::smatch match;
    if (std::regex_search(input, match, marker_regex)) {
        return match[1];
    }
    return input;
}

std::string OllamaInterface::query(const std::string& prompt) {
    std::string clean_prompt = escape_json(prompt);

    std::ostringstream command;
    command << "curl -s http://localhost:11434/api/generate "
            << "-H \"Content-Type: application/json\" "
            << "-d \"{\\\"model\\\":\\\"codellama:7b-instruct\\\",\\\"prompt\\\":\\\"" << clean_prompt << "\\\"}\"";

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
    return extract_between_markers(result);
}
