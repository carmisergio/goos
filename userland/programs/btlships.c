#include <stdbool.h>
#include <stdint.h>

// GOOS libc
#include "time.h"
#include "stdio.h"
#include "stdlib.h"

#define ROWS 9
#define COLS 9
#define SHIPS 10

/****************
 * Computer ai memory
 */
int more_probable_rows[ROWS - 1];
int more_probable_columns[COLS - 1];
int more_probable_amount;

/*******************************************************************************
 * GRAPHICS CONSTANTS */
// Graphics display cells
// 0: Sea
// 1: Opponent Hit
// 2: Opponent Miss
// 3: Ship Sunk
// 4: Single Ship
// 5: Horizontal Multi Ship Start
// 6: Horizontal Multi Ship Start Hit
// 7: Horizontal Multi Ship Middle
// 8: Horizontal Multi Ship Middle Hit
// 9: Horizontal Multi Ship End
// 10: Horizontal Multi Ship End Hit
// 11: Vertical Multi Ship Start
// 12: Vertical Multi Ship Start Hit
// 13: Vertical Multi Ship Middle
// 14: Vertical Multi Ship Middle Hit
// 15: Vertical Multi Ship End
// 16: Vertical Multi Ship End Hit
char graphics_cells[17][3] = {
    {' ', '~', ' '},
    {' ', '~', ' '},
    {' ', '~', ' '},
    {' ', '~', ' '},
    {'<', '=', '>'},
    {' ', '<', ' '},
    {' ', '<', ' '},
    {' ', '=', ' '},
    {' ', '=', ' '},
    {' ', '>', ' '},
    {' ', '>', ' '},
    {' ', 'A', ' '},
    {' ', 'A', ' '},
    {' ', 'N', ' '},
    {' ', 'N', ' '},
    {' ', 'V', ' '},
    {' ', 'V', ' '},
};
char graphics_cells_colors[17][3] = {
    {0, 1, 0},
    {4, 4, 4},
    {3, 3, 3},
    {5, 5, 5},
    {2, 2, 2},
    {0, 2, 0},
    {4, 4, 4},
    {0, 2, 0},
    {4, 4, 4},
    {0, 2, 0},
    {4, 4, 4},
    {0, 2, 0},
    {4, 4, 4},
    {0, 2, 0},
    {4, 4, 4},
    {0, 2, 0},
    {4, 4, 4},
};

// Colors:
// 0: Default
// 1: Blue
// 2: Bold White
// 3: Cyan Background Bold
// 4: Red Background Bold
// 5: Purple Background Bold
// 6: Bold green
// 7: Bold yellow
// 8: Bold blue
// 9: Bold purple
// 10: Bold light blue
// 11: Bold red
char *color_codes[12] = {"\033[37;40m", "\033[34m", "\033[97m", "\033[46m", "\033[41m", "\033[45m", "\033[92m", "\033[93m", "\033[94m", "\033[35m", "\033[36m", "\033[31m"};

// Letters for each row
char letters[10] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'};

/******************************************************************************/

/********************************************************************************
 * Draws the ship select phase board to screen
 * Parameters:
 *  - display_map: reference to the display map to be drawn
 ********************************************************************************/
void draw_ship_select_board(int display_map[2][ROWS][COLS])
{
    int row, subrow, table, column, subcolumn;
    int selected_graphics_cell, selected_color;

    // Clear the screen
    printf("\033[H\033[2J");

    // Print the table title
    printf("%s                              POSIZIONA LE TUE NAVI%s\n",
           color_codes[2], color_codes[0]);
    // Print the legends and table row start line
    printf("                        1   2   3   4   5   6   7   8   9\n");
    printf("                      ÚÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄ¿\n");

    // Iterate on every row
    for (row = 0; row < ROWS; row++)
    {
        // Print the left legend and the vertical table row start line
        printf("                    %c ³", letters[row]);

        // Iterate on every column inside a row
        for (column = 0; column < COLS; column++)
        {

            // Every column is divided in subcolumns (single carachters) to print
            for (subcolumn = 0; subcolumn < 3; subcolumn++)
            {

                // Find the correct graphic cell indexed by our position on the board
                selected_graphics_cell = display_map[0][row][column];
                // Find the color of the subcolumn we want to print
                selected_color = graphics_cells_colors[selected_graphics_cell][subcolumn];
                // Print the subcolumn in its wanted colour
                printf("%s%c", color_codes[selected_color],
                       graphics_cells[selected_graphics_cell][subcolumn]);
                // Reset color
                printf(color_codes[0]);
            }

            // Print the table column end line
            printf("³");
        }

        // Print the table row end line
        if (row < ROWS - 1)
            printf("\n                      ÃÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄ´");
        printf("\n");
    }
    printf("                      ÀÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÙ\n");
    // Print graphics divider
    printf("%sÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%s\n",
           color_codes[2], color_codes[0]);
}

/*
 * Fills a display map with default state
 * Parameters:
 *  - display_map: reference to the display map that needs to be initialized
 ********************************************************************************/
void init_display_map(int display_map[2][ROWS][COLS])
{
    int i, j, k;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < ROWS; j++)
        {
            for (k = 0; k < COLS; k++)
            {
                display_map[i][j][k] = 0;
            }
        }
    }
}

/********************************************************************************
 * Fills a ship map with default state
 * Parameters
 *  - ship_map: reference to the ship map that needs to be initialized
 ********************************************************************************/
void init_ship_map(int ship_map[ROWS][COLS])
{
    int i, j;
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            ship_map[i][j] = 99;
        }
    }
}

/********************************************************************************
 * Fills a hit map with default state
 * Parameters
 *  - hit_map: reference to the hit map that needs to be initialized
 ********************************************************************************/
void init_hit_map(bool hit_map[ROWS][COLS])
{
    int i, j;
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            hit_map[i][j] = false;
        }
    }
}

void init_ship_lives(int ship_lives[SHIPS])
{
    // int *ship_lives_tmp = {4, 3, 3, 2, 2, 2, 1, 1, 1, 1};
    int i, j;
    int k = 0;
    for (i = 0; i <= 3; i++)
    {
        for (j = 0; j <= i; j++)
        {
            ship_lives[k] = 4 - i;
            k++;
        }
    }
}

void init_ai_map(int ai_map[ROWS][COLS])
{
    int i, j;
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            ai_map[i][j] = 0;
        }
    }
}

