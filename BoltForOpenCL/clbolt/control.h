#pragma once

#include <CL/cl.hpp>

namespace clbolt {


	class control {
	public:
		enum e_UseHostMode {NoUseHost=0, UseHost=1};  /* Use host CPU for parts of the algorithm that likely run best on the CPU */
		enum e_AutoTuneMode{NoAutoTune=0, AutoTuneDevice=0x1, AutoTuneWorkShape=0x2, AutoTuneAll=0x3};
		struct debug {
			static const unsigned None=0;
			static const unsigned Compile = 0x1;
			static const unsigned SaveCompilerTemps = 0x2;
			static const unsigned AutoTune = 0x4;
		};
	public:

		// Construct a new control structure, copying from default control.
		control(
			cl::CommandQueue commandQueue=_defaultControl.commandQueue(),
			e_UseHostMode useHost=_defaultControl.useHost(),
			unsigned debug=_defaultControl.debug()
			) :
			_commandQueue(commandQueue),
			_useHost(useHost),
			_debug(debug),
			_autoTune(_defaultControl._autoTune),
			_wgPerComputeUnit(_defaultControl._wgPerComputeUnit)
		{};

		// getters:
		cl::CommandQueue commandQueue() const { return _commandQueue; };
		cl::Context context() const { return _commandQueue.getInfo<CL_QUEUE_CONTEXT>();};
		cl::Device device() const { return _commandQueue.getInfo<CL_QUEUE_DEVICE>();};
		e_UseHostMode useHost() const { return _useHost; };
		unsigned debug() const { return _debug;};
		int const wgPerComputeUnit() const { return _wgPerComputeUnit; };

		//setters:
		void debug(unsigned debug) { _debug = debug; };

		static control &getDefault() {return _defaultControl; }; 
	private:

		// This constructor is only used to create the initial global default control structure.
		control(bool createGlobal,
			cl::CommandQueue commandQueue=cl::CommandQueue::getDefault(), 
			e_UseHostMode useHost=UseHost,
			unsigned debug=debug::None
			) :
			_commandQueue(commandQueue),
			_useHost(useHost),
			_debug(debug),
			_autoTune(AutoTuneAll),
			_wgPerComputeUnit(8)
		{};

	private:
		cl::CommandQueue    _commandQueue;
		e_UseHostMode       _useHost;
		e_AutoTuneMode      _autoTune;  /* auto-tune the choice of device CPU/GPU and  workgroup shape */
		unsigned         _debug;

		int                 _wgPerComputeUnit;

		// This is the default for all bolt calls if no user-specified default is specified.  It uses the default constructor.
		static control      _defaultControl;
	};


	// queue->device->
	//device->platform.  (CL_DEVICE_PLATFORM)
};



// Sample usage:
// bolt::control c(myCmdQueue);
// c.debug(bolt::control::ShowCompile);
// clbolt::reduce(c, a.begin(), a.end(), std::plus<int>);
// 
//
// reduce (bolt::control(myCmdQueue), 

