// main.cpp
#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <regex>
#include <fstream>
#include "core/ollama_interface.h"
#include "core/memory.h"
#include "core/system_handler.h"

bool debug = false;
bool log_initialized = false;

void log_debug(const std::string& message) {
    if (debug == true) {
        std::cout << "[DEBUG] " << message << "\n";
    }
}

void log_to_file(const std::string& content) {
    std::ios_base::openmode mode = std::ios::app;
    if (!log_initialized) {
        mode = std::ios::trunc;
        log_initialized = true;
    }
    std::ofstream logfile("lurkuitae_log.txt", mode);
    if (logfile.is_open()) {
        logfile << content << "\n";
        logfile.flush(); // Ensure write is immediate
    }
}

std::string json_escape(const std::string& input) {
    std::ostringstream escaped;
    for (char c : input) {
        switch (c) {
            case '\"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
                    escaped << "\\u"
                            << std::hex << std::setw(4) << std::setfill('0') << (int)(unsigned char)c;
                } else {
                    escaped << c;
                }
        }
    }
    return escaped.str();
}

std::string escape_for_prompt(const std::string& input) {
    std::string output = input;

    try {
        // Ordre important pour éviter double échappement
        output = std::regex_replace(output, std::regex(R"(\\)"), "\\\\");
        output = std::regex_replace(output, std::regex("\""), "\\\"");
        output = std::regex_replace(output, std::regex("\n"), "\\n");
        output = std::regex_replace(output, std::regex("\r"), "\\r");
        output = std::regex_replace(output, std::regex("\t"), "\\t");
    } catch (const std::regex_error& e) {
        return "[Erreur échappement : regex invalide]";
    }

    std::ostringstream sanitized;
    for (unsigned char c : output) {
        if (c < 32 || c > 126) {
            sanitized << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        } else {
            sanitized << c;
        }
    }

    return sanitized.str();
}

std::string safe_query(const std::string& prompt, const std::string& label) {
    std::string response;
    int attempts = 0;
    while (response.empty() && attempts < 3) {
        response = OllamaInterface::query(prompt);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        attempts++;
        if (debug == true) log_debug("Tentative " + std::to_string(attempts) + " - " + label + " : " + response);
    }
    if (response.empty()) response = "[Erreur : réponse vide]";
    return response;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--debug" || arg == "-d") {
            debug = true;
        }
    }

    std::cout << "∴ LURKUITAE ∴ Terminal Codex Vivant ∴ (LLM Local + Mémoire + Shell + Interprétation";
    if (debug == true) std::cout << " + DEBUG";
    std::cout << ")\n";

    std::string input;
    while (true) {
        std::cout << "\nPose ta question ou commande (ou tape 'exit'): ";
        std::getline(std::cin, input);
        if (input == "exit") break;

        std::string validity_prompt = "Est-ce que cette phrase peut être interprétée/traduite comme une commande shell Ubuntu valide ? Réponds uniquement par 'oui' ou 'non' : " + input;
        if (debug == true) log_debug("Envoi du prompt de validation : " + validity_prompt);
        std::string validity_response = safe_query(validity_prompt, "validité");

        std::transform(validity_response.begin(), validity_response.end(), validity_response.begin(), ::tolower);

        std::ostringstream log_stream;
        log_stream << "[LOG] Entrée utilisateur : " << input << "\n";

        if (validity_response.find("oui") != std::string::npos) {
            std::string guess_command_prompt = "Traduis la phrase suivante en une commande shell Ubuntu exécutable, sans guillemets ni backticks, juste la commande brute. Phrase : " + input;
            if (debug == true) log_debug("Envoi du prompt de devinette : " + guess_command_prompt);
            std::string guessed_command = safe_query(guess_command_prompt, "commande devinée");

            guessed_command.erase(std::remove(guessed_command.begin(), guessed_command.end(), '\n'), guessed_command.end());
            std::string system_output = handle_system_command(guessed_command);
            if (debug == true) log_debug("Résultat de la commande système :\n" + system_output);
            std::cout << "\nRésultat de la commande :\n" << system_output << std::endl;

            bool is_view_command = guessed_command.find("cat ") == 0 || guessed_command.find("less ") == 0;
            std::string escaped_output = is_view_command ? json_escape(system_output) : system_output;

            std::string beautify_prompt;
            if (is_view_command && (guessed_command.find(".cpp") != std::string::npos || guessed_command.find(".h") != std::string::npos)) {
                beautify_prompt = "Voici un fichier source C++ :\n" + escaped_output + "\nPeux-tu le reformuler comme un résumé clair de ce qu’il fait, sans le modifier ?";
            } else {
                beautify_prompt = "Voici le résultat brut d'une commande shell Ubuntu" + std::string(is_view_command ? " (échappé pour clarté)" : "") + " :\n" + escaped_output + "\nPeux-tu simplement le reformuler de manière claire, concise et légèrement poétique si tu veux, sans exagérer ?";
            }

            if (debug == true) log_debug("Envoi du prompt d'embellissement : " + beautify_prompt);
            std::string ai_response = safe_query(beautify_prompt, "embellissement");

            std::cout << "\nRéponse embellie :\n" << ai_response << std::endl;
            log_stream << "Commande exécutée : " << guessed_command << "\nSortie : " << system_output << "\nRéponse : " << ai_response << "\n";
            Memory::append(log_stream.str());
        } else {
            if (debug == true) log_debug("L’IA ne pense pas que ce soit une commande valide. Passage en réponse classique.");
            std::string context = Memory::get_context();
            std::string prompt = "Répond simplement à cette question, dans le contexte suivant :\n" + context + "\nNouvelle entrée : " + input;
            if (debug == true) log_debug("Envoi du prompt classique : " + prompt);

            std::string ai_response = safe_query(prompt, "réponse classique");
            std::cout << "\nRéponse :\n" << ai_response << std::endl;
            log_stream << "Réponse : " << ai_response << "\n";
            Memory::append(log_stream.str());
        }

        log_to_file(log_stream.str());
    }

    return 0;
}
