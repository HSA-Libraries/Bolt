/******************************************************************************
 * Asynchronous Profiler 
 *****************************************************************************/
#include "bolt/AsyncProfiler.h"
#include <iostream>

size_t AsyncProfiler::getTime()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter( &currentTime );
    return static_cast<size_t>( (currentTime.QuadPart - constructionTimeStamp.QuadPart)*timerPeriodNs);
}

/******************************************************************************
 * Step Class
 *****************************************************************************/

char *AsyncProfiler::attributeNames[] = {
    "ID",
    "Device",
    "Time[ns]",
    "Bytes",
    "Bytes/s",
    "Flops",
    "Flops/s",
    "Start[ns]",
    "Stop[ns]"};

char *AsyncProfiler::trialAttributeNames[] = {
    "ID",
    "Device",
    "Tot Time[ns]",
    "Tot Bytes",
    "Agg Bytes/s",
    "Tot Flops",
    "Agg Flops/s",
    "Start[ns]",
    "Stop[ns]",

};

AsyncProfiler::Step::Step( )
{
    for( int i = 0; i < NUM_ATTRIBUTES; i++ )
    {
        attributeValues[i] = 0;
        stdDev[i] = 0;
    }
}

AsyncProfiler::Step::~Step( )
{
    // none
}

void AsyncProfiler::Step::set( size_t index, size_t value)
{
    if (index >= 0 && index < NUM_ATTRIBUTES)
    {
        attributeValues[index] = value;
    }
    else
    {
        ::std::cerr << "Out-Of-Bounds: attributeIndex " << index << " not in [" << 0 << ", " << NUM_ATTRIBUTES << "]; Line: " << __LINE__ << " of File: " << __FILE__ << std::endl;
    }
}
size_t AsyncProfiler::Step::get( size_t index ) const
{
    return attributeValues[index];
}
void AsyncProfiler::Step::setName( const ::std::string& name )
{
    stepName = name;
}
std::string AsyncProfiler::Step::getName( ) const
{
    return stepName;
}
::std::ostream& AsyncProfiler::Step::writeLog( ::std::ostream& s ) const
{
    s << "\t\t<STEP";
    //s << " serial=\"" << serial << "\"";
    s << " name=\"" << stepName.c_str() << "\"";
    s << ">";
    s << std::endl;
    for (size_t i = 0; i < NUM_ATTRIBUTES; i++)
    {
        if (attributeValues[i] > 0 || i==device)
        {
            s << "\t\t\t<ATTR";
            s << " name=\"" << AsyncProfiler::attributeNames[i] << "\"";
            s << " value=\"" << attributeValues[i] << "\"";
            if ( stdDev[i] > 0)
            {
                s << " stddev=\"" << stdDev[i] << "\"";
            }
            s << " />";
            s << std::endl;
        }
    }
    s << "\t\t</STEP>";
    s << std::endl;
    return s;
}

void AsyncProfiler::Step::computeDerived()
{
    // time
    if (attributeValues[time] == 0)
    attributeValues[time] = attributeValues[stopTime] - attributeValues[startTime];
    //std::cout << "time:" << attributeValues[time] << " = stop:" << attributeValues[stopTime] << " - start:" << attributeValues[startTime] << std::endl;

    // flops / sec
    if (attributeValues[flops_s] == 0)
    {
    attributeValues[flops_s] = static_cast<size_t>(1000000000.0 * attributeValues[flops] / attributeValues[time]);
    //std::cout << "Flops/s: " << attributeValues[flops_s] << " now" << std::endl;
    }
    else
    {
        //std::cout << "Flops/s: " << attributeValues[flops_s] << " already" << std::endl;
    }

    // bandwidth [bytes / sec]
    if (attributeValues[bandwidth] == 0)
    attributeValues[bandwidth] = static_cast<size_t>(1000000000.0 * attributeValues[memory] / attributeValues[time]);
}


/******************************************************************************
 * Trial Class
 *****************************************************************************/

AsyncProfiler::Trial::Trial(void) : currentStepIndex( 0 )
{
    for( int i = 0; i < NUM_ATTRIBUTES; i++ )
    {
        attributeValues[i] = 0;
    }
    steps.resize( 1 );
}
AsyncProfiler::Trial::Trial( size_t n )
{
    for( int i = 0; i < NUM_ATTRIBUTES; i++ )
    {
        attributeValues[i] = 0;
    }
    steps.resize(n);
}
void AsyncProfiler::Trial::resize( size_t n )
{
    steps.resize(n);
}
AsyncProfiler::Trial::~Trial(void)
{
    // none
}
size_t AsyncProfiler::Trial::size() const
{
    return steps.size();
}

