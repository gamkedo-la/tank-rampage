#pragma once

#include <random>
#include <algorithm>
#include <concepts>
#include <iterator>
#include <numeric>

namespace RandUtils
{
	TRCORE_API unsigned GenerateSeed();

	template<typename Random, std::forward_iterator Iter>
	void ShuffleIndices(Iter Begin, Random& Rng, std::size_t Count);
}

#pragma region Template Definitions

namespace RandUtils
{
	template<typename Random, std::forward_iterator Iter>
	void ShuffleIndices(Iter BeginIt, Random& Rng, std::size_t Count)
	{
		const auto EndIt = std::next(BeginIt, Count);

		// 0, 1, 2, 3...
		std::iota(BeginIt, EndIt, 0);
		std::shuffle(BeginIt, EndIt, Rng);
	}
}

#pragma endregion Template Definitions