#define UNIX
#define WHOLE_BOARD
// #define DEBUG
// #define DEBUG_NETWORK
// #define DEBUG_OF
// #define DEBUG_MATRIX
// #define DEBUG_TESTING
// #define DEBUG_PROB
// #define DEBUG_CROSSOVER
// #define DEBUG_MUTATE
// #define DEBUG_EVOLVE
#define DEBUG_SAVE
// #define DEBUG_SUMMON
#define DEBUG_PLAY
#define DEBUG_TEST
#define DEBUG_MOVE

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <signal.h>
#include <string>

#ifdef UNIX
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "pawn.h"

#ifndef SIGUSR1
#define SIGUSR1 10
#endif

#define EXEC "exec"

#ifdef WHOLE_BOARD
#define INPUT_LEN X_LIM * Y_LIM
#else
#define INPUT_LEN X_LIM * 2
#endif

#define OUTPUT_LEN X_LIM * 3
#define HIDDEN_LEN X_LIM * 2
#define HIDDEN_NUM 3

#if HIDDEN_NUM != 0
#define CONNECT_NUM HIDDEN_LEN * (INPUT_LEN + HIDDEN_NUM*HIDDEN_LEN + OUTPUT_LEN)
#else
#define CONNECT_NUM INPUT_LEN * OUTPUT_LEN
#endif

#define POPULATION_NUM 100
// #define POPULATION_NUM 10
#define GENERATIONS_NUM 10000
// #define GENERATIONS_NUM 10
#define CROSSOVER_PROB 60
#define MUTATION_PROB 10
#define MUTATION_SCALE 1
#define ELITE_LEN 10

#define PROCESS_NUM 6
#define SEM_GUYS "SEM_GUYS"
#define SIG_ORDER "SIG_ORDER"

using namespace std;

struct Action{
    int pawn;
    Move move;
};

struct Guy{
    int fitness;
    double genotype[CONNECT_NUM];
};

struct GIndex{
    int index;
};

enum Order {NO_ORDER, GEN_INIT_POP, GEN_POP, REGEN_POP, TEST_POP, TERMINATE};

Action actions[] = {
    {0, SE}, {0, South}, {0, SW},
    {1, SE}, {1, South}, {1, SW},
    {2, SE}, {2, South}, {2, SW},
    {3, SE}, {3, South}, {3, SW},
    {4, SE}, {4, South}, {4, SW},
    {5, SE}, {5, South}, {5, SW},
    {6, SE}, {6, South}, {6, SW},
    {7, SE}, {7, South}, {7, SW},
    {-1, X_Move}
};

string filepath = "AI/";
string filename = "player";

bool play = false;

int execution = 0;
int turns;

pid_t mainProc;
GIndex *guyIndex = NULL;
Guy *pawnStars = NULL;
Guy oldPop[POPULATION_NUM];
Order *order = NULL;
int *compOrder = NULL;

vector<int> prob;

int process;

#ifdef UNIX
int guyIshmid = -1, pawnSshmid = -1, ordershmid = -1, compOshmid = -1;
sem_t *sem_guys, *sig_order;
#endif

bool compare_guys(Guy g1, Guy g2){
    return g1.fitness > g2.fitness;
}

/**
 * @brief Saves given Individual's genotype in file <filepath>/<filename><execution>.ai.
 * 
 * @param guy Individual to save
 */
void save_guy(Guy& guy){
    string path = filepath + filename + to_string(execution) + ".ai";

    FILE *pawnFile = fopen(path.c_str(), "w");

    const unsigned char *doubleByte;
    for(int i = 0; i < CONNECT_NUM; i++){
        doubleByte = reinterpret_cast<const unsigned char*>(guy.genotype + i);

        for(unsigned long j = 0; j < sizeof(double); j++){
            fprintf(pawnFile, "%c", doubleByte[j]);
        }
    }

    fclose(pawnFile);

    #ifdef DEBUG_SAVE
    cout << "Saved: (" << guy.fitness << ")";
    // for(int i = 0; i < CONNECT_NUM; i++){
    //     cout << " " << guy.genotype[i];
    // }
    cout << "\n";
    #endif
}

