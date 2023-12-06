#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<ncurses.h>
#include<string.h>
#include<unistd.h>
#include<time.h>

                                                  

// a function to randomly generate a number with given bound
int random_number(int lower, int upper)
{
    // generate the random number
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}

// type define a struct for agent
typedef struct AGENT
{
    int x;
    int y;
    int heading_x;
    int heading_y;
    int health;
    int entropy;
    int direction; // 0: up, 1: right, 2: down, 3: left
    char apearances[10]; // e.g. + + + +
    char heading[10]; // e.g. ^ > v <
    int speed; // from 1 to 4
    int color_index; // 0 - white, 1 - red, 2 - green, 3 - yellow, 4 - blue, 5 - magenta
    int critical_health;
    int hop_distance;
    int hop_cool_down;
    int force_impulse_on;
    int force_impulse_default_radius;
    int force_impulse_radius;
    int max_health;
} AGENT;

typedef struct EFFECTOR
{
    int id; // 0 - radicalist, 1 - motherboard, 2 - batterytruck, 3 - null
    int x;
    int y;
    int delta_health;
    int delta_entropy;
    char look;
    int color_index; // 0 - white, 1 - red, 2 - green, 3 - yellow, 4 - blue, 5 - magenta
    int direction; // -1: none, 0: up, 1: right, 2: down, 3: left
    int paralyzed_count_down; // radicalist only moves when this is 0.
} EFFECTOR;

typedef struct GAMELEVEL
{
    int level_index;
    int number_of_radicalists;
    int number_of_motherboards;
    int number_of_batterytrucks;
    int max_number_of_levels;
    int *entry_requirements; // a pointer to an array of entry requirements
} GAMELEVEL;


int entry_requirements_setting[] = {0, 10 , 20, 30,60};
// a function to set game level parameters
GAMELEVEL *set_GAMELEVEL(int level_index)
{
    GAMELEVEL *g = malloc(sizeof(GAMELEVEL));
    // set entry requirements
    g->entry_requirements = entry_requirements_setting;
    g->max_number_of_levels = (int) sizeof(entry_requirements_setting)/sizeof(entry_requirements_setting[0])-1;
    switch (level_index)
    {
        case 0:
            g->level_index = 0;
            g->number_of_radicalists = random_number(15,20);
            g->number_of_motherboards = random_number(5,6);
            g->number_of_batterytrucks = random_number(8,9);
           break;
        case 1:
            g->level_index = 1;
            g->number_of_radicalists = random_number(17,27);
            g->number_of_motherboards = random_number(6,7);
            g->number_of_batterytrucks = random_number(4,5);
            break;
        case 2:
            g->level_index = 2;
            g->number_of_radicalists = random_number(30,45);
            g->number_of_motherboards = random_number(7,8);
            g->number_of_batterytrucks = random_number(15,16);
            break;
        case 3:
            g->level_index = 3;
            g->number_of_radicalists = random_number(60,75);
            g->number_of_motherboards = random_number(9,10);
            g->number_of_batterytrucks = random_number(10,16);
            break;
        
    }
    return g;
}

// a function to initialize the agent
AGENT *init_AGENT(int x, int y)
{
    AGENT *a = malloc(sizeof(AGENT));
    a->x = x;
    a->y = y;
    a->health = 20;
    a->critical_health = 3;
    a->entropy = 0;
    a->direction = 1;
    a->apearances[0] = '+';
    a->apearances[1] = '+';
    a->apearances[2] = '+';
    a->apearances[3] = '+';
    a->heading[0] = '|';
    a->heading[1] = '-';
    a->heading[2] = '|';
    a->heading[3] = '-';
    a->heading_x = a->x;
    a->heading_y = a->y-1;
    a->speed = 1;
    a->color_index = 0; // white
    a->hop_distance = 12;
    a->hop_cool_down = 0;
    a->force_impulse_on = 0;
    a->force_impulse_default_radius = 5;
    a->force_impulse_radius = 5;
    a->max_health = 20;
    return a;
}

EFFECTOR *init_Radicalist(int x, int y)
{
    EFFECTOR *r = malloc(sizeof(EFFECTOR));
    r->id = 0;
    r->x = x;
    r->y = y;
    r->delta_health = -1;
    r->delta_entropy = 0;
    r->look = 'X';
    r->color_index = 1; // red
    r->direction = random_number(0,3);
    r->paralyzed_count_down = 0;
    return r;
}

