/**
 * @file main.c
 * @brief Clear screen utility
 *
 * This utility clears the terminal screen by printing ANSI escape sequences.
 * It provides a simple cross-platform way to clear the console output.
 *
 * @author monakaibrahim-cmyk
 * @version 1.0.0
 */

#include <stdio.h>

/**
 * @brief Main entry point for the clear command
 *
 * This function clears the terminal screen by sending the ANSI escape
 * sequence for cursor home position (\033[H) followed by the clear
 * screen sequence (\033[J). This is compatible with most modern
 * terminal emulators.
 *
 * @param argc Unused parameter. Included for compatibility.
 * @param argv Unused parameter. Included for compatibility.
 * @return Always returns 0 indicating successful execution
 *
 * @note This function assumes a compatible terminal that supports
 *       ANSI escape sequences
 */
int main(int, char**)
{
    printf("\033[H\033[J");

    return 0;
}