/**
 * @brief Reads genotype of an individual stored in file named <filepath>/<filename><execution>.ai.
 * 
 * @param execution execution in which Individual was created
 * @return Guy - recovered Individual
 */
Guy summon(int execution){
    string path = filepath + filename + to_string(execution) + ".ai";

    FILE *pawnFile = fopen(path.c_str(), "r");

    Guy guy;
    guy.fitness = -1;

    char doubleByte[sizeof(double)];
    for(int i = 0; i < CONNECT_NUM; i++){
        for(unsigned long j = 0; j < sizeof(double); j++){
            doubleByte[j] = fgetc(pawnFile);

            if(doubleByte[j] == EOF){
                cout << "Something's gone wrong reading ai file\n";
                fclose(pawnFile);
                return guy;
            }
        }

        memcpy(guy.genotype + i, doubleByte, sizeof(double));
    }

    fclose(pawnFile);

    #ifdef DEBUG_SUMMON
    cout << "Summoned:";
    for(int i = 0; i < CONNECT_NUM; i++){
        cout << " " << guy.genotype[i];
    }
    cout << "\n";
    #endif

    guy.fitness = 0;
    return guy;
}

/**
 * @brief Transformes given action to match a Black piece's action.
 * This function is used to correct a non-player behaviour.
 * 
 * @param act White piece's action
 * @return Action a Black piece should take
 */
Action white_to_black_action(Action& act){
    switch(act.move){
        case SE:
            act.move = NE;
            break;

        case South:
            act.move = North;
            break;

        case SW:
            act.move = NW;
            break;

        default:
            act.move = X_Move;
            break;
    }

    return act;
}

/**
 * @brief Calculates guy Individual's neuronal output to current problem.
 * 
 * @param guy index in pawnStars of individual solving problem
 * @param playsW wether Individual is playing White pieces
 * @return Action - Action to preform
 */
Action network(int guy, bool playsW){
    int max_layer = OUTPUT_LEN;
    if(max_layer < INPUT_LEN) max_layer = INPUT_LEN;
    if(max_layer < HIDDEN_LEN) max_layer = HIDDEN_LEN;

    double *last_out = (double*) malloc(max_layer * sizeof(double));
    if(last_out == NULL) return actions[24];

    double *out = (double*) malloc(max_layer * sizeof(double));
    if(out == NULL) return actions[24];

    double *temp;

    #ifdef WHOLE_BOARD
    if(playsW){
        int lo = 0;
        for(vector<Piece> row: get_board()){
            for(Piece p: row) last_out[lo++] = p;
        }
    }
    else{
        int lo = INPUT_LEN;
        for(vector<Piece> row: get_board()){
            for(Piece p: row) last_out[--lo] = p;
        }
    }
    #else
    int lo = 0;
    if(playsW){
        // Friendly Pieces.
        for(schar w: get_whites()){
            last_out[lo++] = w;
        }
        // Enemy Pieces
        for(schar b: get_blacks()){
            last_out[lo++] = b;
        }
    }
    else{
        // Friendly Pieces
        // We want to reverse the row because Individuals are trained to move white pieces
        for(schar b: get_blacks()){
            schar bs = (Y_LIM - (b & 0b111) - 1) | (b & 0b111000);
            last_out[lo++] = bs;
        }
        // Enemy Pieces
        for(schar w: get_whites()){
            schar ws = (Y_LIM - (w & 0b111) - 1) | (w & 0b111000);
            last_out[lo++] = ws;
        }
    }
    #endif

    #ifdef DEBUG_NETWORK
    cout << ((playsW) ? "White" : "Black");

    for(int j = 0; j < INPUT_LEN; j++){
        schar pos = last_out[j];
        cout << "  ";
        if(pos == -1) cout << -1;
        else cout << (char)(((char) (pos >> 3)) + 'A') << (char)(((char) (pos & 0b111)) + '1');
    }
    cout << "\n";
    #endif

    int last_layer = INPUT_LEN, i = 0;

    for(int h = 0; h < HIDDEN_NUM; h++){  
        
        double o;
        for(int j = 0; j < HIDDEN_LEN; j++){
            o = 0.;
            for(int k = 0; k < last_layer; k++, i++){
                o += last_out[k] * pawnStars[guy].genotype[i];
            }

            out[j] = tanh(o);
        }

        #ifdef DEBUG_NETWORK
        cout << "HL " << h << " ";

        for(int j = 0; j < HIDDEN_LEN; j++){
            cout << " " << (double) out[j];
        }
        cout << "\n";
        #endif

        last_layer = HIDDEN_LEN;
        temp = last_out;
        last_out = out;
        out = temp;
    }

    double max_val = -100;
    int max_ind = -1;

    for(int j = 0; j < OUTPUT_LEN; j++){
        for(int k = 0; k < last_layer; k++, i++){
            out[j] += last_out[k] * pawnStars[guy].genotype[i];
        }

        if(max_val < out[j]){
            max_val = out[j];
            max_ind = j;
        }
    }

    #ifdef DEBUG_NETWORK
    cout << "Output";

    for(int j = 0; j < OUTPUT_LEN; j++){
        cout << " " << (double) out[j];
    }

    cout << "\nMove " << actions[max_ind].pawn << " " << move_to_str(actions[max_ind].move) << "\n";
    #endif

    free(last_out);
    free(out);

    if(!playsW) return white_to_black_action(actions[max_ind]);
    return actions[max_ind];
}