size_t AsyncProfiler::Trial::get( size_t attributeIndex) const
{
    return steps[currentStepIndex].get(attributeIndex);
}
size_t AsyncProfiler::Trial::get( size_t stepIndex, size_t attributeIndex) const
{
    if (stepIndex >= 0 && stepIndex < steps.size())
    {
        return steps[stepIndex].get(attributeIndex );
    }
    else
    {
        ::std::cerr << "Out-Of-Bounds: stepIndex " << stepIndex << " not in [" << 0 << ", " << steps.size() << "]; Line: " << __LINE__ << " of File: " << __FILE__ << std::endl;
        return 0;
    }
}

void AsyncProfiler::Trial::set( size_t attributeIndex, size_t attributeValue)
{
    steps[currentStepIndex].set(attributeIndex, attributeValue);
}

void AsyncProfiler::Trial::set( size_t stepIndex, size_t attributeIndex, size_t attributeValue)
{
    if (stepIndex >= 0 && stepIndex < steps.size())
    {
        steps[stepIndex].set(attributeIndex, attributeValue);
    }
    else
    {
        ::std::cerr << "Out-Of-Bounds: stepIndex " << stepIndex << " not in [" << 0 << ", " << steps.size() << "]; Line: " << __LINE__ << " of File: " << __FILE__ << std::endl;
    }
}
void AsyncProfiler::Trial::setSerial( size_t s )
{
    serial = s;
}
void AsyncProfiler::Trial::startStep()
{
    //steps[currentStepIndex].set( id, currentStepIndex );
}
size_t AsyncProfiler::Trial::nextStep()
{
    //steps[currentStepIndex].computeDerived();
    currentStepIndex++;
    Step tmp;
    steps.push_back( tmp );
    //steps[currentStepIndex].set( id, currentStepIndex );
    return currentStepIndex;
}
void AsyncProfiler::Trial::computeStepsDerived()
{
    for (size_t i = 0; i < steps.size(); i++)
    {
        steps[i].computeDerived();
    }
}
void AsyncProfiler::Trial::computeAttributes()
{
    //std::cout << "computing derived" << std::endl;
    attributeValues[startTime] = steps[0].get(startTime);
    attributeValues[stopTime] = steps[steps.size()-1].get(stopTime);
    attributeValues[time] = attributeValues[stopTime] - attributeValues[startTime];
    attributeValues[flops] = 0;
    attributeValues[memory] = 0;
    for (size_t i = 0; i < steps.size(); i++)
    {
        //std::cout << "\tadding " << steps[i].get(time) << " ns" << std::endl;
        attributeValues[flops] += steps[i].get(flops); //steps[i].get(flops_s) * steps[i].get(time);
        attributeValues[memory] += steps[i].get(memory); //steps[i].get(bandwidth) * steps[i].get(time);
        //std::cout << "\tadding " << steps[i].get(bandwidth)* steps[i].get(time) << " bytes" << std::endl;
    }
    
    attributeValues[flops_s]   = static_cast<size_t>(1000000000.0*attributeValues[flops] / attributeValues[time]);
    attributeValues[bandwidth] = static_cast<size_t>(1000000000.0*attributeValues[memory] / attributeValues[time]);

    //attributeValues[flops] /= 1000000000;
    //attributeValues[memory] /= 1000000000;
}

::std::ostream& AsyncProfiler::Trial::writeLog( ::std::ostream& s ) const
{
    s << "\t<TRIAL";
    s << " serial=\"" << serial << "\"";
    s << ">";
    s << std::endl;

    for (size_t i = 0; i < NUM_ATTRIBUTES; i++)
    {
        if (attributeValues[i] > 0 )
        {
            s << "\t\t<ATTR";
            s << " name=\"" << AsyncProfiler::trialAttributeNames[i] << "\"";
            s << " value=\"" << attributeValues[i] << "\"";
            if ( stdDev[i] > 0)
            {
                s << " stddev=\"" << stdDev[i] << "\"";
            }
            s << " />";
            s << std::endl;
        }
    }


    for (size_t i = 0; i < steps.size(); i++)
    {
        steps[i].writeLog(s);
    }
    s << "\t</TRIAL>";
    s << std::endl;
    return s;
}
size_t AsyncProfiler::Trial::getStepNum() const
{
    return currentStepIndex;
}
void AsyncProfiler::Trial::setStepName( const ::std::string& name)
{
    steps[currentStepIndex].setName( name );
}
void AsyncProfiler::Trial::setStepName( size_t stepNum, const ::std::string& name)
{
    steps[stepNum].setName( name );
}
std::string AsyncProfiler::Trial::getStepName( ) const
{
    return steps[currentStepIndex].getName( );
}
std::string AsyncProfiler::Trial::getStepName( size_t stepNum ) const
{
    return steps[stepNum].getName( );
}

