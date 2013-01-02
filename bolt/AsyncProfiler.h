#pragma once
/******************************************************************************
 * Asynchronous Profiler 
 *****************************************************************************/
#include <vector>
#include <Windows.h>


class AsyncProfiler
{
private:
  LARGE_INTEGER constructionTimeStamp;
  double timerFrequency;
  unsigned int getTime();

public:

  static enum attributeTypes {
    /*native*/  id, startTime, stopTime, memory, device, flops,
    /*derived*/ time, bandwidth, flops_s,
    /*total*/   NUM_ATTRIBUTES};
  static char *attributeNames[];// = {"ID", "StartTime", "StopTime", "Memory", "Device", "Flops"};

  /******************************************************************************
   * Class Step 
   *****************************************************************************/
  class Step
  {
  private:
    unsigned int serial;
    ::std::string stepName;
    unsigned int attributeValues[NUM_ATTRIBUTES];

  public:
    /******************************************************************************
     * Constructors
     *****************************************************************************/
    Step( );
    ~Step(void);

    /******************************************************************************
     * Member Functions
     *****************************************************************************/
    void setSerial( unsigned int s );
    void set( unsigned int index, unsigned int value);
    unsigned int get( unsigned int index ) const;
    void setName( const ::std::string& name );
    void computeDerived();
    ::std::ostream& writeLog( ::std::ostream& s ) const;
  
  }; // class Step


  /******************************************************************************
   * Class Trial
   *****************************************************************************/
  class Trial
  {
  private:
    unsigned int serial;
    std::vector<Step> steps;
    unsigned int currentStepIndex;

  public:
    /******************************************************************************
     * Constructors
     *****************************************************************************/
    Trial(void);
    Trial( size_t n );
    ~Trial(void);

    /******************************************************************************
     * Member Functions
     *****************************************************************************/
    void setSerial( unsigned int s );
    unsigned int size() const;
    unsigned int get( unsigned int attributeIndex) const;
    unsigned int get( unsigned int stepIndex, unsigned int attributeIndex) const;
    void set( unsigned int attributeIndex, unsigned int attributeValue);
    void set( unsigned int stepIndex, unsigned int attributeIndex, unsigned int attributeValue);
    void startStep();
    unsigned int nextStep();
    unsigned int getStepNum() const;
    ::std::ostream& writeLog( ::std::ostream& s ) const;
    void setStepName( const ::std::string& name);
  }; // class Trial


/******************************************************************************
 * Resume Class AsyncProfiler
 *****************************************************************************/
private:
  unsigned int currentTrialIndex;
  std::vector<Trial> trials;

public:
  /******************************************************************************
   * Constructors
   *****************************************************************************/
  AsyncProfiler(void);
  ~AsyncProfiler(void);

  /******************************************************************************
   * Member Functions
   *****************************************************************************/
  void startTrial();
  void stopTrial();
  void nextTrial();

  void nextStep();

  void set( unsigned int attributeIndex, unsigned int attributeValue);

  void set( unsigned int stepIndex, unsigned int attributeIndex, unsigned int attributeValue);
  
  void set( unsigned int trialIndex, unsigned int stepIndex, unsigned int attributeIndex, unsigned int attributeValue);

  unsigned int get( unsigned int attributeIndex) const;

  unsigned int get( unsigned int stepIndex, unsigned int attributeIndex) const;
  
  unsigned int get( unsigned int trialIndex, unsigned int stepIndex, unsigned int attributeIndex) const;
  
  void setStepName( const ::std::string& name);

  unsigned int getNumTrials() const;

  unsigned int getNumSteps() const;

  unsigned int getTrialNum() const;

  unsigned int getStepNum() const;

  ::std::ostream& writeLog( ::std::ostream& s ) const;
  ::std::ostream& write( ::std::ostream& s ) const;

}; // class AsyncProfiler

