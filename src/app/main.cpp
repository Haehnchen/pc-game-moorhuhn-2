#include "app/application.hpp"

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_messagebox.h>

#include <exception>
#include <iostream>

int main(int argc, char**) {
    if (argc != 1) {
        std::cerr << "moorhuhn2: command-line arguments are not supported\n";

        return 1;
    }

    try {
        return moorhuhn2::app::run();
    } catch (const std::exception& error) {
        std::cerr << "moorhuhn2: " << error.what() << '\n';
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Moorhuhn 2", error.what(), nullptr);

        return 1;
    }
}