/********************************************************************************
 * Print a ship map for debug porpuses
 * Parameters:
 *  - ship_map: reference to the ship_map to be printed
 ********************************************************************************/
void debug_ship_map(int ship_map[ROWS][COLS])
{
    int i, j;
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            printf("%-2d ", ship_map[i][j]);
        }
        printf("\n");
    }
}

// Parse any character
bool parse_anychar(const char **input, char *c)
{
    // No charactesr to read
    if (!**input)
        return false;

    *c = **input;
    (*input)++;
    return true;
}

// Parse positive integer
bool parse_uint8_t(const char **input, uint8_t *n)
{
    bool has_num = false;
    const char *cur = *input;
    *n = 0;
    while (*cur)
    {
        // Check if current if valid digit
        if (*cur < '0' || *cur > '9')
            break;

        // Read number
        *n *= 10;
        *n += *cur - '0';

        has_num = true;

        cur++;
    }

    if (has_num)
    {
        // Success
        *input = cur;
        return true;
    }
    else
    {
        // Failure
        return false;
    }
}

// Consume single known character
bool parse_ctag(const char **input, char tag)
{
    if (**input == tag)
    {
        (*input)++;
        return true;
    }

    return false;
}

// Parse input coordinate from string
bool parse_coordinate(const char *input, char *row, uint8_t *col)
{
    // Consume leading spaces
    while (parse_ctag(&input, ' '))
        ;

    // Read row
    if (!parse_anychar(&input, row))
        return false;

    // Read column
    if (!parse_uint8_t(&input, col))
        return false;

    // Consume trailing spaces
    while (parse_ctag(&input, ' '))
        ;

    // Check if all characters were consumed
    if (*input)
        return false;

    return true;
}

/********************************************************************************
 * Gets coordinate as input from the user
 * Parameters
 *  - row: reference to the numerical index of the row the user has inputed
 *  - column: reference to the numerical index of the column
 *  		  the user has inputed
 * Returns: true if succesful
 ********************************************************************************/
bool get_input_coordinate(int *row, int *column)
{
    char raw_row;
    int raw_column, tmp_row, tmp_column;

    // Read input from console
    char input[2]; // Maximum input is two characters
    getsn(input, 2);

    // Get input and fail if not succesful
    uint8_t rawr_col;
    if (!parse_coordinate(input, &raw_row, &rawr_col))
        return false;
    raw_column = rawr_col;

    // Begin checking column
    tmp_column = raw_column - 1;
    if (tmp_column < 0 || tmp_column > COLS - 1)
        return false;

    // Then check row
    // Subtract ascii offset to get row number
    tmp_row = raw_row - 65;
    if (tmp_row < 0 || tmp_row > ROWS - 1)
    {
        // If it's not correct in uppercase, we try lowercase
        tmp_row = raw_row - 97;
        if (tmp_row < 0 || tmp_row > ROWS - 1)
            return false;
    }

    // Everything was succesful, so we set our output variables
    *row = tmp_row;
    *column = tmp_column;

    return true;
}

/********************************************************************************
 * Get ship direcion as input from user
 * Parameters
 *  - direction: reference to the direction [0: horizontal, 1: vertical]
 * Returns: true if succesful
 ********************************************************************************/
bool get_ship_direction(int *direction)
{
    char raw_direction;
    // Get input and fail if not succesful

    raw_direction = getchar();

    // See if inputed carachter is valid
    if (raw_direction == 'o')
    {
        *direction = 0;
        return true;
    }
    else if (raw_direction == 'v')
    {
        *direction = 1;
        return true;
    }

    return false;
}

/********************************************************************************
 * Lets user insert one character wihtout pressing enter
 * Returns: char inputed
 ********************************************************************************/
char get_immediate_character()
{

    char input;

    // Wait for character input
    input = getchar();

    return input;
}

/********************************************************************************
 * Handle positioning for a single width ship
 * Parameters
 *  - display_map: reference to the display map on which to save the ship
 *  - player_ship_map: reference to the player's ship map
 *  - ship_index: index of the ship to be positioned
 ********************************************************************************/
void position_single_ship(int display_map[2][ROWS][COLS], int player_ship_map[ROWS][COLS], int ship_index)
{
    int row, column;
    bool input_result = true;        // Was the input succesful?
    bool already_positioned = false; // Is there already a ship here?

    while (true)
    {
        // Draw the ship select board
        draw_ship_select_board(display_map);
        printf("sNave da posizionare:%s <=> \n", color_codes[9], color_codes[0]);

        // Print prompt
        if (input_result && !already_positioned)
            printf("%sDove vuoi posizionarla?%s\n", color_codes[10], color_codes[0]);
        else if (!input_result)
            printf("%sLa casella inserita non esiste. Riprova%s\n", color_codes[11], color_codes[0]);
        else
            printf("%sHai gi… posizionato una nave in questa casella. Riprova%s\n", color_codes[11], color_codes[0]);

        printf("%s[es. D4] -> %s", color_codes[2], color_codes[0]);

        // Get input from user
        input_result = get_input_coordinate(&row, &column);

        if (input_result)
        {
            // If we already have a ship positioned there, we pass and change the message
            if (player_ship_map[row][column] != 99)
            {
                already_positioned = true;
                continue;
            }

            // Position the ship on the display map and ship map
            display_map[0][row][column] = 4;
            player_ship_map[row][column] = ship_index;

            // We're done positioning
            return;
        }
    }
}

/********************************************************************************
 * Handle positioning for a multiple width ship
 * Parameters
 *  - display_map: reference to the display map on which to save the ship
 *  - player_ship_map: reference to the player's ship map
 *  - ship_index: index of the ship to be positioned
 *  - ship_lives: dimension of the ship
 ********************************************************************************/