/**
 * @brief Individual's objective function.
 * 
 * @param turns number of turns taken by Individual.
 * @return int - fitness.
 */
int only_fans(int turns){
    int reward = 0;
    
    if(end_game() == 0 && get_w_points() >= get_b_points()){
        // If Individual's won, Reward them based on their score, and penalize them based on time taken.
        reward = get_w_points() << 6;

        reward -= turns;

        #ifdef DEBUG_OF
        cout << "Won\n";
        #endif
    }
    else{
        // If Individual's lost, Reward them based on how long they survived for.
        reward = turns;

        reward += get_w_points() << 4;

        #ifdef DEBUG_OF
        cout << "Lost\n";
        #endif
    }
    
    #ifdef DEBUG_OF
    cout << "    Point dif: " << (get_w_points() - get_b_points()) << "\n"
         << "    Turns: " << turns << "\n"
         << "    Reward: " << reward << "\n";
    #endif

    return reward;
}

/**
 * @brief Simulates a game with given Individual.
 * 
 * @param guy index in guys for Individual being tested.
 * @param opponent index in guys for Individual's opponent.
 * @return int - Individual's fitness.
 */
int matrix(int guy, int opponent){
    init_board();
    
    Action act;
    int can_move, turns = 0;
    while((can_move = end_game()) != 0){
        // Let Individual play a turn
        if(can_move & 1){
            act = network(guy, true);

            if(!move_white(act.pawn, act.move)) break;
        
            turns++;
        }

        #ifdef DEBUG_MATRIX
        print_board();
        #endif

        // Let opponent play a turn
        if(can_move & 2){
            act = network(opponent, false);

            if(!move_black(act.pawn, act.move)){
                move_any_black();
            }
        }

        #ifdef DEBUG_MATRIX
        print_board();
        #endif
    }

    return only_fans(turns);
}

/**
 * @brief Generates initial population setting genotype to random values between -1 and 1 in every connection's weight.
 * Coordinated with other processes.
 */
void gen_init_pop_proc(){
    sem_wait(sem_guys);

    while(guyIndex->index < POPULATION_NUM){
        // Get next guy
        guy = (guyIndex->index)++;
        sem_post(sem_guys);

        for(int i = 0, random; i < CONNECT_NUM; i++){
            random = rand();

            // Assign random values to this
            pawnStars[guy].genotype[i] = (random & 0b1) ? ((random >> 1) % 100) / 100. : -(((random >> 1) % 100) / 100.);
        }

        #ifdef DEBUG_INIT_POP
        cout << "Ind " << guy << ":";
        for(int i = 0; i < CONNECT_NUM; i++){
            cout << " " << pawnStars[guy].genotype[i];
        }
        cout << "\n";
        #endif

        sem_wait(sem_guys);
        // Mark order's item as complete
        (*compOrder)++;
    }

    sem_post(sem_guys);
}

