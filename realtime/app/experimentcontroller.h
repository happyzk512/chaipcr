#ifndef EXPERIMENTCONTROLLER_H
#define EXPERIMENTCONTROLLER_H

#include <instance.h>

class DBControl;
class Experiment;

namespace Poco { class Timer; }

class ExperimentController : public Instance<ExperimentController>
{
public:
    enum MachineState
    {
        Idle,
        LidHeating,
        Running,
        Complete
    };

    ExperimentController();
    ~ExperimentController();

    inline MachineState machineState() const { return _machineState; }

    bool start(int experimentId);
    void stop();

    void stepBegun();

private:
    void run();
    void complete();

    void holdStepCallback(Poco::Timer &timer);

private:
    std::atomic<MachineState> _machineState;

    DBControl *_dbControl;
    Experiment *_experiment;

    Poco::Timer *_holdStepTimer;
};

#endif // EXPERIMENTCONTROLLER_H