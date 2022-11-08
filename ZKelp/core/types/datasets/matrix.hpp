/*
*	Desc: Matrix dataset
*/
#pragma once
#include "vectors.hpp"

namespace types
{
	template<typename _Ty> struct Matrix final : public Dataset<Vec<_Ty>>
	{
		using Dataset<Vec<_Ty>>::Dataset;

		PRODUCT_OPERATOR(Matrix<_Ty>, Matrix<_Ty>& m2)
		{
			Matrix<_Ty> ret;
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

	using Md = Matrix<double>;
}