/**
 * @brief Generates initial population setting genotype to random values between -1 and 1 in every connection's weight.
 * 
 * @return true if successful
 * @return false if failed 
 */
bool generate_init_population(){
    if(pawnStars == NULL) return false;
    
    #ifdef UNIX
    if(guyIndex == NULL) return false;

    guyIndex->index = 0;
    *compOrder = 0;

    // Define order to give to processes
    *order = GEN_INIT_POP;
    sem_post(sig_order);

    // Wait for all items relevant to given order to be completed
    sem_wait(sem_guys);
    while(*compOrder < POPULATION_NUM){
        sem_post(sem_guys);
        sem_wait(sem_guys);
    }

    // Reset order values
    *compOrder = 0;
    *order = NO_ORDER;
    
    #else

    for(int guy = 0; guy < POPULATION_NUM; guy++){
        for(int i = 0, random; i < CONNECT_NUM; i++){
            random = rand();

            pawnStars[guy].genotype[i] = (random & 0b1) ? ((random >> 1) % 100) / 100. : -(((random >> 1) % 100) / 100.);
        }

        #ifdef DEBUG_INIT_POP
        cout << "Ind " << guy << ":";
        for(int i = 0; i < CONNECT_NUM; i++){
            cout << " " << pawnStars[guy].genotype[i];
        }
        cout << "\n";
        #endif
    }
    
    #endif

    return true;
}

/**
 * @brief Tests a generation of Individuals.
 * Coordinated with other processes.
 */
void test_pop_proc(){
    sem_wait(sem_guys);

    while(guyIndex->index < POPULATION_NUM){
        // Get next guy
        guy = (guyIndex->index)++;

        #ifdef DEBUG_TESTING
        cout << "New test: " << process << " " << guy << "\n";
        #endif

        sem_post(sem_guys);

        // Simulate guy's game
        pawnStars[guy].fitness = matrix(guy, 0);

        sem_wait(sem_guys);
        // Mark order's item as complete
        (*compOrder)++;
    }

    sem_post(sem_guys);
}

/**
 * @brief Tests a generation of Individuals.
 */
void test_population(){
    if(pawnStars == NULL) return;
    
    #ifdef UNIX
    if(guyIndex == NULL || PROCESS_NUM <= 0) return;

    guyIndex->index = 0;
    *compOrder = 0;

    // Define order to give to processes
    *order = TEST_POP;
    sem_post(sig_order);

    // Wait for all items relevant to given order to be completed
    sem_wait(sem_guys);
    while(*compOrder < POPULATION_NUM){
        sem_post(sem_guys);
        sem_wait(sem_guys);
    }

    // Reset order values
    *compOrder = 0;
    *order = NO_ORDER;
    
    #else

    for(int guy = 0; guy < POPULATION_NUM; guy++){
        pawnStars[guy].fitness = matrix(guy, 0);
    }

    #endif
}

// TODO: Understand this
/**
 * @brief Creates a vector with population's indexes organized by "normalized" fitness.
 * 
 * @return vector<int> hard to explain.
 */
vector<int> generate_prob(){
    // Normalize (assuming it's sorted)
    for(int i = POPULATION_NUM, fit, last_fit = 0, last_som = 1; i--;){
        fit = pawnStars[i].fitness;
        
        if(fit >= 0){
            if(((fit - last_fit) >> 3) > 0){
                last_som += (fit - last_fit);
                last_fit = fit;
            }

            pawnStars[i].fitness = last_som;
        }
        else{
            pawnStars[i].fitness = 1;
        }
    }

    // Sum new fits
    int sum_fit = 0;
    for(int i = 0; i < POPULATION_NUM; i++){
        sum_fit += pawnStars[i].fitness;
    }

    // Create prob vector
    vector<int> prob;
    prob.reserve(sum_fit);

    for(int i = 0; i < POPULATION_NUM; i++){
        vector<int> newGuy = vector<int>(pawnStars[i].fitness, i);

        prob.insert(prob.end(), newGuy.begin(), newGuy.end());
    }

    #ifdef DEBUG_PROB
    cout << "Prob: ";
    for(int p: prob){
        cout << " " << p;
    }
    cout << "\n";
    #endif

    return prob;
}