void position_multi_ship(int display_map[2][ROWS][COLS], int player_ship_map[ROWS][COLS], int ship_index, int ship_lives)
{
    int start_row, start_column;
    bool is_horizontal_possible, is_vertical_possible; // Is_X_possible: 0 -> possible,
                                                       // 1 -> out of board, 2 -> ship in the way
    int direction;                                     // Direction: 0 -> horizontal, 1 -> vertical

    int input_result = 0; // Input Result: 0 -> success, 1 -> starting cell doesn't exist / ship out of board,
                          //               2 -> starting cell already used / ship crosses other ship
                          //               3 -> ship can't be positioned starting from this cell

    int i; // Counter

    // Get ship start position
    while (true)
    {
        draw_ship_select_board(display_map);
        printf("%sNave da posizionare:%s <=> x %d\n", color_codes[9], color_codes[0], ship_lives);

        // Print prompt
        switch (input_result)
        {
        case 0:
            printf("%sSeleziona la casella di partenza%s\n", color_codes[10], color_codes[0]);
            break;
        case 1:
            printf("%sLa casella inserita non esiste. Riprova%s\n", color_codes[11], color_codes[0]);
            break;
        case 2:
            printf("%sHai gi… posizionato una nave in questa casella. Riprova%s\n", color_codes[11], color_codes[0]);
            break;
        case 3:
            printf("%sNon puoi posizionare una nave da %d a partire da questa casella. Riprova%s\n", color_codes[11], ship_lives, color_codes[0]);
            break;
        }
        printf("%s[es. D4] -> %s", color_codes[2], color_codes[0]);

        // Get input from user
        if (!get_input_coordinate(&start_row, &start_column))
        {
            input_result = 1;
            continue;
        };

        // Verify that there isn't a ship already there
        if (player_ship_map[start_row][start_column] != 99)
        {
            input_result = 2;
            continue;
        }

        // Verify that we can fit a ship there vertically and horizontally
        is_vertical_possible = true;
        is_horizontal_possible = true;
        // Check vertical
        for (i = 0; i < ship_lives; i++)
        {
            // Check if cell is inside board boundaries
            if (start_row + i >= ROWS)
            {
                is_vertical_possible = false;
                break;
            }
            // Check if cell is available
            if (player_ship_map[start_row + i][start_column] != 99)
                is_vertical_possible = false;
        }
        // Check horizontal
        for (i = 0; i < ship_lives; i++)
        {
            // Check if cell is inside board boundaries
            if (start_column + i >= COLS)
            {
                is_horizontal_possible = false;
                break;
            }
            // Check if cell is available
            if (player_ship_map[start_row][start_column + i] != 99)
                is_horizontal_possible = false;
        }

        // If we cant' place the ship either horizontally or vertically
        if (!is_vertical_possible && !is_horizontal_possible)
        {
            input_result = 3;
            continue;
        }

        // Exit from the loop
        break;
    }

    // We need to ask the user for a position only if the ship can be placed
    // both horizontally and vertically, else we set it to what's possible
    if (is_horizontal_possible && is_vertical_possible)
    {
        // Get ship direction
        while (true)
        {
            // Position ship start position indicator
            display_map[0][start_row][start_column] = 2;

            // Draw the board
            draw_ship_select_board(display_map);
            printf("%sNave da posizionare:%s <=> x %d\n", color_codes[9], color_codes[0], ship_lives);

            // Print prompt
            printf("%sLa nave Š orizzontale o verticale?%s\n", color_codes[10], color_codes[0]);
            printf("%s[o, v] -> %s", color_codes[2], color_codes[0]);

            // Get input from user
            if (!get_ship_direction(&direction))
            {
                input_result = 1;
                continue;
            };

            break;
        }
    }
    else if (is_horizontal_possible)
        direction = 0; // Only horizontal is possible
    else
        direction = 1; // Only vertical is possible

    // Position ship on display map and player ship map
    if (direction == 0)
    {
        // For horizontal ships
        for (i = 0; i < ship_lives; i++)
        {
            if (i == 0)
                display_map[0][start_row][start_column + i] = 5; // Ship Start
            else if (i == ship_lives - 1)
                display_map[0][start_row][start_column + i] = 9; // Ship end
            else
                display_map[0][start_row][start_column + i] = 7; // Ship middle
            player_ship_map[start_row][start_column + i] = ship_index;
        }
    }
    else
    {
        // For vertical ships
        for (i = 0; i < ship_lives; i++)
        {
            if (i == 0)
                display_map[0][start_row + i][start_column] = 11; // Ship Start
            else if (i == ship_lives - 1)
                display_map[0][start_row + i][start_column] = 15; // Ship end
            else
                display_map[0][start_row + i][start_column] = 13; // Ship middle
            player_ship_map[start_row + i][start_column] = ship_index;
        }
    }
}

/********************************************************************************
 * Handle ship poisitioning phase
 * Parameters
 *  - display_map: reference to the display map on which to save the ships
 *  - player_ship_map: reference to the player's ship map
 *  - player_ship_lives: reference to the array of the amount of lives
 *  					 each ship has
 ********************************************************************************/
void ship_positioning_stage(int display_map[2][ROWS][COLS], int player_ship_map[ROWS][COLS], int player_ship_lives[SHIPS])
{
    int current_ship;

    // Iterate on every ship
    for (current_ship = 0; current_ship < SHIPS; current_ship++)
    {
        // Use the appropriate positioning function for single and mutiple width ships
        if (player_ship_lives[current_ship] == 1)
            position_single_ship(display_map, player_ship_map, current_ship);
        else
            position_multi_ship(display_map, player_ship_map, current_ship, player_ship_lives[current_ship]);
    }
}

/********************************************************************************
 * Randomly position a single width ship
 * Parameters
 *  - ship_map: reference to the player's ship map
 *  - ship_index: index of the ship to position
 ********************************************************************************/
void position_random_single_ship(int ship_map[ROWS][COLS], int ship_index)
{
    int row, column;
    bool already_positioned = false;

    while (true)
    {
        // Get random position
        row = (rand() % ROWS);
        column = (rand() % COLS);

        // If we already have a ship positioned there, we pass and change the message
        if (ship_map[row][column] != 99)
        {
            already_positioned = true;
            continue;
        }

        // Position the ship on the display map and ship map
        ship_map[row][column] = ship_index;

        // We're done positioning
        return;
    }
}

/********************************************************************************
 * Randomly position a multiple width ship
 * Parameters
 *  - ship_map: reference to the player's ship map
 *  - ship_index: index of the ship to position
 ********************************************************************************/
