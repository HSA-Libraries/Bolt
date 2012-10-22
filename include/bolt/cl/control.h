/***************************************************************************                                                                                     
*   Copyright 2012 Advanced Micro Devices, Inc.                                     
*                                                                                    
*   Licensed under the Apache License, Version 2.0 (the "License");   
*   you may not use this file except in compliance with the License.                 
*   You may obtain a copy of the License at                                          
*                                                                                    
*       http://www.apache.org/licenses/LICENSE-2.0                      
*                                                                                    
*   Unless required by applicable law or agreed to in writing, software              
*   distributed under the License is distributed on an "AS IS" BASIS,              
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.         
*   See the License for the specific language governing permissions and              
*   limitations under the License.                                                   

***************************************************************************/                                                                                     

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

        /*! The \p control class lets you control the parameters of a specific Bolt algorithm call, 
         such as the command-queue where GPU kernels run, debug information, load-balancing with 
		 the host, and more.  Each Bolt Algorithm call accepts the 
        \p control class as an optional first argument.  Additionally, Bolt contains a global default
		\p control structure that is used in cases where the \p control argument is not specified, and 
		developers can also modify this structure.  Some examples:

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


        * It can sometimes be useful to set the global default \p control structure that is used by Bolt algorithms 
		* calls that do not explicitly specify
        * a control parameter as the first argument.  For example, the application initialization routine can examine 
        * all the available GPU devies and select the one to be used for all subsequent Bolt calls.  This can easily be 
        * achieved by writing the global default \p control structure, i.e.:
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

            // Construct a new control structure, copying from default control for arguments that are not overridden.
            control(
                const ::cl::CommandQueue& commandQueue = getDefault().commandQueue(),
                e_UseHostMode useHost=getDefault().useHost(),
                unsigned debug=getDefault().debug()
                ) :
            m_commandQueue(commandQueue),
                m_useHost(useHost),
                m_forceRunMode(getDefault().m_forceRunMode),
                m_debug(debug),
                m_autoTune(getDefault().m_autoTune),
                m_wgPerComputeUnit(getDefault().m_wgPerComputeUnit),
                m_compileOptions(getDefault().m_compileOptions),
                m_compileForAllDevices(getDefault().m_compileForAllDevices)
            {};

            //setters:
            //! Set the OpenCL command queue (and associated device) for Bolt algorithms to use.  
            //! Only one command-queue can be specified for each call; Bolt does not load-balance across
            //! multiple command queues.  Bolt also uses the specified command queue to determine the OpenCL context and
            //! device.
            void commandQueue(::cl::CommandQueue commandQueue) { m_commandQueue = commandQueue; };

            //! If enabled, Bolt can use the host CPU to run parts of the algorithm.  If false, Bolt runs the
            //! entire algorithm using the device specified by the command-queue. This can be appropriate 
            //! on a discrete GPU, where the input data is located on the device memory.
            void useHost(e_UseHostMode useHost) { m_useHost = useHost; };


            //! Force the Bolt command to run on the specifed device.  Default is "Automatic", in which case the Bolt
            //! runtime selects the device.  Forcing the mode to SerialCpu can be useful for debugging the algorithm.
            //! Forcing the mode can also be useful for performance comparisons or for direct 
            //! control over the run location (perhaps due to knowledge that the algorithm is best-suited for GPU).
            void forceRunMode(e_RunMode forceRunMode) { m_forceRunMode = forceRunMode; };

            /*! Enable debug messages to be printed to stdout as the algorithm is compiled, run, and tuned.  See the #debug
            * namespace for a list of values.  Multiple debug options can be combined with the + sign, as in 
            * following example.  Use this technique rather than separate calls to the debug() API; 
            * each call resets the debug level rather than merging with the existing debug() setting.
            * \code
            * bolt::cl::control myControl;
            * // Show example of combining two debug options with '+' sign
            * myControl.debug(bolt::cl::control::debug::Compile + bolt::cl::control:debug::SaveCompilerTemps);
            * \endcode
            */
            void debug(unsigned debug) { m_debug = debug; };


            void wgPerComputeUnit(int wgPerComputeUnit) { m_wgPerComputeUnit = wgPerComputeUnit; }; 
            
            //! 
            //! Specify the compile options which are passed to the OpenCL(TM) compiler
            void compileOptions(std::string &compileOptions) { m_compileOptions = compileOptions; }; 

            // getters:
            ::cl::CommandQueue& commandQueue( ) { return m_commandQueue; };
            const ::cl::CommandQueue& commandQueue( ) const { return m_commandQueue; };

            ::cl::Context context() const { return m_commandQueue.getInfo<CL_QUEUE_CONTEXT>();};
            ::cl::Device device() const { return m_commandQueue.getInfo<CL_QUEUE_DEVICE>();};
            e_UseHostMode useHost() const { return m_useHost; };
            e_RunMode forceRunMode() const { return m_forceRunMode; };
            unsigned debug() const { return m_debug;};
            int const wgPerComputeUnit() const { return m_wgPerComputeUnit; };
            const std::string compileOptions() const { return m_compileOptions; };  

            bool compileForAllDevices() const { return m_compileForAllDevices; };

            /*!
              * Return default default \p control structure.  This structure is used for Bolt API calls when the user
              * does not explicitly specify a \p control structure.  Also, newly created \p control structures copy
              * the default structure for their initial values.  Note that changes to the default \p control structure
              * are not automatically copied to already-created control structures.  Typically, the default \p control
              * structure is modified as part of the application initialiation; then, as other \p control structures
              * are created, they pick up the modified defaults.  Some examples:
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
                static control _defaultControl( true );
                return _defaultControl; 
            };

            static void printPlatforms( bool printDevices = true, cl_device_type deviceType = CL_DEVICE_TYPE_ALL );
            static void printPlatformsRange( std::vector< ::cl::Platform >::iterator begin, std::vector< ::cl::Platform >::iterator end, 
                                            bool printDevices = true, cl_device_type deviceType = CL_DEVICE_TYPE_ALL );

               /*! \brief Convenience method to help users create and initialize an OpenCL CommandQueue
                * \todo The default commandqueue is created with a context that contains all GPU devices in platform.  Since kernels
                * are only compiled on first invocation, switching between GPU devices is OK, but switching to a CPU 
                * device afterwards causes an exception because the kernel was not compiled for CPU.  Should we provide 
                * more options and expose more intefaces to the user?
                */
            static ::cl::CommandQueue getDefaultCommandQueue( );

        private:

            // This is the private constructor is only used to create the initial default control structure.
            control(bool createGlobal) :
                m_commandQueue( getDefaultCommandQueue( ) ),
                m_useHost(UseHost),
                m_forceRunMode(Automatic),
                m_debug(debug::None),
                m_autoTune(AutoTuneAll),
                m_wgPerComputeUnit(8),
                m_compileForAllDevices(true)
            {};

            ::cl::CommandQueue  m_commandQueue;
            e_UseHostMode       m_useHost;
            e_RunMode           m_forceRunMode; 
            e_AutoTuneMode      m_autoTune;  /* auto-tune the choice of device CPU/GPU and  workgroup shape */
            unsigned            m_debug;
            int                 m_wgPerComputeUnit;
            std::string         m_compileOptions;  // extra options to pass to OpenCL compiler.
            bool                m_compileForAllDevices;  // compile for all devices in the context.  False means to only compile for specified device.
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