/**
 * @brief Selects a random Individual from population.
 * Probability of each Individual being selected is proportional to their fitness.
 * 
 * @param pops vector generated by generate_prob function.
 * @return int - index of selected parent
 */
int roulette(){
    return prob[rand()%prob.size()];
}

/**
 * @brief Set given Individual's genotype to be a mix of father and mother.
 * 
 * @param guy child's index
 * @param f father's index
 * @param m mother's index
 */
void crossover(int guy, int f, int m){
    Guy father = oldPop[f], mother = oldPop[m];
    
    // Normalize fitness
    int min_fit = father.fitness;
    if(mother.fitness < min_fit) min_fit = mother.fitness;

    father.fitness -= min_fit - 1;
    mother.fitness -= min_fit - 1;

    // Total fit should never be 0 because of normalization
    int total_fit = father.fitness + mother.fitness;

    #ifdef DEBUG_CROSSOVER
    cout << "Inheritance:";
    #endif
    
    // Inherit each connection's weight from either father or mother
    for(int i = 0; i < CONNECT_NUM; i++){
        pawnStars[guy].genotype[i] = (rand()%total_fit < father.fitness) ? father.genotype[i] : mother.genotype[i];

        #ifdef DEBUG_CROSSOVER
        cout << ((pawnStars[guy].genotype[i] == father.genotype[i]) ? " F" : " M");
        #endif
    }
    
    #ifdef DEBUG_CROSSOVER
    cout << "\n";
    #endif
}

/**
 * @brief Change weight of connections between layers by a random value on a set probability.
 * Probability of mutation is set in MUTATION_PROB.
 * Scape of mutation is set in MUTATION_SCALE.
 * 
 * @param guy mutated Individual's index
 */
void mutate(int guy){
    #ifdef DEBUG_MUTATE
    double pre;
    cout << "Mutate:\n";
    #endif

    // Each connection between layers has a predefined chance
    //  of being altered by a random value in a predefined scale
    for(int i = 0; i < CONNECT_NUM; i++){
        if(rand()%100 < MUTATION_PROB){
            #ifdef DEBUG_MUTATE
            pre = pawnStars[guy].genotype[i];
            #endif

            pawnStars[guy].genotype[i] = MUTATION_SCALE * (-1 * rand()%2) * (rand()%100) / 100.;

            #ifdef DEBUG_MUTATE
            cout << "\t" << i << " " << pre - pawnStars[guy].genotype[i] << "\n";
            #endif
        }
    }
}

/**
 * @brief Creates new population through crossover and mutation.
 * Maintains an elite of a predefined number of Individuals.
 * Coordinated with other processes.
 */
void regen_pop_proc(){
    srand(time(NULL)+i);

    sem_wait(sem_guys);

    while(guyIndex->index < POPULATION_NUM){
        guy = (guyIndex->index)++;
        sem_post(sem_guys);

        // Crossover
        if(rand()%100 < CROSSOVER_PROB){
            crossover(guy, roulette(), roulette());
        }

        // Mutation
        mutate(guy);

        sem_wait(sem_guys);
        // Mark order's item as complete
        (*compOrder)++;
    }
    
    sem_post(sem_guys);
}

/**
 * @brief Creates new population through crossover and mutation.
 * Maintains an elite of a predefined number of Individuals.
 */
void repopulate(){
    if(pawnStars == NULL) return;
    
    #ifdef UNIX
    if(guyIndex == NULL) return;

    guyIndex->index = ELITE_LEN;
    *compOrder = ELITE_LEN;

    // Define order to give to processes
    *order = REGEN_POP;
    sem_post(sig_order);

    // Wait for all items relevant to given order to be completed
    sem_wait(sem_guys);
    while(*compOrder < POPULATION_NUM){
        sem_post(sem_guys);
        sem_wait(sem_guys);
    }

    // Reset order values
    *compOrder = 0;
    *order = NO_ORDER;

    #else

    for(int guy = 0; guy < POPULATION_NUM; guy++){
        // Crossover
        if(rand()%100 < CROSSOVER_PROB){
            crossover(guy, roulette(), roulette());
        }

        // Mutation
        mutate(guy);
    }

    #endif
}

