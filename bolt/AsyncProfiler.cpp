/******************************************************************************
 * Asynchronous Profiler 
 *****************************************************************************/
#include "AsyncProfiler.h"
#include <iostream>

unsigned int AsyncProfiler::getTime()
{
  LARGE_INTEGER currentTime;
  QueryPerformanceCounter( &currentTime );
  std::cout << "time=" << currentTime.QuadPart << ", cons=" << constructionTimeStamp.QuadPart << ", diff=" << currentTime.QuadPart-constructionTimeStamp.QuadPart << std::endl;
  return static_cast<unsigned int>( double(currentTime.QuadPart - constructionTimeStamp.QuadPart) / timerFrequency );
}

/******************************************************************************
 * Step Class
 *****************************************************************************/

char *AsyncProfiler::attributeNames[] = {
  "ID",
  "StartTime[ns]",
  "StopTime[ns]",
  "MemoryAccesses[bytes]",
  "Device",
  "#Flops",
  "Time[ns]",
  "Bandwidth[bytes/s]",
  "Flops/s"};

AsyncProfiler::Step::Step( )
{
  for( int i = 0; i < NUM_ATTRIBUTES; i++ )
  {
    attributeValues[i] = 0;
  }
}

AsyncProfiler::Step::~Step( )
{
  // none
}

