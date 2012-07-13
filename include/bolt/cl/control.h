#pragma once

#include <CL/cl.hpp>

namespace bolt {
	namespace cl {

		/*! \addtogroup miscellaneous
		*/

		/*! \addtogroup control
		* \ingroup miscellaneous
        * \{
		*/

		/*! The \p control class allows user to control the parameters of a specific Bolt algorithm call 
		 such as the command-queue where GPU kernels run, debug information, load-balancing with the host, and more.  Each Bolt Algorithm call accepts the 
		\p control class as an optional first argument.  Additionally, Bolt contains a global default \p control structure which
		is used in cases where the \p control argument is not specified, and developers can also modify this structure.  Some examples:

	 	* \code
		* cl::CommandQueue myCommandQueue = ...
		* bolt::cl::control myControl;
		* myControl.commandQueue(myCommandQueue);
		* int sum = bolt::cl::reduce(myControl, ...); 
		* \endcode


		* Developers can also inialize and save control structures to avoid the cost of copying the default on each function call.  An example:
		* \code
		* class MyClass {
		*   MyClass(...) {
		*      cl::CommandQueue myCommandQueue = ...
		*      _boltControl.commandQueue(myCommandQueue);
		*   };
		*
		*   void runIt() 
		*   {
		*     int sum = bolt::cl::reduce(_boltControl, ...); 
		*   };
		*
		*   private:
		*		bolt::cl::control _boltControl;
		*   }
		* \endcode


		* It can sometimes be useful to set the global default \p control structure that is used by Bolt algorithms calls which do not explicitly specify
		* a control parameter as the first argument.  For example, the application initialization routine may examine 
		* all the available GPU devies and select the one which should be used for all subsequent bolt calls.  This can easily be 
		* achieved by writing the global default \p control structure, ie:
		* \code
		* cl::CommandQueue myCommandQueue = ...
		* bolt::cl::control::getDefault().commandQueue(myCommandQueue); 
		* bolt::cl::control::getDefault().debug(bolt::cl::control::debug:SaveCompilerTemps);  
		* ...
		* \endcode
		* 
		 * \{
		 */
		class control {
		public:
			enum e_UseHostMode {NoUseHost, UseHost};  
            enum e_RunMode     {Automatic,
                                SerialCpu,   
                                MultiCoreCpu, 
                                Gpu };

			enum e_AutoTuneMode{NoAutoTune=0x0, 
                                AutoTuneDevice=0x1, 
                                AutoTuneWorkShape=0x2, 
                                AutoTuneAll=0x3}; // FIXME, experimental
			struct debug {
				static const unsigned None=0;
				static const unsigned Compile = 0x1;
				static const unsigned ShowCode = 0x2;
				static const unsigned SaveCompilerTemps = 0x4;
				static const unsigned DebugKernelRun = 0x8;
				static const unsigned AutoTune = 0x10;
			};
		public:

			// Construct a new control structure, copying from default control for arguments which are not overridden.
			control(
				::cl::CommandQueue commandQueue=getDefault().commandQueue(),
				e_UseHostMode useHost=getDefault().useHost(),
				unsigned debug=getDefault().debug()
				) :
			_commandQueue(commandQueue),
				_useHost(useHost),
                _forceRunMode(getDefault()._forceRunMode),
				_debug(debug),
				_autoTune(getDefault()._autoTune),
				_wgPerComputeUnit(getDefault()._wgPerComputeUnit),
				_compileOptions(getDefault()._compileOptions),
				_compileForAllDevices(getDefault()._compileForAllDevices)
			{};

			//setters:
			//! Set the OpenCL command queue (and associated device) which Bolt algorithms will use.  
			//! Only one command-queue may be specified for each call - Bolt does not load-balance across
			//! multiple command queues.  Bolt also uses the specified command queue to determine the OpenCL context and
			//! device.
			void commandQueue(::cl::CommandQueue commandQueue) { _commandQueue = commandQueue; };

			//! If enabled, Bolt may use the host CPU to run parts of the Algorithm.  If false, Bolt will run the
			//! entire algorithm using the device specified by the command-queue - this might be appropriate 
			//! on a discrete GPU where the input data is located on the device memory.
			void useHost(e_UseHostMode useHost) { _useHost = useHost; };