/**
 * @brief Creates a predefined number of generations starting with given progenitor.
 * 
 * @param progenitor first Individual that will train the rest of the population
 */
void evolve(Guy& progenitor){
    execution++;

    #ifdef DEBUG_EVOLVE
    cout << "Started execution " << execution << ".\n";
    #endif

    if(!generate_init_population()){
        cout << "ERROR: Failed to generate Initial population!";
        return;
    }
    
    if(progenitor.fitness != -1){
        *pawnStars = progenitor;
    }

    #ifdef DEBUG_EVOLVE
    cout << "Finished creating initial population.\n";
    #endif

    test_population();

    #ifdef DEBUG_EVOLVE
    cout << "Finished testing initial population.\n";
    #endif

    sort(pawnStars, pawnStars+POPULATION_NUM, compare_guys);

    #ifdef DEBUG_EVOLVE
    cout << "Sorted Individuals by fitness.\n";
    #endif

    // Repeat crossover, evaluation and all that jazz for each Generation
    for(int gen = 0; gen < GENERATIONS_NUM; gen++){
        for(int i = 0; i < POPULATION_NUM; i++){
            oldPop[i] = pawnStars[i];
        }

        #ifdef DEBUG_EVOLVE
        cout << "Duplicated generation " << gen << ".\n";
        #endif

        // Parent selection
        prob = generate_prob();

        // Create next generation
        repopulate();

        // Test created generation
        test_population();

        #ifdef DEBUG_EVOLVE
        cout << "Finished testing generation " << gen+1 << ".\n";
        #endif

        sort(pawnStars, pawnStars+POPULATION_NUM, compare_guys);
    }

    // Save best Individual
    save_guy(*pawnStars);
}

#ifdef UNIX
/**
 * @brief Waits for an order. Acts accordingly to received order.
 */
void work(){
    while(*order != TERMINATE){
        sem_wait(sig_order);
        
        switch(*order){
            case GEN_INIT_POP:
                // Generate initial population
                gen_init_pop_proc();
                break;

            case GEN_POP:
                // TODO: Generate population
                break;

            case REGEN_POP:
                // TODO: Re-generate population
                regen_pop_proc();
                break;

            case TEST_POP:
                // Test population
                test_pop_proc();
                break;

            case TERMINATE:
                // Terminate process
                exit(0);

            case NO_ORDER:
                // No orders
                break;

            default:
                // Unknown Order
                break;
        }
    }

    exit(0);
}

/**
 * @brief Create task sharing processes
 * 
 * @param n_procs number of processes to create
 */
void create_procs(int n_procs){
    process = 0;

    for(int proc = 1; proc < n_procs; proc++){
        if(fork() == 0){
            process = proc;

            work();

            exit(0);
        }
    }
}
#endif

/**
 * @brief Detaches shared memory and semaphore.
 */
