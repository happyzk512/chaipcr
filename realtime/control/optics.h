/* * Chai PCR - Software platform for Open qPCR and Chai's Real-Time PCR instruments.
 * For more information visit http://www.chaibio.com
 *
 * Copyright 2016 Chai Biotechnologies Inc. <info@chaibio.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _OPTICS_H_
#define _OPTICS_H_

#include "icontrol.h"
#include "adcconsumer.h"

#include "gpio.h"
#include "mux.h"

#include <map>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

class LEDController;

namespace Poco { class Timer; }

////////////////////////////////////////////////////////////////////////////////
// Class Optics
class Optics : public IControl, public ADCConsumer
{
public:
    struct FluorescenceData
    {
        FluorescenceData(unsigned int value, unsigned int wellId, std::size_t channel): value(value), wellId(wellId), channel(channel) {}

        unsigned long value;
        unsigned int wellId;
        std::size_t channel;
    };

    struct MeltCurveData
    {
        MeltCurveData(unsigned int fluorescenceValue, double temperature, int wellId, std::size_t channel): fluorescenceValue(fluorescenceValue), temperature(temperature), wellId(wellId), channel(channel) {}

        unsigned int fluorescenceValue;
        double temperature;
        int wellId;
        std::size_t channel;
    };

    Optics(unsigned int lidSensePin, std::shared_ptr<LEDController> ledController, MUX &&photoDiodeMux);
    ~Optics();

    void process();

    void setADCValue(unsigned int adcValue, std::size_t channel);
    inline const std::map<std::size_t, std::atomic<unsigned long>>& lastAdcValues() const noexcept { return _lastAdcValues; }
	
	//accessors
    inline bool lidOpen() const noexcept { return _lidOpen; }

    inline bool collectData() const noexcept { return _collectData; }
    inline bool isMeltCurveCollection() const noexcept { return _meltCurveCollection; }
    void setCollectData(bool state, bool isMeltCurve = false);

    inline unsigned wellNumber() const noexcept { return _wellNumber; } //Yes, it's used in multithreading. Yes, it isn't thread safe here. It's just for testing

    inline std::shared_ptr<LEDController> getLedController() noexcept { return _ledController; }
    inline MUX& getPhotodiodeMux() { return _photodiodeMux; }

    std::vector<FluorescenceData> getFluorescenceData();
    std::vector<MeltCurveData> getMeltCurveData(bool stopDataCollect = true);

private:
    void toggleCollectData();
    void collectDataCallback(Poco::Timer &timer);
	
private:
    std::shared_ptr<LEDController> _ledController;

    std::atomic<bool> _lidOpen;
    GPIO _lidSensePin;

    std::pair<unsigned long, std::size_t> _adcValue;
    std::mutex _adcMutex;
    std::condition_variable _adcCondition;

    std::atomic<bool> _collectData;
    Poco::Timer *_collectDataTimer;
    mutable std::recursive_mutex _collectDataMutex;

    unsigned int _wellNumber;
    std::map<unsigned int, std::map<std::size_t, std::vector<unsigned long>>> _fluorescenceData;

    std::atomic<bool> _meltCurveCollection;
    std::vector<MeltCurveData> _meltCurveData;
    std::mutex _meltCurveDataMutex;

    MUX _photodiodeMux;

    //Hardcode for testing
    std::map<std::size_t, std::atomic<unsigned long>> _lastAdcValues; //Not thread safe
};

#endif