            //! Force the Bolt command to run on the specifed device.  Default is "Automatic", in which case the Bolt
            //! runtime will select the device.  Forcing the mode to SerialCpu can be useful for debugging the algorithm.
            //! Forcing the mode can also be useful for performance comparisons or when the programmer wants direct 
            //! control over the run location (perhaps due to knowledge that the algorithm is best-suited for GPU).
			void forceRunMode(e_RunMode forceRunMode) { _forceRunMode = forceRunMode; };

			/*! Enable debug messages to be printed to stdout as the algorithm is compiled, run, and tuned.  See the #debug
			* namespace for a list of possible values.  Multiple debug options can be combined with the + sign as in 
			* following example - this technique should be used rather than separate calls to the debug() API 
			* each call resets the debug level rather than merging with the existing debug() setting.
			* \code
			* bolt::cl::control myControl;
			* // Show example of combining two debug options with '+' sign
			* myControl.debug(bolt::cl::control::debug::Compile + bolt::cl::control:debug::SaveCompilerTemps);
			* \endcode
			*/
			void debug(unsigned debug) { _debug = debug; };


			void wgPerComputeUnit(int wgPerComputeUnit) { _wgPerComputeUnit = wgPerComputeUnit; }; 
			
			//! 
			//! Specify the compile options which are passed to the OpenCL(TM) compiler
			void compileOptions(std::string &compileOptions) { _compileOptions = compileOptions; }; 

			// getters:
			::cl::CommandQueue commandQueue() const { return _commandQueue; };
			::cl::Context context() const { return _commandQueue.getInfo<CL_QUEUE_CONTEXT>();};
			::cl::Device device() const { return _commandQueue.getInfo<CL_QUEUE_DEVICE>();};
			e_UseHostMode useHost() const { return _useHost; };
			e_RunMode forceRunMode() const { return _forceRunMode; };
			unsigned debug() const { return _debug;};
			int const wgPerComputeUnit() const { return _wgPerComputeUnit; };
			const std::string compileOptions() const { return _compileOptions; };  

			bool compileForAllDevices() const { return _compileForAllDevices; };

			
			/*!
			  * Return default default \p control structure.  This structure is used for Bolt API calls where the user
			  * does not explicitly specify a \p control structure.  Also, newly created \p control structures copy
			  * the default structure for their initial values.  Note that changes to the default \p control structure
			  * are not automatically copied to already-created control structures.  Typically the default \p control
			  * structure is modified as part of the application initialiation, and then as other \p control structures
			  * are created they will pick up the modified defaults.  Some examples:
			  * \code
			  * bolt::cl::control myControl = bolt::cl::getDefault();  // copy existing default control.
			  * bolt::cl::control myControl;  // same as last line - the constructor also copies values from the default control
			  *
			  * // Modify a setting in the default \p control
			  * bolt::cl::control::getDefault().compileOptions("-g"); A
			  * \endcode
			  */
			static control &getDefault() 
			{
				// Default control structure; this can be accessed by the bolt::cl::control::getDefault()
				static control _defaultControl(true);
				return _defaultControl; 
			}; 
		private:

			// This is the private constructor is only used to create the initial default control structure.
			control(bool createGlobal) :
				_commandQueue(::cl::CommandQueue::getDefault()),
				_useHost(UseHost),
                _forceRunMode(Automatic),
				_debug(debug::None),
				_autoTune(AutoTuneAll),
				_wgPerComputeUnit(8),
				_compileForAllDevices(true)
			{};

		private:
			::cl::CommandQueue    _commandQueue;
			e_UseHostMode       _useHost;
            e_RunMode           _forceRunMode; 
			e_AutoTuneMode      _autoTune;  /* auto-tune the choice of device CPU/GPU and  workgroup shape */
			unsigned			_debug;

			int                 _wgPerComputeUnit;

			std::string			_compileOptions;  // extra options to pass to OpenCL compiler.

			bool				 _compileForAllDevices;  // compile for all devices in the context.  False means to only compile for specified device.
		};

	};
};

// Implementor note:
// When adding a new field to this structure, don't forget to:
//   * Add the new field, ie "int _foo.
//   * Add setter function and getter function, ie "void foo(int fooValue)" and "int foo const { return _foo; }"
//   * Add the field to the private constructor.  This is used to set the global default "_defaultControl".
//   * Add the field to the public constructor, copying from the _defaultControl.

// Sample usage:
// bolt::control c(myCmdQueue);
// c.debug(bolt::control::ShowCompile);
// bolt::cl::reduce(c, a.begin(), a.end(), std::plus<int>);
// 
//
// reduce (bolt::control(myCmdQueue), 