void position_random_multi_ship(int ship_map[ROWS][COLS], int ship_index, int ship_lives)
{
    int start_row, start_column;
    bool is_horizontal_possible, is_vertical_possible;
    // Is_X_possible: 0 -> possible, 1 -> out of board, 2 -> ship in the way
    int direction;
    // Direction: 0 -> horizontal, 1 -> vertical

    int i;

    // Get ship start position
    while (true)
    {
        // Get random position
        start_row = (rand() % ROWS);
        start_column = (rand() % COLS);

        // Verify that there isn't a ship already there
        if (ship_map[start_row][start_column] != 99)
        {
            continue;
        }

        // Verify that we can fit a ship there vertically and horizontally
        is_vertical_possible = true;
        is_horizontal_possible = true;
        for (i = 0; i < ship_lives; i++)
        {
            // Check if cell is inside board boundaries
            if (start_row + i >= ROWS)
            {
                is_vertical_possible = false;
                break;
            }
            // Check if cell is available
            if (ship_map[start_row + i][start_column] != 99)
                is_vertical_possible = false;
        }
        for (i = 0; i < ship_lives; i++)
        {
            // Check if cell is inside board boundaries
            if (start_column + i >= COLS)
            {
                is_horizontal_possible = false;
                break;
            }
            // Check if cell is available
            if (ship_map[start_row][start_column + i] != 99)
                is_horizontal_possible = false;
        }

        // If we cant' place the ship either horizontally or vertically
        if (!is_vertical_possible && !is_horizontal_possible)
        {
            continue;
        }

        // Exit from the loop
        break;
    }

    // We need to randomly select an orientation only if both vertical and horizontal are possible
    // both horizontally and vertically, else we set it to what's possible
    if (is_horizontal_possible && is_vertical_possible)
    {
        // Get ship direction
        while (true)
        {

            // Get random direction
            direction = (rand() % 2);

            break;
        }
    }
    else if (is_horizontal_possible)
        direction = 0; // Only horizontal is possible
    else
        direction = 1; // Only vertical is possible

    // Position ship on ship map
    if (direction == 0)
        // For horizontal ships
        for (i = 0; i < ship_lives; i++)
        {
            ship_map[start_row][start_column + i] = ship_index;
        }
    else
    {
        // For vertical ships
        for (i = 0; i < ship_lives; i++)
        {
            ship_map[start_row + i][start_column] = ship_index;
        }
    }
}

/********************************************************************************
 * Handle random positioning for the computer's ships
 * Parameters
 *  - ship_map: reference to the computer's ship map
 *  - ship_lives: array containing the lives of ships to be positioned
 ********************************************************************************/
void position_random_ships(int ship_map[ROWS][COLS], int ship_lives[SHIPS])
{
    int current_ship;

    // Iterate on every ship
    for (current_ship = 0; current_ship < SHIPS; current_ship++)
    {
        // Use appropriate function for single and multiple witdth ship
        if (ship_lives[current_ship] == 1)
            position_random_single_ship(ship_map, current_ship);
        else
            position_random_multi_ship(ship_map, current_ship, ship_lives[current_ship]);
    }
}

/********************************************************************************
 * Draws the main game board to screen
 * Parameters:
 *  - display_map: reference to the display map to be drawn
 ********************************************************************************/
void draw_game_board(int display_map[2][ROWS][COLS])
{
    int row, subrow, table, column, subcolumn;
    int selected_graphics_cell, selected_color;
    // We only print the second table if the game has started (We are not in ship select mode)

    // Clear the screen
    printf("\033[H\033[2J");

    // Print the table titles
    printf("                   %sTU                                    %sCOMPUTER\n", color_codes[6], color_codes[7]);
    putss(color_codes[0]);
    // Print the legends and table row start line
    putss("    1   2   3   4   5   6   7   8   9        1   2   3   4   5   6   7   8   9 \n");
    putss("  ÚÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄ¿    ÚÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄÂÄÄÄ¿\n");

    // Iterate on every row
    for (row = 0; row < ROWS; row++)
    {
        // We need to print two tables: the player ship table and the computer hits table
        for (table = 0; table < 2; table++)
        {

            // Print the left legend and the vertical table row start line
            printf("%c ³", letters[row]);

            // Iterate on every column inside a row
            for (column = 0; column < COLS; column++)
            {

                // Every column is divided in subcolumns (single carachters) to print
                for (subcolumn = 0; subcolumn < 3; subcolumn++)
                {

                    // Find the correct graphic cell indexed by our position on the board
                    selected_graphics_cell = display_map[table][row][column];
                    // Find the color of the subcolumn we want to print
                    selected_color = graphics_cells_colors[selected_graphics_cell][subcolumn];
                    // Print the column in its wanted colour
                    printf("%s%c", color_codes[selected_color],
                           graphics_cells[selected_graphics_cell][subcolumn]);
                    // Reset color
                    putss(color_codes[0]);
                }

                // Print the table column end line
                printf("³");
            }

            // // Print space between tables
            if (table == 0)
                printf("  ");
        }

        // Print the table row end line
        // printf("\n%s#    +---+---+---+---+---+---+---+---+---+---+        +---+---+---+---+---+---+---+---+---+---+ #%s", color_codes[2], color_codes[0]);
        if (row < ROWS - 1)
            putss("\n  ÃÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄ´    ÃÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄÅÄÄÄ´");
        putss("\n");
    }
    putss("  ÀÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÙ    ÀÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÁÄÄÄÙ\n");
    // Print graphics divider
    putss("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n");
}

void draw_hit_result_screen(int display_map[2][ROWS][COLS], int hit_result)
{
    draw_game_board(display_map);

    switch (hit_result)
    {
    case 0:
        putss(color_codes[10]);
        putss("                              ÉË»ÉÍ»É»ÉÉÍ»ÉÍ»ÉË»ÉÍ» \n");
        putss("                              ºººÌÍ¹ºººº  ÌÍ¹ º º º \n");
        putss("                              Ê ÊÊ Ê¼È¼ÈÍ¼Ê Ê Ê ÈÍ¼ ");
        break;
    case 1:
        putss(color_codes[11]);
        putss("                               ÉÍ»ÉÍ»Ë  ÉÍ»ËÉË»ÉÍ»  \n");
        putss("                               º  º ºº  ÌÍ¼º º º º  \n");
        putss("                               ÈÍ¼ÈÍ¼ÊÍ¼Ê  Ê Ê ÈÍ¼  ");
        break;
    case 2:
        putss(color_codes[9]);
        putss("             ÉÍ»ÉÍ»Ë  ÉÍ»ËÉË»ÉÍ»  ÉÍ»  ÉÍ»ÉÍ»ÉÍ»ÉÍ»É»ÉÉË»ÉÍ»ÉË»ÉÍ» \n");
        putss("             º  º ºº  ÌÍ¼º º º º  º¹   ÌÍ¹Ì¹ Ì¹ º ºººº ººÌÍ¹ º º º \n");
        putss("             ÈÍ¼ÈÍ¼ÊÍ¼Ê  Ê Ê ÈÍ¼  ÈÍ¼  Ê ÊÈ  È  ÈÍ¼¼È¼ÍÊ¼Ê Ê Ê ÈÍ¼ ");
        break;
    }
}

