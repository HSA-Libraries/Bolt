#pragma once

#include <CL/cl.hpp>

namespace bolt {
	namespace cl {

		/*! \addtogroup control
		 * \{
		*/

		/*!
		 \p control allows user to control the paramters of a specific API call, such as the command-queue where GPU kernels run, 
		 enabling of debug information, controlling load-balancing with the host, etc.
		 */
		class control {
		public:
			enum e_UseHostMode {NoUseHost=0, UseHost=1};  /* Use host CPU for parts of the algorithm that likely run best on the CPU */
			enum e_AutoTuneMode{NoAutoTune=0, AutoTuneDevice=0x1, AutoTuneWorkShape=0x2, AutoTuneAll=0x3};
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
				_debug(debug),
				_autoTune(getDefault()._autoTune),
				_wgPerComputeUnit(getDefault()._wgPerComputeUnit),
				_compileOptions(getDefault()._compileOptions),
				_compileForAllDevices(getDefault()._compileForAllDevices)
			{};

			// getters:
			::cl::CommandQueue commandQueue() const { return _commandQueue; };
			::cl::Context context() const { return _commandQueue.getInfo<CL_QUEUE_CONTEXT>();};
			::cl::Device device() const { return _commandQueue.getInfo<CL_QUEUE_DEVICE>();};
			e_UseHostMode useHost() const { return _useHost; };
			unsigned debug() const { return _debug;};
			int const wgPerComputeUnit() const { return _wgPerComputeUnit; };
			const std::string compileOptions() const { return _compileOptions; };
			bool compileForAllDevices() const { return _compileForAllDevices; };

			//setters:
			void commandQueue(::cl::CommandQueue commandQueue) { _commandQueue = commandQueue; };
			void useHost(e_UseHostMode useHost) { _useHost = useHost; };
			void debug(unsigned debug) { _debug = debug; };
			void wgPerComputeUnit(int wgPerComputeUnit) { _wgPerComputeUnit = wgPerComputeUnit; }; 
			void compileOptions(std::string &compileOptions) { _compileOptions = compileOptions; };

			static control &getDefault() 
			{
				// Default control structure; this can be accessed by the bolt::cl::control::getDefault()
				static control _defaultControl(true);
				return _defaultControl; 
			}; 
		private:

			// This constructor is only used to create the initial global default control structure.
			control(bool createGlobal) :
				_commandQueue(::cl::CommandQueue::getDefault()),
				_useHost(UseHost),
				_debug(debug::None),
				_autoTune(AutoTuneAll),
				_wgPerComputeUnit(8),
				_compileForAllDevices(true)
			{};

		private:
			::cl::CommandQueue    _commandQueue;
			e_UseHostMode       _useHost;
			e_AutoTuneMode      _autoTune;  /* auto-tune the choice of device CPU/GPU and  workgroup shape */
			unsigned			_debug;

			int                 _wgPerComputeUnit;

			std::string			_compileOptions;  // extra options to pass to OpenCL compiler.

			bool				 _compileForAllDevices;  // compile for all devices in the context.  False means to only compile for specified device.
		};


		// queue->device->
		//device->platform.  (CL_DEVICE_PLATFORM)
	};
};

// Implementor note:
// When adding a new field to this structure, don't forget to:
//   * Add the new field, ie "int _foo.
//   * Add setter function and getter function.
//   * Add the field to the private constructor.  This is used to set the global "_defaultControl".
//   * Add the field to the public constructor, copying from the _defaultControl.

// Sample usage:
// bolt::control c(myCmdQueue);
// c.debug(bolt::control::ShowCompile);
// bolt::cl::reduce(c, a.begin(), a.end(), std::plus<int>);
// 
//
// reduce (bolt::control(myCmdQueue), 