AsyncProfiler::Step& AsyncProfiler::Trial::operator[](size_t idx)
{
    return steps[idx];
}

/******************************************************************************
 * AsyncProfiler Class
 *****************************************************************************/

AsyncProfiler::AsyncProfiler(void) : currentTrialIndex( 0 )
{
    trials.resize( 0 );
    QueryPerformanceCounter( &constructionTimeStamp );

    LARGE_INTEGER freq;
    QueryPerformanceFrequency( &freq ); // clicks per sec
    timerPeriodNs = 1000000000 / freq.QuadPart; // clocks per ns
    //std::cout << "FreqSec=" << freq.QuadPart << ", FreqNs=" << timerFrequency;
    //std::cout << "Timer Resolution = " << timerPeriodNs << " ns / click\n";
    //std::cout << "AsyncProfiler constructed" << std::endl;
    architecture="";
}

AsyncProfiler::AsyncProfiler(std::string profName) : currentTrialIndex( 0 ), name(profName)
{
    trials.resize( 0 );
    QueryPerformanceCounter( &constructionTimeStamp );

    LARGE_INTEGER freq;
    QueryPerformanceFrequency( &freq ); // clicks per sec
    timerPeriodNs = 1000000000 / freq.QuadPart; // clocks per ns
    //std::cout << "FreqSec=" << freq.QuadPart << ", FreqNs=" << timerFrequency;
    //std::cout << "Timer Resolution = " << timerPeriodNs << " ns / click\n";
    //std::cout << "AsyncProfiler constructed" << std::endl;
    architecture="";
}

AsyncProfiler::~AsyncProfiler(void)
{
    //std::cout << "AsyncProfiler destructed" << std::endl;
}


size_t AsyncProfiler::getNumTrials() const
{
    return trials.size();
}

size_t AsyncProfiler::getNumSteps() const
{
    if (trials.size() < 1)
    {
        return 0;
    }
    else
    {
        return trials[0].size();
    }
}

void AsyncProfiler::stopTrial()
{
    set( stopTime, getTime() ); // prev step stops
    //trials[currentTrialIndex].computeStepsDerived();
    //trials[currentTrialIndex].computeTrialDerived();
    currentTrialIndex++;
}
void AsyncProfiler::startTrial()
{
    Trial tmp;
    trials.push_back( tmp );
    trials[currentTrialIndex].setSerial( currentTrialIndex );
    trials[currentTrialIndex].startStep();
    set( startTime, getTime() );
}
void AsyncProfiler::nextTrial()
{
    stopTrial();
    startTrial();
}

void AsyncProfiler::nextStep()
{
    set( stopTime, getTime() ); // prev step stops
    trials[currentTrialIndex].nextStep();
    set( startTime, getTime() ); // next step starts
}

size_t AsyncProfiler::get( size_t attributeIndex) const
{
    return trials[currentTrialIndex].get( attributeIndex );
}

size_t AsyncProfiler::get( size_t stepIndex, size_t attributeIndex) const
{
    return trials[currentTrialIndex].get( stepIndex, attributeIndex );
}

size_t AsyncProfiler::get( size_t trialIndex, size_t stepIndex, size_t attributeIndex) const
{
    if (trialIndex >= 0 && trialIndex < trials.size() )
    {
        return trials[trialIndex].get( stepIndex, attributeIndex );
    }
    else
    {
        ::std::cerr << "Out-Of-Bounds: trialIndex " << trialIndex << " not in [" << 0 << ", " << trials.size() << "]; Line: " << __LINE__ << " of File: " << __FILE__ << std::endl;
        return 0;
    }
}

// current step of current trial
void AsyncProfiler::set( size_t attributeIndex, size_t attributeValue)
{
    trials[currentTrialIndex].set(attributeIndex, attributeValue); 
}