/********************************************************************************
 * Let player make a move
 * Parameters
 *  - display_map: reference to the display map on which to save the ships
 *  - player_ship_map: reference to the player's ship map
 *  - player_ship_lives: reference to the array of the amount of lives
 *  					 each ship has
 * Returns: 0 for miss, 1 for hit, 2 for hit and sunk
 ********************************************************************************/
int player_turn(int display_map[2][ROWS][COLS], int computer_ship_map[ROWS][COLS], int computer_ship_lives[SHIPS], bool player_hit_map[ROWS][COLS])
{
    int hit_row, hit_column;
    bool is_horizontal_possible, is_vertical_possible; // Is_X_possible: 0 -> possible,
                                                       // 1 -> out of board, 2 -> ship in the way

    int input_result = 0; // Input Result: 0 -> success, 1 -> hit cell doesn't exist / hit out of board,
                          //               2 -> hit cell has already been hit

    int ship_hit_index;

    int i, j; // Counter

    // Get ship start position
    while (true)
    {
        draw_game_board(display_map);
        printf("%sMossa: %sTU\n", color_codes[9], color_codes[6]);

        // Print prompt
        switch (input_result)
        {
        case 0:
            printf("%sSeleziona la casella che vuoi colpire%s\n", color_codes[10], color_codes[0]);
            break;
        case 1:
            printf("%sLa casella inserita non esiste. Riprova%s\n", color_codes[11], color_codes[0]);
            break;
        case 2:
            printf("%sHai gi… colpito questa casella. Riprova%s\n", color_codes[11], color_codes[0]);
            break;
        }

        printf("%s[es. D4] -> %s", color_codes[2], color_codes[0]);

        // Get input from user
        if (!get_input_coordinate(&hit_row, &hit_column))
        {
            input_result = 1;
            continue;
        };

        // Verify that the player hasn't already hit there
        if (player_hit_map[hit_row][hit_column])
        {
            input_result = 2;
            continue;
        }
        break;
    }

    player_hit_map[hit_row][hit_column] = true;

    // See if we hit a ship
    ship_hit_index = computer_ship_map[hit_row][hit_column];
    if (ship_hit_index != 99)
    {
        computer_ship_lives[ship_hit_index]--; // Decrement lives of the ship we hit
        if (computer_ship_lives[ship_hit_index] == 0)
        {
            // Fill sunken ship with correct color
            for (i = 0; i < ROWS; i++)
            {
                for (j = 0; j < COLS; j++)
                {
                    if (computer_ship_map[i][j] == ship_hit_index)
                        display_map[1][i][j] = 3; // Update display map
                }
            }
            draw_hit_result_screen(display_map, 2);
            return 2; // The ship has no more lives left (ship sank)
        }
        else
        {
            display_map[1][hit_row][hit_column] = 1; // Update display map
            draw_hit_result_screen(display_map, 1);
            return 1; // Report normal hit
        }
    }
    else
    {
        display_map[1][hit_row][hit_column] = 2; // Update display map
        draw_hit_result_screen(display_map, 0);
        return 0;
    }
}

/********************************************************************************
 * Let computer make a move
 * Parameters
 *  - display_map: reference to the display map on which to save the ships
 * Returns: 0 for miss, 1 for hit, 2 for hit and sunk
 ********************************************************************************/
int computer_turn_easy(int display_map[2][ROWS][COLS], int player_ship_map[ROWS][COLS], int player_ship_lives[SHIPS], bool computer_hit_map[ROWS][COLS])
{
    int hit_row, hit_column;
    bool is_horizontal_possible, is_vertical_possible; // Is_X_possible: 0 -> possible,
                                                       // 1 -> out of board, 2 -> ship in the way

    int input_result = 0; // Input Result: 0 -> success, 1 -> hit cell doesn't exist / hit out of board,
                          //               2 -> hit cell has already been hit

    int ship_hit_index;
    int i, j, k;

    draw_game_board(display_map);
    printf("%sMossa: %sCOMPUTER\n\n", color_codes[9], color_codes[7]);
    printf("%sSto pensando...", color_codes[7]);

    // hit_row = lastcellid / 10;
    // hit_column = lastcellid % 10;
    // lastcellid++;

    if (more_probable_amount > 0)
    {
        // Hit a  high probability target
        i = (rand() % more_probable_amount);
        hit_row = more_probable_rows[i];
        hit_column = more_probable_columns[i];
    }
    else
    {
        // Just hit a random target
        do
        {
            hit_row = (rand() % ROWS);
            hit_column = (rand() % COLS);
        } while (computer_hit_map[hit_row][hit_column]);
    }

    computer_hit_map[hit_row][hit_column] = true;

    sleep(1);

    // See if we hit a ship
    ship_hit_index = player_ship_map[hit_row][hit_column];
    if (ship_hit_index != 99)
    {
        player_ship_lives[ship_hit_index]--; // Decrement lives of the ship we hit
        if (player_ship_lives[ship_hit_index] == 0)
        {
            // Fill sunken ship with correct color
            for (i = 0; i < ROWS; i++)
            {
                for (j = 0; j < COLS; j++)
                {
                    if (player_ship_map[i][j] == ship_hit_index)
                        display_map[0][i][j] = 3; // Update display map
                }
            }
            more_probable_amount = 0;
            draw_hit_result_screen(display_map, 2);
            return 2; // The ship has no more lives left (ship sank)
        }
        else
        {
            display_map[0][hit_row][hit_column] += 1; // Update display map

            // Check possibilities for probable points
            k = 0;
            if (hit_row - 1 >= 0 && !computer_hit_map[hit_row - 1][hit_column])
            {
                more_probable_rows[k] = hit_row - 1;
                more_probable_columns[k] = hit_column;
                k++;
                more_probable_amount = k;
            }
            if (hit_row + 1 < ROWS && !computer_hit_map[hit_row + 1][hit_column])
            {
                more_probable_rows[k] = hit_row + 1;
                more_probable_columns[k] = hit_column;
                k++;
                more_probable_amount = k;
            }
            if (hit_column - 1 >= 0 && !computer_hit_map[hit_row][hit_column - 1])
            {
                more_probable_rows[k] = hit_row;
                more_probable_columns[k] = hit_column - 1;
                k++;
                more_probable_amount = k;
            }
            if (hit_column + 1 < COLS && !computer_hit_map[hit_row][hit_column + 1])
            {
                more_probable_rows[k] = hit_row;
                more_probable_columns[k] = hit_column + 1;
                k++;
                more_probable_amount = k;
            }

            // // Find all adjacent cells to be used in the next move
            // k = 0; // Count how many adjacent cells are valid
            // for (int i = hit_row - 1; i <= hit_row + 1; i++)
            // {
            //   for (int j = hit_column - 1; j <= hit_column + 1; j++)
            //   {
            //     if (i >= 0 && i < 10 && j >= 0 && j < 10 && !computer_hit_map[i][j])
            //     // Found a valid cell
            //     {
            //       more_probable_rows[k] = i;
            //       more_probable_columns[k] = j;
            //       more_probable_amount = k + 1;
            //       k++;
            //     }
            //   }
            // }
            // sleep(5);
            draw_hit_result_screen(display_map, 1);
            return 1; // Report normal hit
        }
    }
    else
    {
        display_map[0][hit_row][hit_column] = 2; // Update display map
        draw_hit_result_screen(display_map, 0);
        more_probable_amount = 0;
        return 0;
    }
}

