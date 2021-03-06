//
// Chai PCR - Software platform for Open qPCR and Chai's Real-Time PCR instruments.
// For more information visit http://www.chaibio.com
//
// Copyright 2016 Chai Biotechnologies Inc. <info@chaibio.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "updatemanager.h"
#include "constants.h"
#include "dbcontrol.h"
#include "upgrade.h"
#include "qpcrapplication.h"
#include "util.h"
#include "logger.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>

#include <sys/eventfd.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <Poco/File.h>
#include <Poco/URI.h>
#include <Poco/Util/Timer.h>
#include <Poco/Util/TimerTaskAdapter.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPClientSession.h>

#define DOWNLOAD_BUFFER_SIZE 8192

class CheckTimerTask : public Poco::Util::TimerTask
{
public:
    CheckTimerTask(std::function<void()> runFunction, Poco::Event &event): _runFunction(runFunction), _event(event) {}

protected:
    void run()
    {
        _runFunction();
        _event.set();
    }

private:
    std::function<void()> _runFunction;
    Poco::Event &_event;
};

UpdateManager::UpdateManager(std::shared_ptr<DBControl> dbControl):
    _updateEvent(false), _uploadEvent(false)
{
    _downloadEventFd = eventfd(0, EFD_NONBLOCK);

    if (_downloadEventFd == -1)
        throw std::system_error(errno, std::generic_category(), "Software update manager: unable to create event fd -");

    _dbControl = dbControl;
    _httpClient = new Poco::Net::HTTPClientSession(kUpdateHost);
    _updateTimer = new Poco::Util::Timer();
    _updateState = Unknown;
}

UpdateManager::~UpdateManager()
{
    stopChecking(false);
    stopDownload();

    delete _updateTimer;
    delete _httpClient;

    close(_downloadEventFd);
}

void UpdateManager::startChecking()
{
    stopChecking();

    _updateTimer->schedule(Poco::Util::TimerTask::Ptr(new CheckTimerTask(std::bind(&UpdateManager::checkUpdateCallback, this, true), _updateEvent)), 3 * 60 * 1000, kUpdateInterval);
}

void UpdateManager::stopChecking(bool wait)
{
    _updateTimer->cancel(wait);
}

bool UpdateManager::checkUpdate()
{
    if (_updateState == Available)
        return true;

    Poco::Util::TimerTask::Ptr task(new CheckTimerTask(std::bind(&UpdateManager::checkUpdateCallback, this, false), _updateEvent));

    _updateEvent.reset();
    _updateTimer->schedule(task, Poco::Timestamp(0));

    try
    {
        _updateEvent.wait(15 * 1000);
    }
    catch (const Poco::TimeoutException &/*ex*/)
    {
        task->cancel();

        return false;
    }

    return true;
}

bool UpdateManager::update()
{
    Poco::LogStream logStream(Logger::get());
    UpdateState state = Available;

    if (_updateState.compare_exchange_strong(state, Updating))
    {
        try
        {
            Poco::File dir(kUpdateFolder + "/scripts");
            if (dir.exists())
                dir.remove(true);

            if (!Util::watchProcess("tar xf " + kUpdateFilePath + " --directory " + kUpdateFolder + " scripts", _downloadEventFd,
                                    [&logStream](const char buffer[]){ logStream << "UpdateManager::update - tar: " << buffer << std::endl; }))
                return false; //This will happen only if the app is getting closed
        }
        catch (...)
        {
            _updateState = Unknown;

            throw std::runtime_error("Unknown error occurred during extracting an upgrade archive");
        }

        try
        {
            Poco::File file(kUpdateScriptOutputPath);
            if (file.exists())
                file.remove();

            Util::watchProcess(kUpdateScriptPath + ' ' + kUpdateScriptOutputPath, [&](const char buffer[]) { logStream << "UpdateManager::update - perform_upgrade: " << buffer << std::endl; });
        }
        catch (...)
        {
            _updateState = Unknown;

            std::string message;
            std::ifstream file(kUpdateScriptOutputPath);

            if (file.is_open())
            {
                std::getline(file, message);

                throw std::runtime_error(message);
            }
            else
                throw std::runtime_error("Unknown error occured during upgrade");
        }

        return true;
    }

    return false;
}

