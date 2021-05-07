
#include"State_of_Art_Random.h"


Random::Random(unsigned int seed)
{
	srand(seed);
}

Random::~Random()
{

}

int Random::uniform(int min, int max)
{
	/*int range= max - min + 1;

	return (int)range*(genrand_close_open()) + min;*/
	if (max - min == 0) {
		return min;
	}
	return (rand() % (max - min)) + min;
}

double Random::uniform(double min, double max)
{
	//return ((double)genrand_close_open())*(max - min) +  min;

	//return min + (int)max * rand() / (RAND_MAX + 1);//dont trust csdn
	return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

double Random::uniform()
{
	//return genrand_close_open(); // [0,1)
	return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}