void clear(int sig){
    #ifdef UNIX
    if(process != 0) exit(0);
    #endif

    switch(sig){
        case SIGINT:
            cout << " SIGINT received, terminating\n";
            break;

        case SIGUSR1:
            cout << "Program finished execution normally\n";
            break;
        
        default:
            cout << "Program ran into an unexpected error\n";
            break;
    }

    if(play){
        if(pawnStars) free(pawnStars);

        exit(0);
    }
    
    FILE *execfile = fopen(EXEC, "w");
    fprintf(execfile, "%d", execution);
    fclose(execfile);
    
    #ifdef UNIX
    while(wait(NULL) != -1);

    if(getpid() != mainProc) exit(0);

    if(guyIndex != NULL){
        shmdt(guyIndex);
        guyIndex = NULL;
    }
    if(guyIshmid >= 0){
        shmctl(guyIshmid, IPC_RMID, NULL);
        guyIshmid = -1;
    }

    if(pawnStars != NULL){
        shmdt(pawnStars);
        pawnStars = NULL;
    }
    if(pawnSshmid >= 0){
        shmctl(pawnSshmid, IPC_RMID, NULL);
        pawnSshmid = -1;
    }

    if(order != NULL){
        shmdt(order);
        order = NULL;
    }
    if(ordershmid >= 0){
        shmctl(ordershmid, IPC_RMID, NULL);
        ordershmid = -1;
    }

    if(compOrder != NULL){
        shmdt(compOrder);
        compOrder = NULL;
    }
    if(compOshmid >= 0){
        shmctl(compOshmid, IPC_RMID, NULL);
        compOshmid = -1;
    }

    if(sem_guys){
        sem_close(sem_guys);
        sem_unlink(SEM_GUYS);
        sem_guys = NULL;
    }

    if(sig_order){
        sem_close(sig_order);
        sem_unlink(SIG_ORDER);
        sig_order = NULL;
    }

    #else

    if(guyIndex != NULL){
        free(guyIndex);
        guyIndex = NULL;
    }

    if(pawnStars != NULL){
        free(pawnStars);
        pawnStars = NULL;
    }

    #endif

    exit(0);
}

/**
 * @brief Initiates shared memory and necessary semaphore.
 * 
 * @return int - 0 if successful, otherwise, type of error.
 */
int init(){
    FILE *execfile = fopen(EXEC, "r");
    fscanf(execfile, "%d", &execution);
    fclose(execfile);

    srand(time(NULL));
    
    #ifdef UNIX
    mainProc = getpid();

    if((guyIshmid = shmget(IPC_PRIVATE, sizeof(GIndex), IPC_CREAT | 0766)) < 0){
		return -1;
    }
    if((guyIndex = (GIndex*) shmat(guyIshmid, NULL, 0)) == (GIndex*)-1){
        guyIndex = NULL;
		return -2;
    }

    if((pawnSshmid = shmget(IPC_PRIVATE, POPULATION_NUM * sizeof(Guy), IPC_CREAT | 0766)) < 0){
		return -3;
    }
    if((pawnStars = (Guy*) shmat(pawnSshmid, NULL, 0)) == (Guy*)-1){
        pawnStars = NULL;
		return -4;
    }

    if((ordershmid = shmget(IPC_PRIVATE, sizeof(Order), IPC_CREAT | 0766)) < 0){
		return -5;
    }
    if((order = (Order*) shmat(ordershmid, NULL, 0)) == (Order*)-1){
        order = NULL;
		return -6;
    }
    *order = NO_ORDER;

    if((compOshmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0766)) < 0){
		return -5;
    }
    if((compOrder = (int*) shmat(compOshmid, NULL, 0)) == (int*)-1){
        compOrder = NULL;
		return -6;
    }
    *compOrder = 0;

    sem_unlink(SEM_GUYS);
	if((sem_guys = sem_open(SEM_GUYS, O_CREAT|O_EXCL, 0700, 1)) == SEM_FAILED){
        sem_guys = NULL;
        return -7;
    }

    sem_unlink(SIG_ORDER);
	if((sig_order = sem_open(SIG_ORDER, O_CREAT|O_EXCL, 0700, 0)) == SEM_FAILED){
        sig_order = NULL;
        return -8;
    }
    #else

    pawnStars = (Guy*) malloc(POPULATION_NUM * sizeof(Guy));

    #endif

    return 0;
}

/**
 * @brief Receives input from user to play a match against selected Individual.
 */
void play_match(){
    int canMove;
    Action act;
    char col, row;
    Move move;
    string m;

    init_board();

    #ifdef DEBUG_PLAY
    cout << "Game Start\n";
    #endif

    while((canMove = end_game()) != 0){
        turns++;
        
        // Let Opponent take a turn
        if(canMove & 1){
            act = network(0, true);

            if(!move_white(act.pawn, act.move)){
                #ifdef DEBUG_PLAY
                cout << "Opponent tried illegal move\n";
                #endif
                
                move_any_white();
            }
        
        }

        // Let player take a turn
        if(canMove & 2){
            print_board();
            
            do{
                cout << "Action:\n";
                fflush(stdout);
                
                cin >> col;
                if(col == '0'){
                    cout << "Player gave up.\n";
                    return;
                }

                cin >> row;
                if(row == '0'){
                    cout << "Player gave up.\n";
                    return;
                }

                cin >> m;
                if(m.compare("0") == 0){
                    cout << "Player gave up.\n";
                    return;
                }

                move = str_to_move(m);
            } while(!player_move_black(col, row, move));
        }
    }

    if(get_b_points() > get_w_points()){
        cout << "!YOU WIN!\n";
    }
    else if(get_b_points() < get_w_points()){
        cout << "!YOU LOSE!\n";
    }
    else cout << "TIE\n";
}