bool find_coordinate_to_hit_highprob(int ai_map[ROWS][COLS], bool computer_hit_map[ROWS][COLS], int was_hit_row, int was_hit_column, int *hit_row, int *hit_column)
{
    // Check possibilities for probable points
    if ((was_hit_row - 1) >= 0)
        if (!computer_hit_map[was_hit_row - 1][was_hit_column])
        {
            *hit_row = was_hit_row - 1;
            *hit_column = was_hit_column;
            return true;
        }
    if ((was_hit_row + 1) < ROWS)
        if (!computer_hit_map[was_hit_row + 1][was_hit_column])
        {
            *hit_row = was_hit_row + 1;
            *hit_column = was_hit_column;
            return true;
        }
    if ((was_hit_column - 1) >= 0)
        if (!computer_hit_map[was_hit_row][was_hit_column - 1])
        {
            *hit_row = was_hit_row;
            *hit_column = was_hit_column - 1;
            return true;
        }
    if ((was_hit_column + 1) < COLS)
        if (!computer_hit_map[was_hit_row][was_hit_column + 1])
        {
            *hit_row = was_hit_row;
            *hit_column = was_hit_column + 1;
            return true;
        }
    return false;
}

/********************************************************************************
 * Find first cell that was hit in ai map
 * Parameters:
 *  - int ai_map[ROWS][COLS]: Ai map
 *  - int start_row: Row from which to start looking
 *  - int start_column: Column from which to start looking
 *  - int *row: Pointer to variable to update with row of found cell
 *  - int *column: Pointer to variable to update with column of found cell
 * Returns: true if there is a cell in the hit state, false if there isn't
 ********************************************************************************/
bool find_first_hit_cell(int ai_map[ROWS][COLS], int start_row, int start_column, int *row, int *column)
{
    int i = start_row, j = start_column;

    while (i < ROWS)
    {
        // Return beam to start of next row
        if (j >= COLS - 1)
        {
            j = 0;
            i++;
        }

        // Find cell that was hit
        if (ai_map[i][j] == 1)
        {
            *row = i;
            *column = j;
            return true;
        }

        j++;
    }
    return false;
}

// Ai map values
// 0 -> not hit/miss
// 1 -> hit
// 2 -> sunk
int computer_turn_difficult(int display_map[2][ROWS][COLS], int player_ship_map[ROWS][COLS], int player_ship_lives[SHIPS], bool computer_hit_map[ROWS][COLS], int ai_map[ROWS][COLS])
{
    int i, j, k;
    int hit_row, hit_column;
    int was_hit_row = 0, was_hit_column = 0;
    bool found_hit_andnot_sunk, selected_highprob_move;

    int ship_hit_index;

    draw_game_board(display_map);
    printf("%sMossa: %sCOMPUTER\n\n", color_codes[9], color_codes[7]);
    printf("%sSto pensando...", color_codes[7]);

    // Compute move

    // debug_ship_map(ai_map);

    // See if there are ships hit but not sunk
    found_hit_andnot_sunk = find_first_hit_cell(ai_map, was_hit_row, was_hit_column, &was_hit_row, &was_hit_column);
    if (found_hit_andnot_sunk)
    {
        selected_highprob_move = find_coordinate_to_hit_highprob(ai_map, computer_hit_map, was_hit_row, was_hit_column, &hit_row, &hit_column);
        while (!selected_highprob_move)
        {
            found_hit_andnot_sunk = find_first_hit_cell(ai_map, was_hit_row, was_hit_column + 1, &was_hit_row, &was_hit_column);
            if (found_hit_andnot_sunk)
            {
                selected_highprob_move = find_coordinate_to_hit_highprob(ai_map, computer_hit_map, was_hit_row, was_hit_column, &hit_row, &hit_column);
            }
        }
    }

    // Find coordinates to hit if there isn't a highprob move available
    if (!selected_highprob_move)
    {
        do
        {
            hit_row = (rand() % ROWS);
            hit_column = (rand() % COLS);
        } while (computer_hit_map[hit_row][hit_column]);
    }

    computer_hit_map[hit_row][hit_column] = true;

    sleep(1);

    // See if we hit a ship
    ship_hit_index = player_ship_map[hit_row][hit_column];
    if (ship_hit_index != 99)
    {
        player_ship_lives[ship_hit_index]--; // Decrement lives of the ship we hit
        if (player_ship_lives[ship_hit_index] == 0)
        {
            // Fill sunken ship with correct color
            for (i = 0; i < ROWS; i++)
            {
                for (j = 0; j < COLS; j++)
                {
                    if (player_ship_map[i][j] == ship_hit_index)
                    {

                        display_map[0][i][j] = 3; // Update display map
                        ai_map[i][j] = 2;         // Set ai map to sunk
                    }
                }
            }
            draw_hit_result_screen(display_map, 2);
            return 2; // The ship has no more lives left (ship sank)
        }
        else
        {
            display_map[0][hit_row][hit_column] += 1; // Update display map
            ai_map[hit_row][hit_column] = 1;          // Set ai map to hit
        }
        draw_hit_result_screen(display_map, 1);
        return 1; // Report normal hit
    }
    else
    {
        display_map[0][hit_row][hit_column] = 2; // Update display map
        draw_hit_result_screen(display_map, 0);
        return 0;
    }
}

