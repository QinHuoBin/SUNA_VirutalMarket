
#include"State_of_Art_Random.h"


// from https://stackoverflow.com/questions/21237905/how-do-i-generate-thread-safe-uniform-random-numbers
#if defined (_MSC_VER)  // Visual studio
#define thread_local __declspec( thread )
#elif defined (__GCC__) // GCC
#define thread_local __thread
#endif

Random::Random(unsigned int seed)
{
	srand(seed);
}

Random::~Random()
{

}

using namespace std;

//int intRand(const int& min, const int& max) {
//	static thread_local mt19937* generator = nullptr;
//	if (!generator) generator = new mt19937(clock());
//	uniform_int_distribution<int> distribution(min, max);
//	return distribution(*generator);
//}

int Random::uniform(int min, int max)
{
	/*int range= max - min + 1;

	return (int)range*(genrand_close_open()) + min;*/

	//if (max - min == 0) {
	//	return min;
	//}
	//return (rand() % (max - min)) + min;

	// 再次修改以获得线程安全的随机数
	static thread_local mt19937* generator = nullptr;
	if (!generator) generator = new mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
	
	// 原本代码的“特性”,神经网络开始时会传入[0,-1]
	// 在random->uniform(0, number_of_connections - 1)
	if (max - min == -1)
		max++;
	uniform_int_distribution<int> distribution(min, max);
	return distribution(*generator);
}

double Random::uniform(double min, double max)
{
	//return ((double)genrand_close_open())*(max - min) +  min;

	//return min + (int)max * rand() / (RAND_MAX + 1);//dont trust csdn
	
	//return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));

	static thread_local mt19937* generator = nullptr;
	if (!generator) generator = new mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));

	// 开始时会传入[1,-1]
	// 在random->uniform(-variance, variance);
	if (max < min) {
		auto temp = max;
		max = min;
		min = temp;
	}

	uniform_real_distribution<double> distribution(min, max);
	return distribution(*generator);
}

double Random::uniform()
{
	//return genrand_close_open(); // [0,1)

	//return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

	static thread_local mt19937* generator = nullptr;
	if (!generator) generator = new mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
	uniform_real_distribution<double> distribution(0, 1);
	return distribution(*generator);
}
