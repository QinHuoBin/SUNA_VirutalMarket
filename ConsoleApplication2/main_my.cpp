
#include "stdio.h"
#include "stdlib.h"
#include <time.h>

//agents
#include "agents/Unified_Neural_Model.h"

//environments
#include "environments/Virtual_Market.h"

#include "parameters.h"

FILE *main_log_file;

// // ThreadPool
// 			出错error: use of deleted function ‘std::future<void>::future(const std::future<void>&)’
// 原因：future不能被push_back复制，而要用emplace_back添加进vector里
// //	A simple C++11 Thread Pool implementation.
// // 		https://github.com/progschj/ThreadPool
#include "ThreadPool.h"
//#include "omp.h"
// https://github.com/lzpong/threadpool
// #include "threadpool.h"
//#include"threadpool11/threadpool11.hpp"

#include <chrono>
using std::chrono::duration_cast;
using std::chrono::milliseconds;

#include <iostream>
#define P(EX) std::cout << #EX << ": " << EX << std::endl;

double computeAverage(double *last_rewards, int counter)
{
	int k;

	if (counter > 100)
	{
		counter = 100;
	}

	double avg_rewards = 0.0;
	//printf("AAA\n");
	for (k = 0; k < counter; ++k)
	{
		//printf("%f\n",last_rewards[k]);
		avg_rewards += last_rewards[k];
	}
	avg_rewards = avg_rewards / (double)counter;

	return avg_rewards;
}

void setFeatures(Reinforcement_Environment *env)
{

#ifdef SET_NORMALIZED_INPUT
	bool feature_available;

	feature_available = env->set(NORMALIZED_OBSERVATION);

	if (feature_available == false)
	{
		printf("NORMALIZED_OBSERVATION feature not available\n");
		exit(1);
	}
	else
	{
		fprintf(main_log_file, "Normalized Observation enabled\n");
	}
#endif

#ifdef SET_NORMALIZED_OUTPUT
	bool feature_available;

	feature_available = env->set(NORMALIZED_ACTION);

	if (feature_available == false)
	{
		printf("NORMALIZED_ACTION feature not available\n");
		exit(1);
	}
	else
	{
		fprintf(main_log_file, "Normalized Action enabled\n");
	}
#endif
}

double aaaa(Unified_Neural_Model *agent, Virtual_Market *env, int the_individual)
{
	//do one trial (multiple steps until the environment finish the trial or the trial reaches its MAX_STEPS)
	// 个体在环境进行多步
	double accum_reward = 0;
	double reward;
	for (int step_counter = 0; step_counter < env->MAX_STEPS; step_counter++)
	{
		agent->step_my(env->get_observation(the_individual), 0, the_individual);

		reward = env->step(agent->action, the_individual);

		accum_reward += reward;
	}
	env->rewind(the_individual);						// 环境倒带
	agent->endEpisode_my(accum_reward, the_individual); // 更新个体的reward等
	return accum_reward;
}

