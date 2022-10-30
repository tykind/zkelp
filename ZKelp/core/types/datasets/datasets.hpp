/*
*	Desc: Simple wrapper around the std::vector class
*	Note: This is just a base class for other datasets
*/
#pragma once
#include <vector>

#define PRODUCT_OPERATOR(SELF, ARG) SELF operator*(ARG)
#define ADD_OPERATOR(SELF, ARG) SELF operator+(ARG)

namespace types
{
	template<typename _Ty, auto _Size = 0u> struct Dataset : public std::vector<_Ty, _Size>
	{
		using std::vector<_Ty, _Size>::vector;
	};
}