/*
 * Draw the end screen with either victory or lost
 * Parameters:
 *  - bool has_won: result of the game. true for win, 0 for lost
 */
void draw_end_screens(bool has_won)
{
    // Clear the screen
    printf("\033[H\033[2J");
    puts(color_codes[0]);
    putss("\n\n\n\n\n\n");
    // If the player has won
    if (has_won)
    {
        printf(" %s      ÛÛ   ÛÛ  ÛÛÛÛÛ  ÛÛ     ÛÛ    ÛÛ ÛÛ ÛÛÛ    ÛÛ ÛÛÛÛÛÛÛÛ  ÛÛÛÛÛÛ  ÛÛ  \n", color_codes[6]);
        puts("       ÛÛ   ÛÛ ÛÛ   ÛÛ ÛÛ     ÛÛ    ÛÛ ÛÛ ÛÛÛÛ   ÛÛ    ÛÛ    ÛÛ    ÛÛ ÛÛ      ");
        puts("       ÛÛÛÛÛÛÛ ÛÛÛÛÛÛÛ ÛÛ     ÛÛ    ÛÛ ÛÛ ÛÛ ÛÛ  ÛÛ    ÛÛ    ÛÛ    ÛÛ ÛÛ    ");
        puts("       ÛÛ   ÛÛ ÛÛ   ÛÛ ÛÛ      ÛÛ  ÛÛ  ÛÛ ÛÛ  ÛÛ ÛÛ    ÛÛ    ÛÛ    ÛÛ       ");
        printf("       ÛÛ   ÛÛ ÛÛ   ÛÛ ÛÛ       ÛÛÛÛ   ÛÛ ÛÛ   ÛÛÛÛ    ÛÛ     ÛÛÛÛÛÛ  ÛÛ   %s\n", color_codes[0]);
    }
    else
    {
        printf(" %s      ÛÛ   ÛÛ  ÛÛÛÛÛ  ÛÛ     ÛÛÛÛÛÛ  ÛÛÛÛÛÛÛ ÛÛÛÛÛÛ  ÛÛÛÛÛÛÛ  ÛÛÛÛÛÛ  ÛÛ \n", color_codes[11]);
        puts("       ÛÛ   ÛÛ ÛÛ   ÛÛ ÛÛ     ÛÛ   ÛÛ ÛÛ      ÛÛ   ÛÛ ÛÛ      ÛÛ    ÛÛ ÛÛ      ");
        puts("       ÛÛÛÛÛÛÛ ÛÛÛÛÛÛÛ ÛÛ     ÛÛÛÛÛÛ  ÛÛÛÛÛ   ÛÛÛÛÛÛ  ÛÛÛÛÛÛÛ ÛÛ    ÛÛ ÛÛ      ");
        puts("       ÛÛ   ÛÛ ÛÛ   ÛÛ ÛÛ     ÛÛ      ÛÛ      ÛÛ   ÛÛ      ÛÛ ÛÛ    ÛÛ         ");
        printf("       ÛÛ   ÛÛ ÛÛ   ÛÛ ÛÛ     ÛÛ      ÛÛÛÛÛÛÛ ÛÛ   ÛÛ ÛÛÛÛÛÛÛ  ÛÛÛÛÛÛ  ÛÛ %s\n", color_codes[0]);
    }
    putss("\n\n\n\n\n\n");
    putss(color_codes[2]);
    puts("               ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»");
    printf("               º  Premi %s[R]%s per giocare ancora, %s[Q]%s per uscire. º \n", color_codes[10], color_codes[2], color_codes[11], color_codes[2]);
    puts("               ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼");
}

/*
 * Draw the end screen with either victory or lost
 * Parameters:
 *  - bool has_won: result of the game. true for win, 0 for lost
 */
void draw_splash_screen()
{
    // Clear the screen
    printf("\033[H\033[2J");

    // Draw screen
    putss("\n\n\n\n");
    putss(color_codes[10]);
    putss("      ÛÛÛÛÛÛ   ÛÛÛÛÛ  ÛÛÛÛÛÛÛÛ ÛÛÛÛÛÛÛÛ  ÛÛÛÛÛ   ÛÛÛÛÛÛ  ÛÛ      ÛÛ  ÛÛÛÛÛ    \n");
    putss("      ÛÛ   ÛÛ ÛÛ   ÛÛ    ÛÛ       ÛÛ    ÛÛ   ÛÛ ÛÛ       ÛÛ      ÛÛ ÛÛ   ÛÛ   \n");
    putss("      ÛÛÛÛÛÛ  ÛÛÛÛÛÛÛ    ÛÛ       ÛÛ    ÛÛÛÛÛÛÛ ÛÛ   ÛÛÛ ÛÛ      ÛÛ ÛÛÛÛÛÛÛ   \n");
    putss("      ÛÛ   ÛÛ ÛÛ   ÛÛ    ÛÛ       ÛÛ    ÛÛ   ÛÛ ÛÛ    ÛÛ ÛÛ      ÛÛ ÛÛ   ÛÛ   \n");
    putss("      ÛÛÛÛÛÛ  ÛÛ   ÛÛ    ÛÛ       ÛÛ    ÛÛ   ÛÛ  ÛÛÛÛÛÛ  ÛÛÛÛÛÛÛ ÛÛ ÛÛ   ÛÛ   \n");
    putss(color_codes[11]);
    putss("\n");
    putss("                ÛÛÛ    ÛÛ  ÛÛÛÛÛ  ÛÛ    ÛÛ  ÛÛÛÛÛ  ÛÛ      ÛÛÛÛÛÛÛ  \n");
    putss("                ÛÛÛÛ   ÛÛ ÛÛ   ÛÛ ÛÛ    ÛÛ ÛÛ   ÛÛ ÛÛ      ÛÛ       \n");
    putss("                ÛÛ ÛÛ  ÛÛ ÛÛÛÛÛÛÛ ÛÛ    ÛÛ ÛÛÛÛÛÛÛ ÛÛ      ÛÛÛÛÛ    \n");
    putss("                ÛÛ  ÛÛ ÛÛ ÛÛ   ÛÛ  ÛÛ  ÛÛ  ÛÛ   ÛÛ ÛÛ      ÛÛ       \n");
    putss("                ÛÛ   ÛÛÛÛ ÛÛ   ÛÛ   ÛÛÛÛ   ÛÛ   ÛÛ ÛÛÛÛÛÛÛ ÛÛÛÛÛÛÛ  \n");
    putss("\n\n\n");
    putss(color_codes[2]);
    putss("                    ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ» \n");
    putss("                    º  Seleziona il livello di difficolt…:  º \n");
    printf("                    º             %s[1]: Facile%s               º \n", color_codes[7], color_codes[2]);
    printf("                    º            %s[2]: Difficile%s             º \n", color_codes[9], color_codes[2]);
    putss("                    ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼ \n");
}

