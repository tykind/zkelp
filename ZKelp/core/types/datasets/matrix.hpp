/*
*	Desc: Matrix dataset
*/
#pragma once
#include "vectors.hpp"

namespace types
{
	template<typename _Ty, auto _Size = 0u> struct Matrix final : public Dataset<Vec<_Ty, _Size>>
	{
		using Dataset<Vec<_Ty, _Size>>::Dataset;

		PRODUCT_OPERATOR(Matrix<_Ty, _Size>, Matrix<_Ty, _Size>& m2)
		{
			Matrix<_Ty, _Size> ret;
			ret.reserve(m2.size());

			for (auto i = 0u; i < this->size(); i++)
			{
				ret.push_back({});
				for (auto j = 0u; j < m2.size(); j++)
					ret[i].push_back(std::inner_product(this->operator[](i).begin(), this->operator[](i).end(), m2[j].begin(), 0.0));
			}
			return ret;
		}
	};

	template<auto _Size = 0u>
	using Md = Matrix<double, _Size>;
}