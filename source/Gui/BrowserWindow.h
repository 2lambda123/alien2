#pragma once

#include "EngineInterface/Definitions.h"

#include "AlienWindow.h"
#include "RemoteSimulationData.h"
#include "Definitions.h"

class _BrowserWindow : public _AlienWindow
{
public:
    _BrowserWindow(SimulationController const& simController);
    ~_BrowserWindow();

private:
    void processIntern() override;
    void processActivated() override;

    std::string _server;
    std::string _filter;
    std::vector<RemoteSimulationData> _remoteSimulationDatas;

    std::string _test;

    SimulationController _simController;
};