void UpdateManager::upload(std::istream &dataStream)
{
    std::unique_lock<std::recursive_mutex> lock(_downloadMutex);

    stopDownload();

    UpdateState state = _updateState.exchange(ManualDownloading);

    if (state != Updating && state != ManualDownloading)
    {
        try
        {
            _uploadEvent.reset();

            lock.unlock();

            if (checkMountPoint() && checkSdcard())
            {
                _dbControl->setUpgradeDownloaded(false);

                uint64_t i = 0;
                std::ofstream file(kUpdateFilePath.c_str(), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

                if (file.is_open())
                {
                    char buffer[DOWNLOAD_BUFFER_SIZE];

                    while (dataStream.good() && file.good())
                    {
                        std::fill(std::begin(buffer), std::end(buffer), 0);

                        dataStream.read(buffer, DOWNLOAD_BUFFER_SIZE);

                        if (dataStream.gcount() > 0)
                            file.write(buffer, dataStream.gcount());

                        read(_downloadEventFd, &i, sizeof(i));
                        if (i > 0)
                            break;
                    }

                    if (dataStream.eof())
                        _updateState = Available;
                    else
                        _updateState = Unknown;
                }
                else
                {
                    APP_LOGGER << "UpdateManager::upload - unable to open file " << kUpdateFilePath << ": " << std::strerror(errno) << std::endl;

                    _updateState = Unknown;
                }
            }
            else
                _updateState = Unknown;

            _uploadEvent.set();
        }
        catch (const std::exception &ex)
        {
            APP_LOGGER << "UpdateManager::upload - exception: " << ex.what() << std::endl;

            _updateState = Unknown;
        }
    }
    else if (state != ManualDownloading)
    {
        state = ManualDownloading;

        _updateState.compare_exchange_strong(state, Updating);
    }
}

void UpdateManager::stopDownload()
{
    std::lock_guard<std::recursive_mutex> lock(_downloadMutex);

    if (_downloadThread.joinable())
    {
        uint64_t i = 1;
        write(_downloadEventFd, &i, sizeof(i));

        _downloadThread.join();

        //Clear event fd
        read(_downloadEventFd, &i, sizeof(i));

        UpdateState state = Downloading;
        _updateState.compare_exchange_strong(state, Unknown); //In case the downloading thread has set the state
    }
    else if (_updateState == ManualDownloading)
    {
        uint64_t i = 1;
        write(_downloadEventFd, &i, sizeof(i));

        _uploadEvent.wait();

        //Clear event fd
        read(_downloadEventFd, &i, sizeof(i));

        UpdateState state = ManualDownloading;
        _updateState.compare_exchange_strong(state, Unknown); //In case the uploading thread has set the state
    }
}

void UpdateManager::checkUpdateCallback(bool checkHash)
{
    Upgrade upgrade;
    UpdateState state = _updateState;

    if (state == ManualDownloading || state == Updating)
        return;

    try
    {
        Poco::URI uri(kUpdatesUrl);
        uri.setQueryParameters({ {"v", "1"}, {"model_number", qpcrApp.settings().device.modelNumber}, {"software_version", qpcrApp.settings().configuration.version},
                                 {"software_platform", qpcrApp.settings().configuration.platform}, {"serial_number", qpcrApp.settings().device.serialNumber} });

        Poco::Net::HTTPRequest request("GET", uri.toString());
        Poco::Net::HTTPResponse response;

        _httpClient->sendRequest(request);

        boost::property_tree::ptree ptree;
        boost::property_tree::read_json(_httpClient->receiveResponse(response), ptree);

        upgrade.setVersion(ptree.get<std::string>("software_version"));
        upgrade.setChecksum(ptree.get<std::string>("software_checksum"));
        upgrade.setBriedDescription(ptree.get<std::string>("brief_description"));
        upgrade.setFullDescription(ptree.get<std::string>("full_description"));
        upgrade.setReleaseDate(Util::parseIsoTime(ptree.get<std::string>("release_date")));
        upgrade.setPassword(ptree.get<std::string>("password"));
        upgrade.setImageUrl(ptree.get<std::string>("image_rsync_url"));

        bool isCurrent = false;
        bool downloaded = false;

        _dbControl->updateUpgrade(upgrade, isCurrent, downloaded);

        if (isCurrent)
        {
            if (downloaded)
            {
                std::string sum;

                if (!checkHash || (Util::getFileChecksum(kUpdateFilePath, _downloadEventFd, sum) && sum == upgrade.checksum()))
                {
                    if (qpcrApp.settings().configuration.version != upgrade.version())
                        _updateState.compare_exchange_strong(state, Available);
                    else
                        _updateState.compare_exchange_strong(state, Unavailable);

                    return;
                }
                else if (_updateState.compare_exchange_strong(state, Unknown))
                {
                    state = Unknown;

                    _dbControl->setUpgradeDownloaded(false);
                }
            }
            else if (state == Downloading)
                return;
        }
    }
    catch (const std::exception &ex)
    {
        APP_LOGGER << "UpdateManager::checkUpdateCallback - " << ex.what() << std::endl;

        _httpClient->reset();

        if (state != Downloading)
            _updateState.compare_exchange_strong(state, Unknown);

        return;
    }

    std::lock_guard<std::recursive_mutex> lock(_downloadMutex);

    state = _updateState;
    if (state == ManualDownloading || state == Updating)
        return;

    if (qpcrApp.settings().configuration.version != upgrade.version())
    {
        stopDownload();

        if (_updateState.compare_exchange_strong(state, Downloading))
            _downloadThread = std::thread(static_cast<void(UpdateManager::*)(Upgrade)>(&UpdateManager::downlaod), this, upgrade);
    }
    else
        _updateState.compare_exchange_strong(state, Unavailable);
}

void UpdateManager::downlaod(Upgrade upgrade)
{
    _updateState = Downloading;

    try
    {
        std::string sum;

        if (checkMountPoint() && checkSdcard() && downlaod(upgrade.imageUrl(), upgrade.password()) && Util::getFileChecksum(kUpdateFilePath, _downloadEventFd, sum) && sum == upgrade.checksum())
        {
            _updateState = Available;
            _dbControl->setUpgradeDownloaded(true);
        }
        else
            _updateState = Unknown;
    }
    catch (const std::exception &ex)
    {
        APP_LOGGER << "UpdateManager::downlaod - " << ex.what() << std::endl;

        _updateState = Unknown;
    }
}

bool UpdateManager::downlaod(const std::string &imageUrl, const std::string &apiPassword)
{
    Poco::LogStream logStream(Logger::get());

    std::stringstream stream;
    stream << "sshpass -p \'" << apiPassword << "\' rsync -a --checksum --no-whole-file --inplace " << imageUrl << " " << kUpdateFilePath;

    return Util::watchProcess(stream.str(), _downloadEventFd, [&logStream](const char buffer[]){ logStream << "UpdateManager::downlaod - rsync: " << buffer << std::endl; });
}

bool UpdateManager::checkMountPoint()
{
    std::string output;

    if (Util::watchProcess("cat /etc/mtab | grep " + kUpdateMountPoint, _downloadEventFd, [&output](const char buffer[]){ output = buffer; }))
        return output.find(kUpdateMountPoint) != std::string::npos;
    else
        return false;
}

bool UpdateManager::checkSdcard()
{
    Poco::LogStream logStream(Logger::get());

    return Util::watchProcess(kCheckSdcardPath, _downloadEventFd, [&logStream](const char buffer[]){ logStream << "UpdateManager::checkSdcard - check_sdcard: " << buffer << std::endl; });
}