void AsyncProfiler::Step::set( unsigned int index, unsigned int value)
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
unsigned int AsyncProfiler::Step::get( unsigned int index ) const
{
  return attributeValues[index];
}
void AsyncProfiler::Step::setName( const ::std::string& name )
{
  stepName = name;
}
void AsyncProfiler::Step::setSerial( unsigned int s )
{
  serial = s;
}
::std::ostream& AsyncProfiler::Step::writeLog( ::std::ostream& s ) const
{
  s << "\t\t<STEP";
  s << " serial=\"" << serial << "\"";
  s << " name=\"" << stepName.c_str() << "\"";
  s << ">";
  s << std::endl;
  for (unsigned int i = 0; i < NUM_ATTRIBUTES; i++)
  {
    if (attributeValues[i] > 0)
    {
      s << "\t\t\t<ATTR";
      s << " name=\"" << AsyncProfiler::attributeNames[i] << "\"";
      s << " value=\"" << attributeValues[i] << "\" />";
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
  attributeValues[time] = attributeValues[stopTime] - attributeValues[startTime];

  // flops / sec
  attributeValues[flops_s] = 1000000000 * attributeValues[flops] / attributeValues[time];

  // bandwidth [bytes / sec]
  attributeValues[bandwidth] = attributeValues[memory] / attributeValues[time];
}


/******************************************************************************
 * Trial Class
 *****************************************************************************/

AsyncProfiler::Trial::Trial(void) : currentStepIndex( 0 )
{
  steps.resize( 1 );
}
AsyncProfiler::Trial::Trial( size_t n )
{
  steps.resize(n);
}
AsyncProfiler::Trial::~Trial(void)
{
  // none
}
unsigned int AsyncProfiler::Trial::size() const
{
  return steps.size();
}

unsigned int AsyncProfiler::Trial::get( unsigned int attributeIndex) const
{
  return steps[currentStepIndex].get(attributeIndex);
}
unsigned int AsyncProfiler::Trial::get( unsigned int stepIndex, unsigned int attributeIndex) const
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

void AsyncProfiler::Trial::set( unsigned int attributeIndex, unsigned int attributeValue)
{
  steps[currentStepIndex].set(attributeIndex, attributeValue);
}

void AsyncProfiler::Trial::set( unsigned int stepIndex, unsigned int attributeIndex, unsigned int attributeValue)
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
void AsyncProfiler::Trial::setSerial( unsigned int s )
{
  serial = s;
}
void AsyncProfiler::Trial::startStep()
{
  steps[currentStepIndex].setSerial( currentStepIndex );
}
unsigned int AsyncProfiler::Trial::nextStep()
{
  steps[currentStepIndex].computeDerived();
  currentStepIndex++;
  steps.emplace_back();
  steps[currentStepIndex].setSerial( currentStepIndex );
  return currentStepIndex;
}
::std::ostream& AsyncProfiler::Trial::writeLog( ::std::ostream& s ) const
{
  s << "\t<TRIAL";
  s << " serial=\"" << serial << "\"";
  s << ">";
  s << std::endl;
  for (unsigned int i = 0; i < steps.size(); i++)
  {
    steps[i].writeLog(s);
  }
  s << "\t</TRIAL>";
  s << std::endl;
  return s;
}
unsigned int AsyncProfiler::Trial::getStepNum() const
{
  return currentStepIndex;
}
void AsyncProfiler::Trial::setStepName( const ::std::string& name)
{
  steps[currentStepIndex].setName( name );
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
  timerFrequency = freq.QuadPart / 1000000000.0; // clocks per ns
  std::cout << "FreqSec=" << freq.QuadPart << ", FreqNs=" << timerFrequency;
  
  std::cout << "AsyncProfiler constructed" << std::endl;
}

AsyncProfiler::~AsyncProfiler(void)
{
  std::cout << "AsyncProfiler destructed" << std::endl;
}


unsigned int AsyncProfiler::getNumTrials() const
{
  return trials.size();
}

unsigned int AsyncProfiler::getNumSteps() const
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
  currentTrialIndex++;
}
void AsyncProfiler::startTrial()
{
  trials.emplace_back();
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

unsigned int AsyncProfiler::get( unsigned int attributeIndex) const
{
  return trials[currentTrialIndex].get( attributeIndex );
}

unsigned int AsyncProfiler::get( unsigned int stepIndex, unsigned int attributeIndex) const
{
  return trials[currentTrialIndex].get( stepIndex, attributeIndex );
}

unsigned int AsyncProfiler::get( unsigned int trialIndex, unsigned int stepIndex, unsigned int attributeIndex) const
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
void AsyncProfiler::set( unsigned int attributeIndex, unsigned int attributeValue)
{
  trials[currentTrialIndex].set(attributeIndex, attributeValue); 
}

// specified step of current trial
void AsyncProfiler::set( unsigned int stepIndex, unsigned int attributeIndex, unsigned int attributeValue)
{
  trials[currentTrialIndex].set( stepIndex, attributeIndex, attributeValue);
}

// specified step of specified trial
void AsyncProfiler::set( unsigned int trialIndex, unsigned int stepIndex, unsigned int attributeIndex, unsigned int attributeValue)
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

::std::ostream& AsyncProfiler::writeLog( ::std::ostream& s ) const
{
  s << "<ASYNC_PROFILE name=\"Log\" >";
  s << std::endl;
  for (unsigned int t = 0; t < trials.size()-1; t++) // last trial always incomplete
  {
    trials[t].writeLog(s);
  }
  s << "</ASYNC_PROFILE>";
  s << std::endl;
  return s;
}

unsigned int AsyncProfiler::getTrialNum() const
{
  return currentTrialIndex;
}

unsigned int AsyncProfiler::getStepNum() const
{
  return trials[currentTrialIndex].getStepNum();
}

void AsyncProfiler::setStepName( const ::std::string& name)
{
  trials[currentTrialIndex].setStepName( name );
}

::std::ostream& AsyncProfiler::write( ::std::ostream& os ) const
{
  size_t numSteps = getNumSteps();
  Trial total(numSteps), average(numSteps), count(numSteps);
  // sum attributes
  for (unsigned int t = (trials.size()>1) ? 1 : 0; t < trials.size()-1; t++)
  {
    for (unsigned int s = 0; s < trials[t].size(); s++)
    {
      for (unsigned int a = 0; a < NUM_ATTRIBUTES; a++)
      {
        unsigned int value = get(t, s, a);
        if (value > 0)
        {
          unsigned int updatedValue = total.get(s, a) + value;
          total.set(s, a, updatedValue );
          count.set(s, a, count.get(s, a)+1 );
        }
      }
    }
  }
  // average attributes
  for (unsigned int s = 0; s < total.size(); s++)
  {
    for (unsigned int a = 0; a < NUM_ATTRIBUTES; a++)
    {
      unsigned int n = count.get(s, a);
      if (count.get(s, a) > 0)
      {
        unsigned int tot = total.get(s, a);
        average.set(s, a, tot / n);
      }
    }
  }

  os << "<ASYNC_PROFILE name=\"Average\" >";
  os << std::endl;
  total.writeLog(os);
  count.writeLog(os);
  average.writeLog(os);
  os << "</ASYNC_PROFILE>";
  os << std::endl;
  return os;
}