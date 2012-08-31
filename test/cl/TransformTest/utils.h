

template<typename InputIterator1, typename InputIterator2>
int checkResults(std::string msg, InputIterator1 first1 , InputIterator1 end1 , InputIterator2 first2)
{
	int errCnt = 0;
	static const int maxErrCnt = 20;
	size_t sz = end1-first1 ;
	for (int i=0; i<sz ; i++) {
		if (first1 [i] != first2 [i]) {
			errCnt++;
			if (errCnt < maxErrCnt) {
				std::cout << "MISMATCH " << msg << " STL= " << first1[i] << "  BOLT=" << first2[i] << std::endl;
			} else if (errCnt == maxErrCnt) {
				std::cout << "Max error count reached; no more mismatches will be printed...\n";
			}
		};
	};

	if (errCnt==0) {
		std::cout << " PASSED  " << msg << " Correct on all " << sz << " elements." << std::endl;
	} else {
		std::cout << "*FAILED  " << msg << "mismatch on " << errCnt << " / " << sz << " elements." << std::endl;
	};

	return errCnt;
};
