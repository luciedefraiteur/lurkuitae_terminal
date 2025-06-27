// main.cpp
#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <regex>
#include "core/ollama_interface.h"
#include "core/memory.h"
#include "core/system_handler.h"

bool debug = false;
bool log_initialized = false;

void log_to_file(const std::string& content) {
    std::ios_base::openmode mode = std::ios::app;
    if (!log_initialized) {
        mode = std::ios::trunc;
        log_initialized = true;
    }
    std::ofstream logfile("lurkuitae_log.txt", mode);
    if (logfile.is_open()) {
        logfile << content << "\n";
        logfile.flush();
    }
}

void log_debug(const std::string& message) {
    if (debug == true) {
        std::cout << "[DEBUG] " << message << "\n";
        log_to_file("[DEBUG] " + message);
    }
}

std::string json_escape(const std::string& input) {
     std::ostringstream out;
    for (unsigned char c : input) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"':  out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '#':  out << "\\#"; break;  // Ajout spécial
            default:
                if (c < 32 || c > 126) continue;
                out << c;
        }
    }
    return out.str();
}

std::string remove_ansi_sequences(const std::string& input) {
    return std::regex_replace(input, std::regex("\\033\\[[0-9;]*m"), "");
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

        std::string validity_prompt = "tu es un terminal shell complexe et intelligent, tu vas recevoir une phrase et tu dois me dire si c'est une commande shell intelligente valide ou pas (exemple: 'affiche les fichiers de mon repertoire' doit aussi fonctionner). Réponds uniquement par 'oui' ou 'non'. Phrase : " + input;
        if (debug == true) log_debug("Envoi du prompt de validation : " + validity_prompt);
        std::string validity_response = safe_query(validity_prompt, "validité");

        std::transform(validity_response.begin(), validity_response.end(), validity_response.begin(), ::tolower);

        std::ostringstream log_stream;
        log_stream << "[LOG] Entrée utilisateur : " << input << "\n";

        if (validity_response.find("oui") != std::string::npos) {
            std::string guess_command_prompt = "" 
            "tu es un terminal shell complexe et intelligent, tu vas recevoir une phrase qui est une commande shell intelligente valide,"
            "mais elle est peut etre entourée de délimiteurs spécifiques, ou melangée entre code et language clair."
            "Tu dois deviner la commande shell standard ubuntu exacte à partir de cette phrase. Réponds uniquement par la commande shell sans rien d'autre,"
            " sans explications, sans guillemets ni crochets, juste la commande brute: \n ```" + input + "```";
            
            if (debug == true) log_debug("Envoi du prompt de devinette : " + guess_command_prompt);
            std::string guessed_command = safe_query(guess_command_prompt, "commande devinée");
            guessed_command = OllamaInterface::extract_between_markers(guessed_command);
            
            guessed_command.erase(std::remove(guessed_command.begin(), guessed_command.end(), '\n'), guessed_command.end());
            std::cout << "\nCommande devinée : " << guessed_command << std::endl;

            std::string system_output = handle_system_command(guessed_command);
            if (debug == true) log_debug("Résultat de la commande système :\n" + system_output);

            std::string view_check_prompt = "Est-ce que cette commande shell risque d'afficher du code source ou du contenu technique lisible par un humain ? Réponds uniquement par 'oui' ou 'non'. Commande : " + guessed_command;
            std::string is_code_output = safe_query(view_check_prompt, "nature sortie");

            bool likely_code = is_code_output.find("oui") != std::string::npos;
            std::string escaped_output = likely_code ? json_escape(system_output) : system_output;

            log_stream << "Commande exécutée : " << guessed_command << "\n";
            log_stream << "Sortie brute (échappée=" << (likely_code ? "oui" : "non") << ") :\n" << escaped_output << "\n";

            log_to_file(remove_ansi_sequences(log_stream.str()));

            std::string beautify_prompt = "Voici le résultat brut d'une commande shell Ubuntu" 
            + std::string(likely_code ? 
                " (échappé pour clarté), les ((( contenu... ))) délimitent le contenu " : "") 
                + " :\n(((" + 
                escaped_output 
                + 
                ")))\nPeux-tu simplement le reformuler de manière claire, concise et légèrement poétique si tu veux, sans exagérer, et expliquer son contenu ? la commande shell intelligente de base était: " + guessed_command;
            if (debug == true) log_debug("Envoi du prompt d'embellissement : " + beautify_prompt);
            std::string ai_response = safe_query(beautify_prompt, "embellissement");

            std::cout << "\nRéponse embellie :\n" << ai_response << std::endl;

            log_stream << "Réponse embellie : " << ai_response << "\n";
            Memory::append(remove_ansi_sequences(log_stream.str()));
            log_to_file(remove_ansi_sequences(log_stream.str()));
        } else {
            if (debug == true) log_debug("L’IA ne pense pas que ce soit une commande valide. Passage en réponse classique.");
            std::string context = Memory::get_context();
            std::string prompt = "Répond simplement à cette question, dans le contexte suivant :\n" + context + "\nNouvelle entrée : " + input;
            if (debug == true) log_debug("Envoi du prompt classique : " + prompt);

            std::string ai_response = safe_query(prompt, "réponse classique");
            std::cout << "\nRéponse :\n" << ai_response << std::endl;
            log_stream << "Réponse : " << ai_response << "\n";
            Memory::append(remove_ansi_sequences(log_stream.str()));
            log_to_file(remove_ansi_sequences(log_stream.str()));
        }
    }

    return 0;
}