/**
 * @brief Tests a selected individual against itself.
 */
void test_guy(){
    init_board();

    Action act;
    int can_move;
    while((can_move = end_game()) != 0){
        // Let Individual play a turn
        if(can_move & 1){
            act = network(0, true);

            if(!move_white(act.pawn, act.move)){
                cout << "W Invalid move\n";
                move_any_white();
            }
        }

        #ifdef DEBUG_TEST
        print_board();
        #endif

        // Let opponent play a turn
        if(can_move & 2){
            act = network(0, false);

            if(!move_black(act.pawn, act.move)){
                cout << "B Invalid move\n";
                move_any_black();
            }
        }

        #ifdef DEBUG_TEST
        print_board();
        #endif
    }

    cout << "W " << get_w_points() << "\nB " << get_b_points() << "\n";
}

int main(int argc, char *argv[]){
    signal(SIGINT, clear);

    if(argc <= 1){
        cout << "./star train\n"
             << "./star train <execution>\n"
             << "./star play <execution>\n";
        
        return 0;
    }

    // Training
    if(strcmp(argv[1], "train") == 0){
        Guy pro;

        if(argc > 2){
            int exec;
            sscanf(argv[2], "%d", &exec);
            
            if(exec == 0){
                cout << "ERROR: " << argv[2] << " is not a valid execution!\n";
                return 0;
            }
            
            pro = summon(exec);
        
            cout << "Starting training with " << exec << " as progenitor.\n";
        }
        else{
            pro.fitness = -1;
            
            cout << "Starting training with no progenitor.\n";
        }

        #ifdef UNIX
        cout << "Using " << PROCESS_NUM << " processes.\n";
        #else
        cout << "Not using multi-processing.\n";
        #endif

        // I have no idea why but the program keeps reprinting the first 2 messages until the end without this fflush
        fflush(stdout); // DEBUG
        
        if(init()){
            clear(SIGUSR1);
            return 0;
        }
        
        evolve(pro);
    
        clear(SIGUSR1);
    }

    // Play a match
    else if(strcmp(argv[1], "play") == 0){
        if(argc < 3){
            cout << "./star play <execution>\n";
            return 0;
        }

        int exec;
        sscanf(argv[2], "%d", &exec);

        if(exec == 0){
            cout << "ERROR: " << argv[2] << " is not a valid execution!\n";
            return 0;
        }

        play = true;

        // Play match
        pawnStars = (Guy*) malloc(sizeof(Guy));

        *pawnStars = summon(exec);
        play_match();

        free(pawnStars);
        pawnStars = NULL;
    }

    // Test an individual
    else if(strcmp(argv[1], "test") == 0){
        if(argc < 3){
            cout << "./star test <execution>\n";
            return 0;
        }

        int exec;
        sscanf(argv[2], "%d", &exec);

        if(exec == 0){
            cout << "ERROR: " << argv[2] << " is not a valid execution!\n";
            return 0;
        }

        play = true;

        // Play match
        pawnStars = (Guy*) malloc(sizeof(Guy));

        *pawnStars = summon(exec);
        test_guy();

        free(pawnStars);
        pawnStars = NULL;
    }

    else{
        for(int i = 0; i < argc; i++){
            cout << argv[i] << ' ';
        }
        cout << "is not a valid command.\n"
             << "Valid commands should be as following:"
             << "./star train\n"
             << "./star train <execution>\n"
             << "./star play <execution>\n";
    }

    return 0;
}