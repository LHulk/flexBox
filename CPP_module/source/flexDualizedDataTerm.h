#ifndef flexDualizedDataTerm_H
#define flexDualizedDataTerm_H

#include "flexTermDual.h"

template < typename T, typename Tvector >
class flexDualizedDataTerm : public flexTermDual<T, Tvector>
{
private:
public:
	Tvector f;
	#if __CUDACC__
		//save pointer to device memory
		T* fPtr;
	#endif

		flexDualizedDataTerm(prox _p, T _alpha, std::vector<flexLinearOperator<T, Tvector>* > _operatorList, std::vector<T> _f) : flexTermDual<T, Tvector>(_p, _alpha, _operatorList.size(), 1)	{
		//convert input f to device vector
		f.resize(_f.size());

		#if __CUDACC__
			thrust::copy(_f.begin(), _f.end(), this->f.begin());
			fPtr = thrust::raw_pointer_cast(this->f.data());
		#else
			std::copy(_f.begin(), _f.end(), this->f.begin());
		#endif

		int numberPrimals = _operatorList.size();

		this->operatorList = _operatorList;

		//create sigma and tau
		for (int i = 0; i < _operatorList.size() / numberPrimals; ++i)
		{
			for (int j = 0; j < numberPrimals; ++j)
			{
				int opNum = i*numberPrimals + j;

				this->operatorListT.push_back(this->operatorList[opNum]->copy());
				this->operatorListT[opNum]->transpose();
			}
		}
#if __CUDACC__
		this->createGPUOperators();
#endif
	};

	~flexDualizedDataTerm()
	{
		if (VERBOSE > 0) printf("Destructor of data term!");
	}

	void applyProx(flexBoxData<T, Tvector>* data, const std::vector<int> &dualNumbers, const std::vector<int> &primalNumbers)
	{
#if __CUDACC__
#else
		switch (this->p)
		{
			case dualL2DataProx:
			{
				T* ptrY = data->y[dualNumbers[0]].data();
				T* ptrYtilde = data->yTilde[dualNumbers[0]].data();
				T* ptrSigma = data->sigmaElt[dualNumbers[0]].data();

				T* ptrF = this->f.data();
				
				int numElements = data->yTilde[dualNumbers[0]].size();

				#pragma omp parallel for
				for (int i = 0; i < numElements; i++)
				{
					ptrY[i] = this->alpha / (ptrSigma[i] + this->alpha) * (ptrYtilde[i] - ptrSigma[i] * ptrF[i]);
				}

				//flexProxList<T, Tvector>::dualL2DataProx(data, sigma, dualNumbers, primalNumbers, this->alpha, this->f);
				break;
			}
			case dualL1DataProx:
			{
				T* ptrY = data->y[dualNumbers[0]].data();
				T* ptrYtilde = data->yTilde[dualNumbers[0]].data();
				T* ptrSigma = data->sigmaElt[dualNumbers[0]].data();

				T* ptrF = this->f.data();

				int numElements = data->yTilde[dualNumbers[0]].size();

				#pragma omp parallel for
				for (int i = 0; i < numElements; i++)
				{
					ptrY[i] = myMin<T>(this->alpha, myMax<T>(-this->alpha, ptrYtilde[i] - this->alpha * ptrSigma[i] * ptrF[i]));
				}

				//flexProxList<T, Tvector>::dualL1DataProx(data, sigma, dualNumbers, primalNumbers, this->alpha, this->f);
				break;
			}
			case dualKLDataProx:
			{
				T* ptrY = data->y[dualNumbers[0]].data();
				T* ptrYtilde = data->yTilde[dualNumbers[0]].data();
				T* ptrSigma = data->sigmaElt[dualNumbers[0]].data();

				T* ptrF = this->f.data();

				int numElements = data->yTilde[dualNumbers[0]].size();

				#pragma omp parallel for
				for (int i = 0; i < numElements; i++)
				{
					ptrY[i] = (T)0.5 * ((T)1 + ptrYtilde[i] - std::sqrt(myPow2<T>(ptrYtilde[i] - (T)1) + (T)4 * ptrSigma[i] * ptrF[i]));
				}

				//flexProxList<T, Tvector>::dualKLDataProx(data, sigma, dualNumbers, primalNumbers, this->alpha, this->f);
				break;
			}
		}
#endif
	};

#if __CUDACC__
	__device__ void applyProxElement(T** ptrYList, const T* yTilde, T** sigmaList, const int* dualNumbers, int numDuals, int index)
	{
		switch (this->p)
		{
			case dualL2DataProx:
			{
				for (int i = 0; i < numDuals; ++i)
				{
					const int dualNum = dualNumbers[i];

					T tmpSigma = sigmaList[dualNum][index];

					T factor = this->alpha / (tmpSigma + this->alpha);
					ptrYList[dualNum][index] = factor * (yTilde[dualNum] - tmpSigma * this->fPtr[index]);
				}
				break;
			}
			case dualL1DataProx:
			{
				for (int i = 0; i < numDuals; ++i)
				{
					const int dualNum = dualNumbers[i];

					ptrYList[dualNum][index] = myMinGPU(this->alpha, myMaxGPU(-this->alpha, yTilde[dualNum] - this->alpha * sigmaList[dualNum][index] * this->fPtr[index]));
				}
				break;
			}
			case dualKLDataProx:
			{
				
				break;
			}
		}
	}
#endif
};

#endif