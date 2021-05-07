#ifndef VIRTUAL_MARKET_H
#define VIRTUAL_MARKET_H

#include "../parameters.h"
#include "Reinforcement_Environment.h"
#include "../agents/Reinforcement_Agent.h"
#include "../random/State_of_Art_Random.h"
#include "../read_future_data.h"
#include "stdlib.h"
#include "stdio.h"

#define N_TICK_INPUT 16

class Virtual_Market : public Reinforcement_Environment
{
public:
    Virtual_Market(Random *random);
    ~Virtual_Market();

    //All Reinforcement Problems have the observartion variable, although it is not declared here!
    //double* observation;
    //int number_of_observation_vars;
    //int number_of_action_vars;
    //int trial;
    double *(observations[SUBPOPULATION_SIZE]);

    //Reinforcement Problem API
    void start(int &number_of_observation_vars, int &number_of_action_vars);
    double step(double *action, size_t the_the_individual);
    double step(double *action);//弃用
    double restart();
    void print();

    double *get_observation(size_t the_individual);

    void rewind(size_t the_individual);
    void next_episode();
    void choose_random_base_time();
    void flush_observations(size_t the_individual);

    Random *random;
    size_t time[SUBPOPULATION_SIZE];
    size_t base_time;
    double money[SUBPOPULATION_SIZE];
    int position_hold[SUBPOPULATION_SIZE];

    size_t price_size;
    std::vector<rfd::MarketPrice> MarketPrice_vector;
    std::vector<rfd::MarketPrice> MarketPrice_vector_sample;

    double get_price(size_t time);
    double trade(int _position_hold, double money, size_t the_individual);

    void print_observation_vals(size_t the_individual);
};

#endif