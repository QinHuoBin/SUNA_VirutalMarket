#include "Virtual_Market.h"

#include <iostream>
#define P(EX) std::cout << #EX << ": " << EX << std::endl;

using std::cout;
using std::endl;
using std::string;
using std::wstring;
Virtual_Market::Virtual_Market(Random *random)
{
    this->random = random;
    trial = -1; //the first trial will be zero, because restart() is called making it increment

#ifdef _WIN32
    wstring path = L"c:\\Users\\QinHuoBin\\Downloads\\2020.5.18~2020.6.10.期货全市场行情数据\\Data\\";
#else
    wstring path = L"/mnt/c/Users/QinHuoBin/Downloads/2020.5.18~2020.6.10.期货全市场行情数据/Data/";
#endif
    string code = "c2009";

    // size_t a=rfd::collect_all_ticks(MarketPrice_vector, path, code);
    // MarketPrice_vector_sample.clear();
  //rfd::save_binary_tick_file(path, code);

   rfd::read_binary_tick_file(MarketPrice_vector, path, code);

    price_size = rfd::resample(MarketPrice_vector, MarketPrice_vector_sample, 60);

    MarketPrice_vector.clear(); // 清空，防止误使用

    MAX_STEPS = 1000; // 每次运行1/3
    //MAX_STEPS= price_size/3;// 每次运行1/3

    P(price_size);
}

Virtual_Market::~Virtual_Market()
{
}

void Virtual_Market::start(int &number_of_observation_vars, int &number_of_action_vars)
{
    // N_TICK_INPUT个数据观察点+1余额观察点（作为reward）
    number_of_observation_vars = N_TICK_INPUT + 1;
    this->number_of_observation_vars = N_TICK_INPUT + 1;

    for (int i = 0; i < SUBPOPULATION_SIZE; i++)
        observations[i] = (double *)calloc(number_of_observation_vars , sizeof(double));

    number_of_action_vars = 1;
    this->number_of_action_vars = 1;

    restart();
}

double Virtual_Market::step(double *action, size_t the_individual)
{
    //P(time);
    // initial reward
    if (action == NULL)
    {
        return 0;
    }

    // 根据仓位方向和仓位价值获取利润
    double profit = trade(position_hold[the_individual], money[the_individual], the_individual);
    money[the_individual] += profit;

    // 根据agent的动作改变仓位
    if (action[0] > 1)
    {
        position_hold[the_individual] = 1; //多仓
    }
    else if (action[0] < -1)
    {
        position_hold[the_individual] = -1; //空仓
    }
    else
    {
        position_hold[the_individual] = 0; //平仓
    }

    // 改变agent观察点的值
    flush_observations(the_individual);

    time[the_individual]++;

    return profit;
}

double Virtual_Market::trade(int _position_hold, double _money, size_t the_individual)
{
    auto a = get_price(time[the_individual]);
    auto b = get_price(time[the_individual] - 1);
    double pct = (a - b) / b;
    pct *= _position_hold;
    return _money * (pct);
}

// 进行倒带，重置money和position_hold
void Virtual_Market::rewind(size_t the_individual)
{
    time[the_individual] = base_time;

    money[the_individual] = 10000;
    position_hold[the_individual] = 0;

    flush_observations(the_individual);
}

// 基准时间递增
void Virtual_Market::next_episode()
{
    base_time += MAX_STEPS;
    if (base_time + MAX_STEPS > price_size)
    {
        choose_random_base_time();
    }

    for (int i = 0; i < SUBPOPULATION_SIZE; i++)
        rewind(i);
}

// 选取随机的基准时间（比如上一次选取之后数据已经用完）
void Virtual_Market::choose_random_base_time()
{
    base_time = (size_t)random->uniform(N_TICK_INPUT + 100, price_size - MAX_STEPS - 1);
    for (int i = 0; i < SUBPOPULATION_SIZE; i++)
        rewind(i);
}

// 刷新神经网络的观察点
void Virtual_Market::flush_observations(size_t the_individual)
{

    for (int i = 0; i < N_TICK_INPUT; i++)
    {
        observations[the_individual][i] = get_price(time[the_individual] + (-N_TICK_INPUT + 1 + i));
    }
    observations[the_individual][N_TICK_INPUT] = money[the_individual];
}

double Virtual_Market::restart()
{

    base_time = N_TICK_INPUT + 100;
    for (int i = 0; i < SUBPOPULATION_SIZE; i++)
        rewind(i);

    return -1;
}

double Virtual_Market::get_price(size_t _time)
{
    if (_time >= price_size)
    {
        printf("时间超出范围：%ld，最大值：%ld\n", _time, price_size - 1);
        return -9999;
        //exit(-1);
    }
    return MarketPrice_vector_sample[_time].LastPrice / 10000;
}

double *Virtual_Market::get_observation(size_t the_individual)
{
    return observations[the_individual];
}

void Virtual_Market::print()
{
}

void Virtual_Market::print_observation_vals(size_t the_individual)
{
    printf("-----%ld-----\n", time);
    for (int i = 0; i <= N_TICK_INPUT; i++)
    {
        printf("%lf\n", observations[the_individual][i]);
    }
    printf("-----%ld-----\n", time);
}

double Virtual_Market::step(double *action)
{
    cout<<"不允许调用原版step!"<<endl;
    exit(-1);
}