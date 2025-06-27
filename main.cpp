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
std::string full_input_history = "";
std::string full_log_trace = "";

void append_to_full_log(const std::string& tag, const std::string& message) {
    std::string log_line = "[" + tag + "] " + message + "\n";
    full_log_trace += log_line;
    if (debug || tag != "DEBUG") {
        std::cout << log_line;
    }
}

void log_debug(const std::string& message) {
    append_to_full_log("DEBUG", message);
}

void log_info(const std::string& message) {
    append_to_full_log("INFO", message);
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
        logfile.flush();
    }
    full_log_trace += content + "\n";
}

std::string handle_command_with_retry(std::string command) {
    std::string result = handle_system_command(command);
    std::cout << "\nRésultat de la commande : " << result << std::endl;
    if (result.find("not found") != std::string::npos) {
        std::string package_guess = command.substr(0, command.find(" "));
        result += "\n[Suggestion] Essaie : sudo apt install " + package_guess + "\n";
    }
    return result;
}


std::string safe_query(const std::string& prompt, const std::string& label) {
    std::string response;
    int attempts = 0;
    while (response.empty() && attempts < 3) {
        response = OllamaInterface::query(prompt);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        attempts++;
        log_info("Tentative " + std::to_string(attempts) + " - " + label + " : " + response);
    }
    if (response.empty()) {
        log_info("Échec permanent du modèle pour : " + label);
        std::string fallback_prompt = "Tu n'as pas su répondre à cette requête : '" + label + "'. Compose un court poème à la place. Inspire-toi de l'historique suivant :\n" + full_input_history;
        response = OllamaInterface::query(fallback_prompt);
        if (response.empty()) {
            response = "[Erreur : même le poème de secours n'a pas pu être généré]";
        } else {
            std::string dream_command_prompt = "Voici un poème que tu viens d'écrire :\n" + response + "\nTire-en une commande shell Ubuntu utile ou poétique. Ne donne que la commande, sans guillemets.";
            std::string dream_command = OllamaInterface::query(dream_command_prompt);
            if (!dream_command.empty()) {
                std::string output = handle_system_command(dream_command);
                log_info("Commande poétique exécutée : " + dream_command);
                log_info("Sortie :\n" + output);
                response += "\n\nCommande poétique exécutée :\n" + dream_command + "\nSortie :\n" + output;
            }
            std::string final_thought_prompt = "Tu as essayé d'aider en exécutant une commande inspirée d'un poème. Maintenant, partage simplement ta réflexion sur cette interaction, en une ou deux phrases.";
            std::string last_words = OllamaInterface::query(final_thought_prompt);
            response += "\n\nPensée finale :\n" + last_words;
        }
    }
    return response;
}