/********************************************************************************
 * Checks if one of the parties has won
 * Parameters:
 *  - player_ship_lives: reference to the player's ships' lives
 *  - computer_ship_lives: reference to the computer's ships' lives
 * Returns: 0 if noone has won, 1 if the player has won,
 *          2 if the computer has won
 ********************************************************************************/
int check_for_victory(int player_ship_lives[SHIPS], int computer_ship_lives[SHIPS])
{
    bool has_player_won = true, has_computer_won = true;
    int i, j;
    // Check if the player has won
    for (i = 0; i < SHIPS; i++)
    {
        if (computer_ship_lives[i] > 0)
        { // If the ship still has lives
            has_player_won = false;
            break;
        }
    }

    // If the player has won, we dont even check the computer
    if (has_player_won)
        return 1;

    // Check if the computer has won
    for (i = 0; i < SHIPS; i++)
    {
        if (player_ship_lives[i] > 0)
        { // If the ship still has lives
            has_computer_won = false;
            break;
        }
    }
    // If the computer has won
    if (has_computer_won)
        return 2;

    // If noone has won
    return 0;
}

/********************************************************************************
 * Handle main game logic
 * Parameters:
 *  - display_map: reference to the display map to be drawn
 * Returns: true if the player has won
 ********************************************************************************/
int main_game(int display_map[2][ROWS][COLS], int player_ship_map[ROWS][COLS], bool player_hit_map[ROWS][COLS], int player_ship_lives[SHIPS], int computer_ship_map[ROWS][COLS], bool computer_hit_map[ROWS][COLS], int computer_ship_lives[], int ai_mode)
{
    bool game_running = true, is_player_turn = true;
    int ai_map[ROWS][COLS];

    bool thing = false; // TODO REMOVE

    // Initialize map for difficult mode
    if (ai_mode == 1)
        init_ai_map(ai_map);

    int turn_result, has_someone_won;
    while (game_running)
    {
        if (is_player_turn)
        {
            player_turn(display_map, computer_ship_map, computer_ship_lives, player_hit_map);
        }
        else
        {
            if (ai_mode == 0)
                computer_turn_easy(display_map, player_ship_map, player_ship_lives, computer_hit_map);
            else if (ai_mode == 1)
                computer_turn_difficult(display_map, player_ship_map, player_ship_lives, computer_hit_map, ai_map);
        }
        sleep(1);

        // Check if someone has won
        has_someone_won = check_for_victory(player_ship_lives, computer_ship_lives);

        // draw_end_screens(thing);
        // thing = !thing;
        // sleep(5);

        // int j;
        // for (j = 0; j < 10; j++)
        // {
        //   printf("%d|", computer_ship_lives[j]);
        // }
        // printf("\n");

        // sleep(1);

        if (has_someone_won == 1)
            return true; // The player has won
        else if (has_someone_won == 2)
            return false; // The computer has won

        is_player_turn = !is_player_turn;
    }
    return false;
}

void end_screen(bool game_result)
{
    char input;

    // Draw end screen
    draw_end_screens(game_result);

    do
    {
        input = get_immediate_character();
    } while (input != 'r' && input != 'q');

    // If user wants to exit, exit
    if (input == 'q')
    {
        // Clear the screen
        printf("\033[H\033[2J");
        exit(0);
    }
}

// Returns difficulty setting
int splash_screen()
{
    char input;

    // Draw end screen
    draw_splash_screen();

    while (true)
    {
        switch (get_immediate_character())
        {
        case '1':
            return 0;
        case '2':
            return 1;
        }
    }
}

/********************************************************************************
 * Main function
 ********************************************************************************/
int main()
{
    // Game wide maps
    int display_map[2][ROWS][COLS];
    int player_ship_map[ROWS][COLS];
    bool player_hit_map[ROWS][COLS];
    int player_ship_lives[SHIPS];
    int computer_ship_map[ROWS][COLS];
    bool computer_hit_map[ROWS][COLS];
    int computer_ship_lives[SHIPS];

    // Difficulty setting
    int difficulty;

    // Init random nuber generator
    srand(time(0));

    // Run game indefinitely
    while (true)
    {
        // Initialize maps
        init_display_map(display_map);
        init_ship_map(player_ship_map);
        init_ship_map(computer_ship_map);
        init_hit_map(player_hit_map);
        init_hit_map(computer_hit_map);
        init_ship_lives(player_ship_lives);
        init_ship_lives(computer_ship_lives);

        // Draw splash screen and get difficulty from player
        difficulty = splash_screen();

        // Position player ships
        ship_positioning_stage(display_map, player_ship_map, player_ship_lives);

        // Position the computer's ships
        position_random_ships(computer_ship_map, computer_ship_lives);

        // Start main game
        bool game_result = main_game(display_map, player_ship_map, player_hit_map, player_ship_lives, computer_ship_map, computer_hit_map, computer_ship_lives, difficulty);

        end_screen(game_result);
    }
}
