#ifndef flexEmptyDataTerm_H
#define flexEmptyDataTerm_H

#include "flexTermPrimal.h"

template < typename T >
class flexEmptyDataTerm : public flexTermPrimal<T>
{
private:

public:
	flexEmptyDataTerm(flexVector<int> _dims) : flexTermPrimal(1, _dims,1.0)
	{
	};

	void applyProx(flexBoxData<T> &data, flexVector<T> tau, flexVector<int> primalNumbers)
	{
		for (int i = 0; i < primalNumbers.size(); ++i)
		{
			data.x[primalNumbers[i]] = data.xTilde[primalNumbers[i]];
		}
	};
};

#endif