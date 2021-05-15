

#include "stdio.h"
#include "stdlib.h"
#include <time.h>
#include <algorithm>

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
ThreadPool pool(16);

//#include "omp.h"
// https://github.com/lzpong/threadpool
// #include "threadpool.h"
//#include"threadpool11/threadpool11.hpp"

#include <chrono>
using std::chrono::duration_cast;
using std::chrono::milliseconds;

#include <iostream>
#define P(EX) std::cout << #EX << ": " << EX << std::endl;

struct TradeHistory {
	long long time;
	double action;
	int position_now;
	int position_change_times = 0;
	double reward;
	double accum_reward;
};

struct ModuleInfo {
	Module* the_module;
	int generation;
	float rank;// 排在种群的百分之几（第一1就是1.00，最末是0.0）
	int index_in_population;
	vector<TradeHistory> results;
};

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

// 把测试结果保存下来
void save_results_to_csv(vector<TradeHistory>& results, string &save_path)
{
	FILE* csv_file;
	csv_file=fopen(save_path.c_str(), "w");

	string header = "time,action,position_now,position_change_times,reward,accum_reward\n";
	fwrite(header.c_str(), 1, header.size(), csv_file);

	//printf("results.size()=%d\n", results.size());
	for (TradeHistory & history : results) {
		fprintf(csv_file, "%ld,%lf,%d,%d,%lf,%lf\n", history.time,history.action, history.position_now, history.position_change_times, history.reward, history.accum_reward);
	}

	fflush(csv_file);
	fclose(csv_file);
}

/*
测试单个module并保存结果
*/
double test_single_module(ModuleInfo& module_info,int index_in_env, Virtual_Market* env, string& save_path) {
	//vector<pair<size_t, double>> results; //result：pair<time, accum_reward>
	double accum_reward = 0;
	double reward;

	double action;

	Module* module_be_test = module_info.the_module;

	int position_last = 0;
	int position_change_times = 0;
	while (env->time[index_in_env]<env->price_size)
	{
		module_be_test->process(env->observations[index_in_env], &action);
		reward = env->step(&action, index_in_env);
		accum_reward += reward;
		//results.push_back(pair<size_t, double>(env->time[index_in_env], accum_reward));

		TradeHistory history;
		history.time = env->time[index_in_env];
		history.action = action;
		history.position_now = env->position_hold[index_in_env];
		if (position_last != history.position_now) {
			position_change_times++;
			position_last = history.position_now;
		}
		history.position_change_times = position_change_times;
		history.reward = reward;
		history.accum_reward = accum_reward;

		module_info.results.push_back(history);
	}

	// 保存到文件中
	save_results_to_csv(module_info.results, save_path);

	return accum_reward;
}

// 测试切片
void test_slices(vector<ModuleInfo> slices,Virtual_Market* env,string &base_path) {
	// 一百一百切片进行测试
	// todo:优化env的逻辑

#ifdef _WIN32
	string test_dir_name = "\\test\\";
	system(("mkdir " + base_path + test_dir_name).c_str());
#else
	string test_dir_name = "/test/";
	system(("mkdir -p " + base_path + test_dir_name).c_str());// -p 递归创建
#endif

	// 每批次测试在slices中的索引，即测试的索引为[start_index,end_index)
	int start_index, end_index;
	for (start_index = 0; start_index < slices.size(); start_index += SUBPOPULATION_SIZE) {
		env->restart();// 重置整个环境
		end_index = start_index + SUBPOPULATION_SIZE;

		// test_index：在slices中的索引
		vector<future<double>> locker;
		for (int test_index = start_index; test_index < end_index&&test_index<slices.size(); test_index++) {
			locker.clear();
				ModuleInfo & module_info = slices[test_index];
				string save_path = base_path + test_dir_name + to_string( module_info.generation)+"-"+to_string(module_info.index_in_population) + "-" +to_string( module_info.rank)+".csv";
			//Module* module_be_test = module_info.the_module;

			// 多线程优化
			
			//test_single_module(module_info, test_index - start_index, env, save_path);
				locker.emplace_back(std::move( pool.enqueue(test_single_module, module_info, test_index - start_index, env, save_path)));
		}
		// 等待这一批次
		for (auto& lock : locker)
			lock.get();
	}
}

bool cmpfunc(const  pair<double,int>& a, const pair<double, int>& b)
{
	return (a.first - b.first)>0;
}

