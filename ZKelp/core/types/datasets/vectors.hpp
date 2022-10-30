/*
*	Desc: This is just a vector dataset
*/
#pragma once
#include "datasets.hpp"

namespace types
{

	template<typename _Ty, auto _Size = 0u> struct Vec final : public Dataset<_Ty, _Size>
	{
		using Dataset<_Ty, _Size>::Dataset;

		ADD_OPERATOR(Vec<_Ty, _Size>, Vec<_Ty, _Size>& v2) {
			vec<_Ty, _Size> ret(v2.begin(), v2.end());

			for (auto i = 0u; i < v2.size(); i++)
				ret[i] += this->operator[](i);
			return ret;
		}

		PRODUCT_OPERATOR(Vec<_Ty, _Size>, Vec<_Ty, _Size>& v2)
		{
			Vec<_Ty, _Size> ret;
			ret.reserve(v2.size());

			for (auto i = 0u; i < this->size(); i++)
				ret.push_back(std::inner_product(this->begin(), this->end(), v2.begin(), 0.0));
			return ret;
		}
	};

	template<auto _Size = 0u>
	using Vd = Vec<double, _Size>;
}