#pragma once

#include <boltCL/bolt.h>


namespace boltcl {

	template<typename InputIterator, typename OutputIterator, typename BinaryFunction> 
	void transform(InputIterator first1, InputIterator last1, InputIterator first2, OutputIterator result, 
		BinaryFunction f, std::string userCode, std::string functorTypeName="")  {

			typedef std::iterator_traits<InputIterator>::value_type T;
			int sz = (int)(last1 - first1); // FIXME - use size_t

			//FIXME - use host pointers for these, since they are only read/written once.
			cl::Buffer A(CL_MEM_READ_ONLY, sizeof(T) * sz);
			cl::enqueueWriteBuffer(A, false, 0, sizeof(T) * sz, &*first1);
			cl::Buffer B(CL_MEM_READ_ONLY, sizeof(T) * sz);
			cl::enqueueWriteBuffer(B, false, 0, sizeof(T) * sz, &*first2);

			cl::Buffer Z(CL_MEM_WRITE_ONLY|CL_MEM_USE_HOST_PTR, sizeof(T) * sz, &*result);

			cl::Buffer userFunctor(CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, sizeof(f), &f );   // Create buffer wrapper so we can access host parameters.

			cl::Kernel k;
			const bool needCompile = true;
			if (needCompile) {
				std::string transformFunctionString = boltcl::fileToString("../../boltCL/transform_shaders.cl");  //FIXME, when this becomes more stable move the shader code to a string in bolt.cpp


				// Instantiate the template using the value and functorType strings:
				// For user-defined types, the user must create a TypeName trait which returns the name of the class
				// or use the function call method which excepts the name of the class.
				std::string valueTypeName = TypeName<T>::get();

				if (functorTypeName.empty()) {
					functorTypeName = TypeName<BinaryFunction>::get();
				}

				std::string instantiationString = 
					"// Host generates this instantiation string with user-specified value type and functor\n"
					"template __attribute__((mangled_name(transformInstantiated)))\n"
					"kernel void transformTemplate(\n"
					"global " + valueTypeName + "* A,\n"
					"global " + valueTypeName + "* B,\n"
					"global " + valueTypeName + "* Z,\n"
					"const int length,\n"
					"global " + functorTypeName + "* userFunctor);\n\n";

				std::string codeStr = userCode + "\n\n" + transformFunctionString +   instantiationString;

				if (1) {
					std::cout << "ValueType  ='" << valueTypeName << "'" << std::endl;
					std::cout << "FunctorType='" << functorTypeName << "'" << std::endl;
					std::cout << "sizeof(Functor)=" << sizeof(f) << std::endl;
					std::cout << "code=" << std::endl << codeStr;
				}

				k = boltcl::compileFunctor(codeStr, "transformInstantiated");
			} // { else read k from cache and make a local copy so we can set the args.

			k.setArg(0, A);
			k.setArg(1, B);
			k.setArg(2, Z);
			k.setArg(3, sz);
			k.setArg(4, userFunctor);

			const int wgSize = 256; // FIXME.  Need to ensure global size is a multiple of this ,etc.


			cl::CommandQueue::getDefault().enqueueNDRangeKernel(
				k, 
				cl::NullRange, 
				cl::NDRange(sz), 
				cl::NDRange(wgSize));

			cl::CommandQueue::getDefault().enqueueMapBuffer(Z, true, CL_MAP_READ | CL_MAP_WRITE, 0/*offset*/, sz);

			//FIXME - need to call map on host pointers.
	};
};