void save_module_to_slices(Unified_Neural_Model* agent, int rank_of_all, std::vector<std::pair<double, int>>& situation, std::vector<ModuleInfo>& slices) {
	ModuleInfo choose;
	choose.generation = agent->generation;
	choose.rank = double(rank_of_all) / SUBPOPULATION_SIZE;
	choose.the_module = agent->subpopulation[0][situation[rank_of_all].second]->clone();
	choose.index_in_population = situation[rank_of_all].second;
	slices.push_back(choose);
}

int main()
{
	time_t time_now = time(0);
#ifdef _WIN32
	string base_path = string("C:\\Users\\QinHuoBin\\source\\repos\\ConsoleApplication2\\ConsoleApplication2\\dna_visual\\") + to_string(time_now) + '\\';
#else
	string base_path = string("/mnt/c/Users/QinHuoBin/source/repos/ConsoleApplication2/ConsoleApplication2/dna_visual/") + to_string(time_now) + '/';
#endif
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
	int trials = 200000;
	//int trials=100000;

	int number_of_observation_vars;
	int number_of_action_vars;

	env->start(number_of_observation_vars, number_of_action_vars);
	agent->init(number_of_observation_vars, number_of_action_vars);

	//starting reward
	//double reward = env->step(NULL);// 虚拟市场不需要初始的step

	// create thread pool with 4 worker threads
	

	//agent->print();
	auto time_mark = std::chrono::system_clock::now();

	double rewards[SUBPOPULATION_SIZE];


	// 在不同代数，对不同fitness排名的子代进行切片，在测试环境中验证其有效性
	vector<ModuleInfo> slices;


	// 进行trials次进化
	// 每次进化周期中，会在内循环对每一个个体进行测试
	// 执行agent->endEpisode后，如果当agent->generation变化之后，种群发生了进化，并发生了进化
	//	此时调用env中的函数，让time进行下n刻
	// 否则当agent->generation不变化，则还是在训练同一批个体，让env进行倒带，给下一个个体使用
	for (int this_generation = 0; this_generation <= trials;)
	{
		double accum_reward = 0; // 一代中各个个体reward之和
		// 对每个个体进行训练
		vector<std::future<double>> results;
		for (int the_individual = 0; the_individual < SUBPOPULATION_SIZE; the_individual++)
		{
			//rewards[the_individual]=aaaa(agent, env, the_individual);
			std::future<double> result = pool.enqueue(aaaa, agent, env, the_individual);
			//printf("放入%d\n", the_individual);
			results.emplace_back(std::move(result));
		}

		for (int the_individual = 0; the_individual < SUBPOPULATION_SIZE; the_individual++)
		{
			double a= results[the_individual].get();
			//printf("---------取出%d\n", the_individual);
			rewards[the_individual] = a;
		}

		// 统计特征值并输出
		// 并且分层地放进切片中
		if (this_generation % 100 == 0)
		{
			// pair<fitness,index>
			vector<pair<double, int>> situation;
			double average_of_all = 0;
			for (int i = 0; i < SUBPOPULATION_SIZE; i++)
			{
				situation.push_back(pair<double, int>(rewards[i], i));
				average_of_all += rewards[i];
			}
			sort(situation.begin(), situation.end(), cmpfunc);
			
			average_of_all /= SUBPOPULATION_SIZE;
			printf("%d best=%lf average=%lf worst=%lf\n", this_generation, situation.front().first, average_of_all, situation.back().first);
			
			// 选出最好的10%
			int ten_percent_quantity = SUBPOPULATION_SIZE / 10;
			for (int rank_of_all = 0; rank_of_all < ten_percent_quantity; rank_of_all++)
			{
				save_module_to_slices(agent, rank_of_all, situation, slices);
			}

			// 选出平庸的10%
			for (int rank_of_all = SUBPOPULATION_SIZE*0.45; rank_of_all < SUBPOPULATION_SIZE * 0.45 + ten_percent_quantity; rank_of_all++)
			{
				save_module_to_slices(agent, rank_of_all, situation, slices);
			}

			// 选出最差的10%
			for (int rank_of_all = SUBPOPULATION_SIZE-ten_percent_quantity; rank_of_all < SUBPOPULATION_SIZE ; rank_of_all++)
			{
				save_module_to_slices(agent, rank_of_all, situation, slices);
			}
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
	
	test_slices(slices, env, base_path);


	agent->save_all_agents(base_path);

	

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