EFFECTOR *init_MotherBoard(int x, int y)
{
    EFFECTOR *m = malloc(sizeof(EFFECTOR));
    m->id = 1;
    m->x = x;
    m->y = y;
    m->delta_health = 0;
    m->delta_entropy = 5;
    m->look = 'M';
    m->color_index = 3; // yellow
    m->direction = -1;
    return m;
}

EFFECTOR *init_BatteryTruck(int x, int y)
{
    EFFECTOR *b = malloc(sizeof(EFFECTOR));
    b->id = 2;
    b->x = x;
    b->y = y;
    b->delta_health = 1;
    b->delta_entropy = 2;
    b->look = 'B';
    b->color_index = 4 ; // blue
    b->direction = -1;
    return b;
}

EFFECTOR *init_NullEffector(int x, int y)
{
    EFFECTOR *n = malloc(sizeof(EFFECTOR));
    n->id = 3;
    n->x = x;
    n->y = y;
    n->delta_health = 0;
    n->delta_entropy = 0;
    n->look = '_';
    n->color_index = 0;
    n->direction = -1;
    return n;
}

int relative_direction(EFFECTOR *e, AGENT *a)
{
    // 0 - up, 1 - right, 2 - down, 3 - left
    int distance_x = abs(e->x - a->x);
    int distance_y = abs(e->y - a->y);
    // based on the distance, determine the direction and follow the direction with the least distance
    if(distance_x > distance_y)
    {
        if(e->x > a->x) return 3;
        else return 1;
    }
    else if(distance_x < distance_y)
    {
        if(e->y > a->y) return 0;
        else return 2;
    }       
    else return 1;
}

int relative_direction_reversed(EFFECTOR *e, AGENT *a)
{
    // this function computes the relative direction from the agent to the effector, then return the opposite direction
    // 0 - up, 1 - right, 2 - down, 3 - left
    int distance_x = abs(e->x - a->x);
    int distance_y = abs(e->y - a->y);
    // based on the distance, determine the direction and follow the direction with the least distance
    if(distance_x > distance_y)
    {
        if(e->x > a->x) return 1;
        else return 3;
    }
    else if(distance_x < distance_y)
    {
        if(e->y > a->y) return 2;
        else return 0;
    }       
    else return 1;
}

void changeEffectorPosition(EFFECTOR *e, int delta)
{
    // based on direction
    // 0 - up, 1 - right, 2 - down, 3 - left
    switch(e->direction)
    {
        case 0:
            e->y -= delta;
            break;
        case 1:
            e->x += delta;
            break;
        case 2:
            e->y += delta;
            break;
        case 3:
            e->x -= delta;
            break;
    }
}

// read a txt file of ASCII and draw the ASCII art in a given window
void DrawASCII_from_TxtFile(char *filename, WINDOW *win , int start_x, int start_y, int color_index)
{
    // open the file
    FILE *fp;
    fp = fopen(filename, "r");
    // read the file line by line
    char line[100];
    int line_index = 0;
    while(fgets(line, 100, fp) != NULL)
    {
        // print the line

        mvwprintw(win, start_y+line_index, start_x, "%s", line);
        line_index += 1;
    }
    // close the file
    fclose(fp);
}