// specified step of current trial
void AsyncProfiler::set( size_t stepIndex, size_t attributeIndex, size_t attributeValue)
{
    trials[currentTrialIndex].set( stepIndex, attributeIndex, attributeValue);
}

// specified step of specified trial
void AsyncProfiler::set( size_t trialIndex, size_t stepIndex, size_t attributeIndex, size_t attributeValue)
{
    if (trialIndex >= 0 && trialIndex < trials.size() )
    {
        trials[trialIndex].set( stepIndex, attributeIndex, attributeValue);
    }
    else
    {
        ::std::cerr << "Out-Of-Bounds: trialIndex " << trialIndex << " not in [" << 0 << ", " << trials.size() << "]; Line: " << __LINE__ << " of File: " << __FILE__ << std::endl;
    }
}

size_t AsyncProfiler::getTrialNum() const
{
    return currentTrialIndex;
}

size_t AsyncProfiler::getStepNum() const
{
    return trials[currentTrialIndex].getStepNum();
}

void AsyncProfiler::setStepName( const ::std::string& name)
{
    trials[currentTrialIndex].setStepName( name );
}

void AsyncProfiler::setDataSize( size_t d )
{
    dataSize = d;
}

void AsyncProfiler::setArchitecture( std::string a )
{
    architecture = a;
}
void AsyncProfiler::setName( std::string n )
{
    name = n;
}









void AsyncProfiler::end()
{
    // compute derived types for all steps within all trials
    for (size_t i = 0; i < trials.size(); i++)
    {
        trials[i].computeStepsDerived();
    }

    // compute attributes for all trials
    for (size_t i = 0; i < trials.size(); i++)
    {
        trials[i].computeAttributes();
    }

    calculateAverage();
}

