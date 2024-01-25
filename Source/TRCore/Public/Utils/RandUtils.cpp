#include "RandUtils.h"

#include <random>

unsigned RandUtils::GenerateSeed()
{
	std::random_device rd;

	return rd();
}
