/*
*	Desc: Simple wrapper around the std::vector class
*	Note: This is just a base class for other datasets
*/
#pragma once
#include <vector>
#include <algorithm>
#include <numeric>

#define PRODUCT_OPERATOR(SELF, ARG) SELF operator*(ARG)
#define ADD_OPERATOR(SELF, ARG) SELF operator+(ARG)

namespace types
{
	template<typename _Ty> 
	struct Dataset : public std::vector<_Ty>
	{
		using std::vector<_Ty>::vector;
	};
}