int main()
{
    // variables
    char *game_name = "The Supercool Untitled Terminal Game";
    int max_x, max_y;

    initscr(); // initialize the screen
    
    getmaxyx(stdscr, max_y, max_x); // get screen size
    mvprintw(0, (max_x-strlen(game_name))/2, "%s", game_name); // print the game name in the middle of the screen

    // initialize colors
    start_color();
    init_pair(0, COLOR_WHITE, COLOR_BLACK);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3,COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_MAGENTA);


    // create a window
    // windows size:
    int win_game_SIZE_X = max_x - 2;
    int win_game_SIZE_Y = 50;
    WINDOW *win_game = newwin(win_game_SIZE_Y, win_game_SIZE_X, 1, 1); // create a window
    // draw the box around the window
    box(win_game, 0, 0);


    // refresh the window
    wrefresh(stdscr); // wrefresh is different from refresh in that it refreshes a specific window
    wrefresh(win_game);

    // show ascii art on the center of the screen
    DrawASCII_from_TxtFile("ohYEAH.txt", win_game, 50, 10, 0);
    // refresh the window
    wrefresh(win_game);
    wrefresh(stdscr);
    // wait for key press
    nodelay(stdscr, FALSE); // heyhey, now we want the blocking getch
    getch();
    // clear the window
    wclear(win_game);
    wrefresh(win_game);

    nodelay(stdscr, TRUE);




    // initialize the agent
    int    init_agent_x = random_number(5, win_game_SIZE_X-5);
    int    init_agent_y = random_number(5, win_game_SIZE_Y-5);
    AGENT *Aleph = init_AGENT(init_agent_x, init_agent_y);

    // initialize the effectors
    // initialize effectors - Radicalists

    // create event loop
    // essential variables
    int ch; // the character that is pressed
    int level_index = 0;
    GAMELEVEL *game_level_config = set_GAMELEVEL(level_index);
    // create a list of effectors
    int number_of_effectors = game_level_config->number_of_radicalists + game_level_config->number_of_motherboards + game_level_config->number_of_batterytrucks;
    EFFECTOR **effectors = malloc(number_of_effectors*sizeof(EFFECTOR*));
    // initialize the effectors
    for (int i = 0; i < game_level_config->number_of_radicalists; i++)
    {
        int init_effector_x = random_number(5, win_game_SIZE_X-5);
        int init_effector_y = random_number(5, win_game_SIZE_Y-5);
        effectors[i] = init_Radicalist(init_effector_x, init_effector_y);
    }
    for (int i = game_level_config->number_of_radicalists; i < game_level_config->number_of_radicalists + game_level_config->number_of_motherboards; i++)
    {
        int init_effector_x = random_number(5, win_game_SIZE_X-5);
        int init_effector_y = random_number(5, win_game_SIZE_Y-5);
        effectors[i] = init_MotherBoard(init_effector_x, init_effector_y);
    }
    for (int i = game_level_config->number_of_radicalists + game_level_config->number_of_motherboards; i < number_of_effectors; i++)
    {
        int init_effector_x = random_number(5, win_game_SIZE_X-5);
        int init_effector_y = random_number(5, win_game_SIZE_Y-5);
        effectors[i] = init_BatteryTruck(init_effector_x, init_effector_y);
    }

    int boarder_color_reset_counter = 0;
    int boarder_color_index = 0;
    int refresh_time = 150; // in milliseconds
    while(level_index < game_level_config->max_number_of_levels)
    {

        // if the agent is dead, end the game
        if(Aleph->health <= 0)
          {
            clear();
            // clean everything in the game window
            wclear(win_game);
            wrefresh(win_game);

            // print game over at the center of the screen
            attron(COLOR_PAIR(1));
            mvprintw(max_y/2, (max_x-strlen("GAME OVER"))/2, "GAME OVER");
            attroff(COLOR_PAIR(1));
            mvprintw(max_y/2+1, (max_x-strlen("It's a pretty simple game..."))/2, "It's a pretty simple game...");
            mvprintw(max_y/2+4, (max_x-strlen("(Press any key to exit)"))/2, "(Press any key to exit)");
            refresh();
            // wait for 3 seconds
            napms(3000);
            // wait for a key press
            nodelay(stdscr, FALSE); // heyhey, now we want the blocking getch
            getch();
            break;
        }
                // check if the player has won the game by completing the last level
        if(Aleph->entropy >= game_level_config->entry_requirements[game_level_config->max_number_of_levels])
        {
            // clear the entire stdscr
            clear();
            // clean everything in the game window
            wclear(win_game);
            wrefresh(win_game);

            // print game over at the center of the screen
            attron(COLOR_PAIR(2));
            mvprintw(max_y/2, (max_x-strlen("YOU WON"))/2, "YOU WON");
            attroff(COLOR_PAIR(2));
            refresh();
            mvprintw(max_y/2+4, (max_x-strlen("Press any key to exit"))/2, "Press any key to exit");
            // wait for 3 seconds
            napms(3000);
            // wait for a key press
            nodelay(stdscr, FALSE); // heyhey, now we want the blocking getch
            getch();
            break;
        }

        // create a string
        char quite_Prompt[100] = "Q - quit";

        // draw the essential stuff
        if(boarder_color_reset_counter > 0) boarder_color_reset_counter -= 1;
        else boarder_color_index = 0;

        wattron(win_game,COLOR_PAIR(boarder_color_index));
        box(win_game, 0, 0); // draw the box around the window
        wattroff(win_game,COLOR_PAIR(boarder_color_index));
        // add a status bar at the bottom of the window
        mvwprintw(win_game, win_game_SIZE_Y-1, 3, "Health: %d | Entropy: %d/%d | POWER %d | IMPULSE %d | %c", Aleph->health, Aleph->entropy, game_level_config->entry_requirements[level_index+1], (Aleph->hop_cool_down==0), Aleph->force_impulse_on,ch);
        mvwprintw(win_game,win_game_SIZE_Y-1, (int) (win_game_SIZE_X - strlen(quite_Prompt))/2 , "LEVEL: %d/%d", level_index+1, game_level_config->max_number_of_levels);
        mvwprintw(win_game, win_game_SIZE_Y-1, win_game_SIZE_X-strlen(quite_Prompt)-5, "%s", quite_Prompt);

        // draw at the bottom of the window outside the box in the stdscr
        // a health bar that is right beneath the box in the stdscr
        // draw the health bar
        int health_bar_length = (int) (Aleph->health*1.0/Aleph->max_health*1.0 * (win_game_SIZE_X-2));
        int health_bar_start_x = 2;
        int health_bar_start_y = win_game_SIZE_Y+1;
        // draw the health bar
        wattron(stdscr, COLOR_PAIR(boarder_color_index));
        for(int i = 0; i < health_bar_length; i++)
        {
            mvwaddch(stdscr, health_bar_start_y, health_bar_start_x+i, '#');
        }
        wattroff(stdscr, COLOR_PAIR(boarder_color_index));

        // clear the health bar
        for(int i = health_bar_length; i <= health_bar_start_x+health_bar_length+win_game_SIZE_X; i++)
        {
            mvwaddch(stdscr, health_bar_start_y, health_bar_start_x+i, ' ');
        }

        // draw barcket around the health bar
        wattron(stdscr, COLOR_PAIR(boarder_color_index));
        mvwaddch(stdscr, health_bar_start_y, health_bar_start_x-1, '[');
        mvwaddch(stdscr, health_bar_start_y, health_bar_start_x+win_game_SIZE_X-2, ']');
        wattroff(stdscr, COLOR_PAIR(boarder_color_index));


        // beneath health bar, draw the keyboard control prompts for players
        // use white
        attron(COLOR_PAIR(0));
        char *control_prompt = "W A S D - up left down right | SPACE - hop | F - impulse";
        mvprintw(health_bar_start_y+1, (max_x-strlen(control_prompt))/2, "%s", control_prompt);
        // draw the hints on game rules
        char *game_rules = "Avoid (X), collect (M), heal with (B), and reach the entropy requirement to advance to the next level.";
        mvprintw(health_bar_start_y+2, (max_x-strlen(game_rules))/2, "%s", game_rules);
        attroff(COLOR_PAIR(0));





        


        // plot the agent

        mvwaddch(win_game, Aleph->y, Aleph->x, Aleph->apearances[Aleph->direction]);
        mvwaddch(win_game, Aleph->heading_y, Aleph->heading_x, Aleph->heading[Aleph->direction]);

        // plot the hop trace of the agent as: ### in the direction of the agent's back
        if(Aleph->hop_cool_down > 0)
        {
            int hop_trace_x = Aleph->x;
            int hop_trace_y = Aleph->y;
            int hop_trace_direction = Aleph->direction;
            for(int i = 0; i < Aleph->hop_distance-1; i++)
            {
                switch(hop_trace_direction)
                { // 0 - up, 1 - right, 2 - down, 3 - left
                    case 0:
                        hop_trace_y += 1;
                        break;
                    case 1:
                        hop_trace_x -= 2;
                        break;
                    case 2:
                        hop_trace_y -= 1;
                        break;
                    case 3:
                        hop_trace_x += 2;
                        break;
                }   
                mvwaddch(win_game, hop_trace_y, hop_trace_x, '#');
            }
        }

        // plot the force impulse of the agent if it is on
        if(Aleph->force_impulse_on>0){
            // draw a cube around the agent representing the force impulse radius
            wattron(win_game, COLOR_PAIR(5));
            // use a loop to draw the cube with radius of impulse radius
            for(int i = 1; i <= Aleph->force_impulse_radius; i++)
            {
                mvwaddch(win_game, Aleph->y-i, Aleph->x-i, ' ');
                mvwaddch(win_game, Aleph->y-i, Aleph->x+i, ' ');
                mvwaddch(win_game, Aleph->y+i, Aleph->x-i, ' ');
                mvwaddch(win_game, Aleph->y+i, Aleph->x+i, ' ');
                mvwaddch(win_game, Aleph->y-i, Aleph->x, ' ');
                mvwaddch(win_game, Aleph->y+i, Aleph->x, ' ');
                mvwaddch(win_game, Aleph->y, Aleph->x-i, ' ');
                mvwaddch(win_game, Aleph->y, Aleph->x+i, ' ');
            }

            wattroff(win_game, COLOR_PAIR(5));
        }


        refresh();


        // plot the effectors based on the level

        for (int i = 0; i < number_of_effectors; i++)
        {
            wattron(win_game, COLOR_PAIR(effectors[i]->color_index));
            mvwaddch(win_game, effectors[i]->y, effectors[i]->x, effectors[i]->look);
            wattroff(win_game, COLOR_PAIR(effectors[i]->color_index));
        }
        

        // listen to the keyboard
        nodelay(stdscr, TRUE);
        noecho();
        cbreak();
        ch = getch();

        // keyboard control setting
        switch(ch)
        {
            case 'w':
                Aleph->direction = 0;
                break;
            case 'd':
                Aleph->direction = 1;
                break;
            case 's':
                Aleph->direction = 2;
                break;
            case 'a':   
                Aleph->direction = 3;
                break;
            // when press space, agent will hop by hop distance
            case ' ':
                if(Aleph->hop_cool_down == 0)
                { 
                    Aleph->hop_cool_down = 7;
                    Aleph->speed = Aleph->hop_distance;
                }
                break;
            case 'f': // tur on force impulse
                Aleph->force_impulse_on = 4;
                break;
            case '-': // decrement health.
                Aleph->health -= 1;
                break;
            case '+': // increment health.
                Aleph->health += 1;
                break;
            case 'm': // increment entropy.
                Aleph->entropy += 5;
                break;
            case 'n':  // next level
                level_index += 1;
                break;
        }

        {
        // AGENT MOTION RULES
        // move the agent based on its direction; 0 - up, 1 - right, 2 - down, 3 - left
        switch(Aleph->direction)
        {
            case 0:
                Aleph->y -= Aleph->speed;
                Aleph->heading_x = Aleph->x;
                Aleph->heading_y = Aleph->y-1;
                // double the refresh time
                refresh_time = 2*50;
                break;
            case 1:
                Aleph->x += Aleph->speed;
                Aleph->heading_x = Aleph->x+1;
                Aleph->heading_y = Aleph->y;
                refresh_time = 80;
                break;
            case 2:
                Aleph->y += Aleph->speed;
                Aleph->heading_x = Aleph->x;
                Aleph->heading_y = Aleph->y+1;
                refresh_time = 2*50;
                break;
            case 3:
                Aleph->x -= Aleph->speed;
                Aleph->heading_x = Aleph->x-1;
                Aleph->heading_y = Aleph->y;
                refresh_time = 80;
                break;
        }
        // reset speed to 1;
        Aleph->speed = 1;
        if(Aleph->hop_cool_down > 0) Aleph->hop_cool_down -= 1;
        else Aleph->hop_cool_down = 0;
        }

        
        {// EFFECTOR MOTION RULES (HAUNT SYSTEM)
        for (int i = 0; i < number_of_effectors; i++)
        {
            // if the effector direction is not -1, move the effector
            if(effectors[i]->direction != -1)
            {// change direction if the effector is at the edge of the window
            if(effectors[i]->x > win_game_SIZE_X-3 || effectors[i]->x < 1 || effectors[i]->y > win_game_SIZE_Y-3 || effectors[i]->y < 1)
            {
                // change to opposite direction
                switch(effectors[i]->direction)
                {
                    case 0:
                        effectors[i]->direction = 2;
                        break;
                    case 1:
                        effectors[i]->direction = 3;
                        break;
                    case 2:
                        effectors[i]->direction = 0;
                        break;
                    case 3:
                        effectors[i]->direction = 1;
                        break;
                }
            }

            // move the effectors based on their direction
            changeEffectorPosition(effectors[i], 1);
            
            // if the effector is a radicalist, if will have 3/5 of change to follow the agent and otherwise follow a random direction
            if(effectors[i]->id == 0)
            {
                int probability_deno = 10;
                int probability_nom = 4;
                int random_number_0_to_2 = random_number(0,probability_deno-1);
                if(random_number_0_to_2 < probability_nom)
                {
                int relative_dir = relative_direction(effectors[i], Aleph);
                if(relative_dir != -1) effectors[i]->direction = relative_dir;
                }
                else effectors[i]->direction = random_number(0,3);

                // if the distance between the agent and the effector is less than 2, the radicalist will have 2/3 change to hop 4 steps in that direction
                int hop_activation_distance = 6;
                if(abs(effectors[i]->x - Aleph->x) < hop_activation_distance && abs(effectors[i]->y - Aleph->y) < hop_activation_distance)
                {
                    int probability_deno = 10;
                    int probability_nom = 6;
                    int random_number_0_to_2 = random_number(0,probability_deno-1);
                    if(random_number_0_to_2 < probability_nom)
                    {
                        
                        // hop direction
                        int hop_dir = relative_direction(effectors[i], Aleph);
                        effectors[i]->direction = hop_dir;
                        // hop distance being equal to the distance on the direction between the agent and the effector
                        int hop = 4;
                        // switch(hop_dir)
                        // {
                        //     case 0:
                        //         hop = abs(effectors[i]->y - Aleph->y);
                        //         break;
                        //     case 1:
                        //         hop = abs(effectors[i]->x - Aleph->x);
                        //         break;
                        //     case 2:
                        //         hop = abs(effectors[i]->y - Aleph->y);
                        //         break;
                        //     case 3:
                        //         hop = abs(effectors[i]->x - Aleph->x);
                        //         break;
                        // }
                        // hop towards the agent
                        changeEffectorPosition(effectors[i], hop);

                    }
                }

                // // finally, check if the radicalist is in the range of an on force impulse agent
                // // distance defined by min(dis x, dis y), explicitly compute
                // int distance_e_a = abs(effectors[i]->x - Aleph->x) < abs(effectors[i]->y - Aleph->y) ? abs(effectors[i]->x - Aleph->x) : abs(effectors[i]->y - Aleph->y);
                // if(Aleph->force_impulse_on > 0 && distance_e_a <= Aleph->force_impulse_radius)
                // {
                //     // if so, the radicalist will be paralyzed for n turns.
                //     int n = 3;
                //     effectors[i]->paralyzed_count_down = n;
                // }
                
            }
            
            }
            
        }
        // set force impulse off
        if(Aleph->force_impulse_on > 0) 
        {
            // decrease force impulse by 1
            Aleph->force_impulse_on -= 1;
            // decrease radius by 1 when the impulse is less than or equal to radius
            if(Aleph->force_impulse_radius >= Aleph->force_impulse_on) Aleph->force_impulse_radius -= 1;
        }
        else
        {
            Aleph->force_impulse_on = 0;
            Aleph->force_impulse_radius = Aleph->force_impulse_default_radius;

        }
        }
        // check if the agent is out of the window
        // if so move it back to the center after reducing its health by 1
        if(Aleph->x > win_game_SIZE_X-2 || Aleph->x < 1 || Aleph->y > win_game_SIZE_Y-2 || Aleph->y < 1)
        {
            Aleph->x = random_number(5, win_game_SIZE_X-5);
            Aleph->y = random_number(5, win_game_SIZE_Y-5);
            Aleph->health -= 1;
            // set boarder color to red
            boarder_color_index = 1;
            boarder_color_reset_counter = 5;
        }

        // check for collision betwen the agent and effectors
        for (int i = 0; i < number_of_effectors; i++)
        {
            if(Aleph->x == effectors[i]->x && Aleph->y == effectors[i]->y)
            {
                // amend the agent's health and entropy based on the effector
                Aleph->health += effectors[i]->delta_health;
                Aleph->entropy += effectors[i]->delta_entropy;

                // if agent's current health is greater than max health, set max health to current health
                if(Aleph->health > Aleph->max_health) Aleph->max_health = Aleph->health;

                if(effectors[i]->delta_health >= 0)
                {
                    boarder_color_index = effectors[i]->color_index; // green
                    boarder_color_reset_counter = 5;
                    // if the agent touched a non-harmful effector, replace the effector with null effector
                    effectors[i] = init_NullEffector(effectors[i]->x, effectors[i]->y);
                    
                    
                }
                else if(effectors[i]->delta_health<0)
                {
                    // if the agent touched a harmful effector, relocate the effector
                    effectors[i]->x = random_number(5, win_game_SIZE_X-5);
                    effectors[i]->y = random_number(5, win_game_SIZE_Y-5);

                    boarder_color_index = 1; // red
                    boarder_color_reset_counter = 5;

                    // when taking damage, pause the game for 0.1 second
                    napms(100);
                }
            }
        }

        

        if(Aleph->health <= Aleph->critical_health)
        {
            // red
            boarder_color_index = 1;
            boarder_color_reset_counter = 1;
        }

        // check if we need to go to the next level
        // RULE: if the agent's entropy is greater than or equal to the entry requirement of the next level, go to the next level
        if(level_index+1 < game_level_config->max_number_of_levels)
        if(Aleph->entropy >= game_level_config->entry_requirements[level_index+1])
        {
            level_index += 1;
            game_level_config = set_GAMELEVEL(level_index); // update game config

            // update the number of effectors
            number_of_effectors = game_level_config->number_of_radicalists + game_level_config->number_of_motherboards + game_level_config->number_of_batterytrucks;
            
            // redefine the list length of effectors
            effectors = realloc(effectors, number_of_effectors*sizeof(EFFECTOR*));
            
            // initialize the effectors
            for (int i = 0; i < game_level_config->number_of_radicalists; i++)
            {
                int init_effector_x = random_number(5, win_game_SIZE_X-5);
                int init_effector_y = random_number(5, win_game_SIZE_Y-5);
                effectors[i] = init_Radicalist(init_effector_x, init_effector_y);
            }
            for (int i = game_level_config->number_of_radicalists; i < game_level_config->number_of_radicalists + game_level_config->number_of_motherboards; i++)
            {
                int init_effector_x = random_number(5, win_game_SIZE_X-5);
                int init_effector_y = random_number(5, win_game_SIZE_Y-5);
                effectors[i] = init_MotherBoard(init_effector_x, init_effector_y);
            }
            for (int i = game_level_config->number_of_radicalists + game_level_config->number_of_motherboards; i < number_of_effectors; i++)
            {
                int init_effector_x = random_number(5, win_game_SIZE_X-5);
                int init_effector_y = random_number(5, win_game_SIZE_Y-5);
                effectors[i] = init_BatteryTruck(init_effector_x, init_effector_y);
            }
        }




        // refresh the window (Event loop control)
        wrefresh(win_game);
        napms(refresh_time);// wait for 1 second
        wclear(win_game);// clear the window


        if(ch == 'q')
        {
            // give a prompt asking if the user wants to quit the game
            // clear the entire stdscr
            clear();
            // clean everything in the game window
            wclear(win_game);
            wrefresh(win_game);
            // print
            mvprintw(max_y/2, (max_x-strlen("Game PAUSED"))/2, "Game PAUSED");
            // Press Y to quit, press N to continue
            // use green
            attron(COLOR_PAIR(1));
            mvprintw(max_y/2+1, (max_x-strlen("Y to quit"))/2, "Y to quit");
            attroff(COLOR_PAIR(1));
            // use red
            attron(COLOR_PAIR(2));
            mvprintw(max_y/2+2, (max_x-strlen("Otherwise to continue"))/2, "Otherwise to continue");
            attroff(COLOR_PAIR(2));
            // wait for a key press
            nodelay(stdscr, FALSE); // heyhey, now we want the blocking getch
            ch = getch();
            if(ch == 'y')
                break;
            else{
                // clear these
                clear();
                wclear(win_game);
                wrefresh(win_game);
                // reset ch to 'q'
            }
        }
    }

    endwin(); // end the window

    
}


// compile with: gcc -o game_macOS game_macOS.c -lncurses -lportaudio
// run with: ./game_macOS