int main()
{
	//int trials_to_change_maze_states= 10000;
	//main_log_file = fopen("log.txt", "w");

	Random *random = new Random(time(NULL));

	//Reinforcement_Environment* env= new Mountain_Car(random);
	//Reinforcement_Environment* env= new Function_Approximation(random,1000,false);
	//Reinforcement_Environment* env= new Single_Cart_Pole(random);
	//Reinforcement_Environment* env= new Double_Cart_Pole(random);
	//Reinforcement_Environment* env= new Multiplexer(3,8,random);
	Virtual_Market *env = new Virtual_Market(random);

	//Reinforcement_Agent* agent= new Dummy(env);
	Unified_Neural_Model *agent = new Unified_Neural_Model(random);

	setFeatures(env);

	//Self_Organizing_Neurons* b= (Self_Organizing_Neurons*)agent;

	//print max accumulated reward seen in N trials, the N trials is given by trial_frequency_to_print
	bool print_max_accum_reward_in_n_trials = true;
	int trial_frequency_to_print = 100;
	double max_accum_reward = 0;
	bool was_initialized = false; //tells if the max_accum_reward was initialized

	bool print_reward = false;
	bool print_step = false;
	bool print_average = false;
	//bool print_accumulated_reward=true;
	bool print_agent_information = false;

	//int trials=100000
	//int trials=200000;
	//int trials=200;
	//int trials=500;
	int trials = 1000;
	//int trials=100000;

	int number_of_observation_vars;
	int number_of_action_vars;

	env->start(number_of_observation_vars, number_of_action_vars);
	agent->init(number_of_observation_vars, number_of_action_vars);

	//starting reward
	//double reward = env->step(NULL);// 虚拟市场不需要初始的step

	// create thread pool with 4 worker threads
	ThreadPool pool(8);

	//agent->print();
	auto time_mark = std::chrono::system_clock::now();

	double rewards[SUBPOPULATION_SIZE];

	// 进行trials次进化
	// 每次进化周期中，会在内循环对每一个个体进行测试
	// 执行agent->endEpisode后，如果当agent->generation变化之后，种群发生了进化，并发生了进化
	//	此时调用env中的函数，让time进行下n刻
	// 否则当agent->generation不变化，则还是在训练同一批个体，让env进行倒带，给下一个个体使用
	for (int this_generation = 0; this_generation < trials;)
	{
		double accum_reward = 0; // 一代中各个个体reward之和
		// 对每个个体进行训练
		vector<std::future<double>> results;
		for (int the_individual = 0; the_individual < SUBPOPULATION_SIZE; the_individual++)
		{
			//rewards[the_individual]=aaaa(agent, env, the_individual);
			std::future<double> result = pool.enqueue(aaaa, agent, env, the_individual);
			results.emplace_back(std::move(result));
		}

		for (int the_individual = 0; the_individual < SUBPOPULATION_SIZE; the_individual++)
		{
			double a= results[the_individual].get();
			rewards[the_individual] = a;
		}

		// 统计特征值并输出
		if (this_generation % 100 == 0)
		{
			double best = rewards[0], average = 0, worst = rewards[0];

			for (int i = 0; i < SUBPOPULATION_SIZE; i++)
			{
				if (rewards[i] > best)
					best = rewards[i];
				if (rewards[i] < worst)
					worst = rewards[i];
				average += rewards[i];
			}
			average /= SUBPOPULATION_SIZE;
			printf("%d best=%lf average=%lf worst=%lf\n", this_generation, best, average, worst);
		}

		// 完成一代后，进行进化
		auto a = std::chrono::system_clock::now();
		agent->spectrumDiversityEvolve();
		auto b = std::chrono::system_clock::now();
		auto c=duration_cast<milliseconds>(b - a);
		if (this_generation % 10 == 0)
		{
			printf("spectrumDiversityEvolve %d\n",long(c.count()*10));
		}

		// 当agent->generation变化之后，种群发生了进化，此时推进market的time
		// 否则，对market进行回档
		if (agent->generation != this_generation)
		{
			this_generation++;
			env->next_episode();
			if (this_generation % 10 == 0)
			{
				auto delta = duration_cast<milliseconds>(std::chrono::system_clock::now() - time_mark);
				printf("now%d %d\n", this_generation, int(delta.count()));
				time_mark = std::chrono::system_clock::now();
			}
		}
		else
		{
			printf("个体已全部响应，但代数未改变！");
			exit(-1);
		}
	}

	//agent->saveAgent("dna_best_individual");
	agent->save_all_agents();

	//printf("reward average %f\n",reward_sum/(double)trials);
	//printf("step average %f\n",step_sum/(double)trials);

	//fclose(main_log_file);

	return 0;
}

// int main()
// {
// 	ThreadPool pool(2);
// 	pool.enqueue(main111);
// 	pool.enqueue(main111);
// }