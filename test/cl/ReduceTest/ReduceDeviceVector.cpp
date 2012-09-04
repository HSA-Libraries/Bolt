#include <bolt/cl/device_vector.h>
#include <bolt/cl/reduce.h>

#include <numeric>

void testDeviceVector()
{
	const int aSize = 1000;
	std::vector<int> hA(aSize);
	bolt::cl::device_vector<int> dA(aSize);

	for(int i=0; i<aSize; i++) {
		hA[i] = i;
		dA[i] = i;
	};

	int hSum = std::accumulate(hA.begin(), hA.end(), 0);

	int sum = bolt::cl::reduce(dA.begin(), dA.end(), 0);
};