void AsyncProfiler::calculateAverage()
{
    if (trials.size() == 0) return;
    //std::cout << "########################################################################" << std::endl;
    //std::cout << "Calculating Average" << std::endl;
    size_t numSteps = getNumSteps();
    average.resize(numSteps);
    average.setSerial(0);
    Trial total(numSteps), count(numSteps);

    // sum step attributes
    size_t trialsAveraged = 0;
    size_t totalTime = 0;
    size_t totalMemory = 0;
    size_t totalFlops = 0;
    for (size_t t = (trials.size()>1) ? 1 : 0; t < trials.size(); t++)
    {
        trialsAveraged++;
        for (size_t s = 0; s < trials[t].size(); s++)
        {
            // time
            size_t oTime = get(t, s, time);
            size_t uTime = total.get(s, time) + oTime;
            total.set(s, time, uTime );
            count.set(s, time, count.get(s, time)+1 );
            totalTime += oTime;

            // flops
            size_t oFlops= get(t, s, flops);
            size_t uFlops = total.get(s, flops) + oFlops;
            total.set(s, flops, uFlops );
            count.set(s, flops, count.get(s, flops)+1 );
            totalFlops += oFlops;

            // memory
            size_t oMemory = get(t, s, memory);
            size_t uMemory = total.get(s, memory) + oMemory;
            total.set(s, memory, uMemory );
            count.set(s, memory, count.get(s, memory)+1 );
            totalMemory += oMemory;
        }
    }

    // assign average trial attributes
    average.attributeValues[time] = totalTime / trialsAveraged;
    average.attributeValues[flops] = totalFlops / trialsAveraged;
    average.attributeValues[memory] = totalMemory / trialsAveraged;
    average.attributeValues[flops_s] = static_cast<size_t>(1000000000.0 * totalFlops / totalTime);
    average.attributeValues[bandwidth] = static_cast<size_t>(1000000000.0 * totalMemory / totalTime);
    //std::cout
    //    << "bandwidth:" << average.attributeValues[bandwidth] << " = "
    //    << "memory:" << totalMemory << " / "
    //    << "time:" << totalTime << " x1000000000" << std::endl;

    // assign average step attributes
    for (size_t s = 0; s < total.size(); s++)
    {
        average.setStepName( s, trials[0].getStepName( s ) );
        for (size_t a = 0; a < NUM_ATTRIBUTES; a++)
        {
            size_t n = count.get(s, a);
            if (n > 0)
            {
                size_t tot = total.get(s, a);
                average.set(s, a, tot / n);
            }
        }
    }
    //std::cout << "########################################################################" << std::endl;
    //std::cout << "Calculating Derived" << std::endl;
    average.computeStepsDerived();

    // accumulate for standard deviation
    for (size_t t = (trials.size()>1) ? 1 : 0; t < trials.size()-1; t++)
    {
        // accumulate for trial
        size_t dTime = trials[t].attributeValues[time] - average.attributeValues[time];
        average.stdDev[time] += dTime * dTime;
        size_t dBandwidth = trials[t].attributeValues[bandwidth] - average.attributeValues[bandwidth];
        average.stdDev[bandwidth] += dBandwidth * dBandwidth;
        size_t dFlops_s = trials[t].attributeValues[flops_s] - average.attributeValues[flops_s];
        average.stdDev[flops_s] += dFlops_s * dFlops_s;

        // accumulate for steps
        for (size_t s = 0; s < trials[t].size(); s++)
        {
            // time
            size_t sTime = get(t, s, time);
            size_t avgTime = average.get(s, time);
            size_t diffTime = sTime - avgTime;
            average[s].stdDev[time] += diffTime*diffTime;

            // flops_s
            size_t sFlops_s = get(t, s, flops_s);
            size_t avgFlops_s = average.get(s, flops_s);
            size_t diffFlops_s = sFlops_s - avgFlops_s;
            average[s].stdDev[flops_s] += diffFlops_s*diffFlops_s;

            // bandwidth
            size_t sBandwidth = get(t, s, bandwidth);
            size_t avgBandwidth = average.get(s, bandwidth);
            size_t diffBandwidth = sBandwidth - avgBandwidth;
            average[s].stdDev[bandwidth] += diffBandwidth*diffBandwidth;
        }
    }

    // final divide for trial stddev
    // time
    average.stdDev[time] = static_cast<size_t>(sqrt(average.stdDev[time]/(trialsAveraged-1.0) ));
    // flops_s
    average.stdDev[flops_s] = static_cast<size_t>(sqrt(average.stdDev[flops_s]/(trialsAveraged-1.0) ));
    // bandwidth
    average.stdDev[bandwidth] = static_cast<size_t>(sqrt(average.stdDev[bandwidth]/(trialsAveraged-1.0) ));
    //std::cout << "Size of Average Trials: " << average.size() << std::endl;
    // final divide for steps stddev
    for (size_t s = 0; s < average.size(); s++)
    {
        // time
        average[s].stdDev[time] = static_cast<size_t>(sqrt(average[s].stdDev[time]/(count.get(s,time)-1.0) ));
        // flops_s
        average[s].stdDev[flops_s] = static_cast<size_t>(sqrt(average[s].stdDev[flops_s]/(count.get(s,flops)-1.0) ));
        // bandwidth
        average[s].stdDev[bandwidth] = static_cast<size_t>(sqrt(average[s].stdDev[bandwidth]/(count.get(s,memory)-1.0) ));
    }

}



::std::ostream& AsyncProfiler::writeSum( ::std::ostream& os ) const
{
    os << "<PROFILE";
    os << " name=\"" << name.c_str() << "\"";
    os << " type=\"average\"";
    os << " trials=\"" << ((trials.size()>1) ? trials.size()-1 : 1) << "\"";
    os << " data[bytes]=\"" << dataSize << "\"";
    os << " timerResolution=\"" << timerPeriodNs << "\"";
    os << " architecture=\"" << architecture.c_str() << "\"";
    os << " >";
    os << std::endl;
    average.writeLog(os);
    os << "</PROFILE>";
    os << std::endl;
    return os;
}


::std::ostream& AsyncProfiler::writeLog( ::std::ostream& os ) const
{
    os << "<PROFILE";
    os << " name=\"" << name.c_str() << "\"";
    os << " type=\"Log\"";
    os << " trials=\"" << ((trials.size()>1) ? trials.size()-1 : 1) << "\"";
    os << " data[bytes]=\"" << dataSize << "\"";
    os << " timerResolution[ns]=\"" << timerPeriodNs << "\"";
    os << " architecture=\"" << architecture.c_str() << "\"";
    os << " >";
    os << std::endl;
    for (size_t t = 0; t < trials.size(); t++)
    {
      trials[t].writeLog(os);
    }
    os << "</PROFILE>";
    os << std::endl;
    return os;
}

::std::ostream& AsyncProfiler::write( ::std::ostream& os ) const
{
    writeLog(os);
    writeSum(os);
    return os;
}