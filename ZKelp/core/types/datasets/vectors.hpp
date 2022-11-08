/*
*	Desc: This is just a vector dataset
*/
#pragma once
#include "datasets.hpp"

namespace types
{

	template<typename _Ty> struct Vec final : public Dataset<_Ty>
	{
		using Dataset<_Ty>::Dataset;

		ADD_OPERATOR(Vec<_Ty>, Vec<_Ty>& v2) {
			Vec<_Ty> ret(v2.begin(), v2.end());

			for (auto i = 0u; i < v2.size(); i++)
				ret[i] += this->operator[](i);
			return ret;
		}

		PRODUCT_OPERATOR(Vec<_Ty>, Vec<_Ty>& v2)
		{
			Vec<_Ty> ret;
			ret.reserve(v2.size());

			for (auto i = 0u; i < this->size(); i++)
				ret.push_back(std::inner_product(this->begin(), this->end(), v2.begin(), 0.0));
			return ret;
		}
	};

	using Vd = Vec<double>;
}