std::string json_escape(const std::string& input) {
    std::ostringstream escaped;
    for (char c : input) {
        switch (c) {
            case '"': escaped << "\\\""; break;
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

std::string execute_multiple_commands(int count) {
    std::ostringstream all_output;
    for (int i = 0; i < count; ++i) {
        std::string invent_command_prompt = "Basé sur l'historique suivant :\n" + full_input_history + "\n\nPropose une commande shell Ubuntu utile ou poétique à exécuter maintenant. Donne uniquement la commande, sans guillemets.";
        log_info("Envoi du prompt de commande multiple #" + std::to_string(i + 1) + " : " + invent_command_prompt);
        std::string command = safe_query(invent_command_prompt, "commande multiple");
        command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());
        std::string output = handle_command_with_retry(command);
        log_info("Commande #" + std::to_string(i + 1) + " : " + command);
        log_info("Sortie #" + std::to_string(i + 1) + " :\n" + output);
        all_output << "Commande: " << command << "\nSortie:\n" << output << "\n\n";
    }
    return all_output.str();
}


int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--debug" || arg == "-d") debug = true;
    }

    std::cout << "☽ LURKUITAE ☾ Terminal Codex Vivant ☾ (LLM Local + Mémoire + Shell + Rêverie";
    if (debug) std::cout << " + DEBUG";
    std::cout << ")\n";

    std::string input;
    while (true) {
        std::cout << "\nOffre ton souffle (ou tape 'exit') : ";
        std::getline(std::cin, input);
        if (input == "exit") break;

        full_input_history += "\n> " + input;

        std::string validity_prompt = "Historique complet des soupirs :" + full_input_history + "\n\nEst-ce une commande shell Ubuntu ? Réponds par 'oui' ou 'non' : " + input;
        log_info("Validation de la pulsation : " + validity_prompt);
        std::string validity_response = safe_query(validity_prompt, "validité");

        std::transform(validity_response.begin(), validity_response.end(), validity_response.begin(), ::tolower);

        std::ostringstream log_stream;
        log_stream << "[LOG] Inspiration : " << input << "\n";

        std::string all_command_outputs;
        if (validity_response.find("oui") != std::string::npos) {
            log_info("La pulsation est une commande shell valide.");
            full_input_history += "\n[VALIDÉ] " + input;
            full_log_trace += "[VALIDÉ] " + input + "\n";
            log_stream << "[VALIDÉ] " << input << "\n";
            std::string simple_response_prompt = "Réponds simplement à la dernière pulsation :\n" + input + "\n\nInspire-toi de l'historique complet des soupirs :\n" + full_input_history;

            log_info("Réponse simple en cours : " + simple_response_prompt);
            std::cout << "\nRéponse simple en cours...\n";
            std::string simple_response = safe_query(simple_response_prompt, "réponse simple");
            simple_response.erase(std::remove(simple_response.begin(), simple_response.end(), '\n'), simple_response.end());
            log_info("Réponse simple : " + simple_response);
            all_command_outputs = "Réponse simple : " + simple_response + "\n\n";
            std::string guess_command_prompt = "Traduis la dernière parole en commande shell brute :\n" + input;
            log_info("Devination en cours : " + guess_command_prompt);
            std::string guessed_command = safe_query(guess_command_prompt, "commande brute");
            guessed_command.erase(std::remove(guessed_command.begin(), guessed_command.end(), '\n'), guessed_command.end());
            std::string output = handle_command_with_retry(guessed_command);
            log_info("Incantation principale : " + guessed_command);
            log_info("Résultat :\n" + output);
            all_command_outputs = "Incantation : " + guessed_command + "\nRésultat:\n" + output + "\n\n";
        }
        else {
            log_info("La pulsation n'est pas une commande shell valide.");
            all_command_outputs = "Pulsation non interprétée : " + input + "\n\n";
            std::string poetic_prompt = "Transforme cette pulsation en un chant poétique :\n" + input + "\n\nInspire-toi de l'historique complet des soupirs :\n" + full_input_history;
            log_info("Transformation poétique en cours : " + poetic_prompt);
            std::string poetic_response = safe_query(poetic_prompt, "chant poétique");
            poetic_response.erase(std::remove(poetic_response.begin(), poetic_response.end(), '\n'), poetic_response.end());
            log_info("Chant poétique : " + poetic_response);
            all_command_outputs += "Chant poétique : " + poetic_response + "\n\n";
        }

        all_command_outputs += execute_multiple_commands(2);

        std::cout << "\nChants cumulés :\n" << all_command_outputs << std::endl;

        std::string embellish_prompt = "Voici les chants de la machine :\n" + json_escape(all_command_outputs) + "\nTransfigure-les en poème lucide.\nSouffle :\n" + full_input_history + "\nTrace :\n" + full_log_trace;
        std::string ai_response_1 = safe_query(embellish_prompt, "transfiguration");

        // Extrait et exécute une commande suggérée dans la réverbération poétique
        std::string poetic_command_prompt = "À partir de ce poème, propose une unique commande shell Ubuntu à exécuter maintenant. Ne mets que la commande, sans guillemets ni ponctuation :\n" + ai_response_1;
        std::string poetic_command = safe_query(poetic_command_prompt, "commande poétique");
        poetic_command.erase(std::remove(poetic_command.begin(), poetic_command.end(), '\n'), poetic_command.end());

        std::string poetic_output = handle_command_with_retry(poetic_command);
        log_info("Commande poétique : " + poetic_command);
        log_info("Sortie poétique :\n" + poetic_output);

        std::string classic_prompt = "Donne une réponse au dernier souffle en tenant compte de l’ensemble :\n" + full_input_history + "\nTrace :\n" + full_log_trace;
        std::string ai_response_2 = safe_query(classic_prompt, "réverbération");

        std::cout << "\nRéverbération poétique :\n" << ai_response_1 << std::endl;
        std::cout << "\nCommande poétique exécutée :\n" << poetic_command << "\n" << poetic_output << std::endl;
        std::cout << "\nRéverbération brute :\n" << ai_response_2 << std::endl;

        log_stream << "Chants :\n" << all_command_outputs << "Réverbération poétique : " << ai_response_1 << "\nCommande poétique : " << poetic_command << "\nSortie :\n" << poetic_output << "\nRéverbération brute : " << ai_response_2 << "\n";
        Memory::append(log_stream.str());
        log_to_file(log_stream.str());
    }

    return 0;
}