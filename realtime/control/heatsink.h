#ifndef _HEATSINK_H_
#define _HEATSINK_H_

#include "fan.h"
#include "thermistor.h"

// Class HeatSink
class HeatSink : public IControl
{
public:
    HeatSink();
	~HeatSink();
	
	//accessors
	inline double temperature() { return thermistor_.temperature(); }
	
    void process();
	
private:
	Fan fan_;
	Thermistor thermistor_;
};

#endif
