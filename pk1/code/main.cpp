#include "game.h"

#include <iostream>

int main()
{
    try
    {
        RunGame();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
