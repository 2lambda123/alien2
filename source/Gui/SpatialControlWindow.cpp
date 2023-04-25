#include "SpatialControlWindow.h"

#include <imgui.h>

#include "Fonts/IconsFontAwesome5.h"

#include "Base/StringHelper.h"
#include "Base/Resources.h"
#include "EngineInterface/DescriptionHelper.h"
#include "EngineInterface/SimulationController.h"
#include "StyleRepository.h"
#include "Viewport.h"
#include "GlobalSettings.h"
#include "AlienImGui.h"

_SpatialControlWindow::_SpatialControlWindow(SimulationController const& simController, Viewport const& viewport)
    : _AlienWindow("Spatial control", "windows.spatial control", true)
    , _simController(simController)
    , _viewport(viewport)
{
}

void _SpatialControlWindow::processIntern()
{
    processZoomInButton();
    ImGui::SameLine();
    processZoomOutButton();
    ImGui::SameLine();
    AlienImGui::ToolbarSeparator();
    ImGui::SameLine();
    processResizeButton();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::BeginChild("##", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {

        ImGui::Text("World size");
        ImGui::PushFont(StyleRepository::getInstance().getLargeFont());
        ImGui::PushStyleColor(ImGuiCol_Text, Const::TextDecentColor);
        auto worldSize = _simController->getWorldSize();
        ImGui::TextUnformatted(
            (StringHelper::format(worldSize.x) + " x " + StringHelper::format(worldSize.y)).c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::Text("Zoom factor");
        ImGui::PushFont(StyleRepository::getInstance().getLargeFont());
        ImGui::PushStyleColor(ImGuiCol_Text, Const::TextDecentColor);
        ImGui::TextUnformatted(StringHelper::format(_viewport->getZoomFactor(), 1).c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::Text("Center position");
        ImGui::PushFont(StyleRepository::getInstance().getLargeFont());
        ImGui::PushStyleColor(ImGuiCol_Text, Const::TextDecentColor);
        auto centerPos = _viewport->getCenterInWorldPos();
        ImGui::TextUnformatted(
            (StringHelper::format(centerPos.x, 1) + ", " + StringHelper::format(centerPos.y, 1)).c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        AlienImGui::Separator();
        AlienImGui::ToggleButton(AlienImGui::ToggleButtonParameters().name("Autotracking on selection"), _centerSelection);
        ImGui::Spacing();
        ImGui::Spacing();
        float sensitivity = _viewport->getZoomSensitivity();
        if (AlienImGui::SliderFloat(AlienImGui::SliderFloatParameters().name("Zoom sensitivity").min(1.0f).max(1.05f).textWidth(130).format(""), &sensitivity)) {
            _viewport->setZoomSensitivity(sensitivity);
        }
    }
    ImGui::EndChild();

    processResizeDialog();
}

void _SpatialControlWindow::processBackground()
{
    processCenterOnSelection();
}

void _SpatialControlWindow::processZoomInButton()
{
    if (AlienImGui::ToolbarButton(ICON_FA_SEARCH_PLUS)) {
        _viewport->setZoomFactor(_viewport->getZoomFactor() * 2);
    }
}

void _SpatialControlWindow::processZoomOutButton()
{
    if (AlienImGui::ToolbarButton(ICON_FA_SEARCH_MINUS)) {
        _viewport->setZoomFactor(_viewport->getZoomFactor() / 2);
    }
}

void _SpatialControlWindow::processResizeButton()
{
    if (AlienImGui::ToolbarButton(ICON_FA_EXPAND_ARROWS_ALT)) {
        _showResizeDialog = true;
        auto worldSize = _simController->getWorldSize();
        _width = worldSize.x;
        _height = worldSize.y;
    }
}

void _SpatialControlWindow::processResizeDialog()
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    if (_showResizeDialog) {
        ImGui::OpenPopup("Resize world");
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Resize world", NULL, 0)) {
            if (ImGui::BeginTable("##", 2, ImGuiTableFlags_SizingStretchProp)) {

                //width
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputInt("##width", &_width);
                ImGui::PopItemWidth();

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Width");

                //height
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputInt("##height", &_height);
                ImGui::PopItemWidth();

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Height");

                ImGui::EndTable();
            }
            AlienImGui::ToggleButton(AlienImGui::ToggleButtonParameters().name("Scale content"), _scaleContent);

            AlienImGui::Separator();

            if (AlienImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
                _showResizeDialog = false;
                onResizing();
            }
            ImGui::SetItemDefaultFocus();

            ImGui::SameLine();
            if (AlienImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
                _showResizeDialog = false;
            }

            ImGui::EndPopup();
            _width = std::max(1, _width);
            _height = std::max(1, _height);
        }
    }
}

void _SpatialControlWindow::processCenterOnSelection()
{
    if (_centerSelection && _simController->isSimulationRunning()) {
        auto shallowData = _simController->getSelectionShallowData();
        if (shallowData.numCells > 0 || shallowData.numParticles > 0) {
            _viewport->setCenterInWorldPos({shallowData.centerPosX, shallowData.centerPosY});
        }
    }
}

void _SpatialControlWindow::onResizing()
{
    auto timestep = _simController->getCurrentTimestep();
    auto generalSettings = _simController->getGeneralSettings();
    auto parameters = _simController->getSimulationParameters();
    auto content = _simController->getClusteredSimulationData();

    _simController->closeSimulation();

    IntVector2D origWorldSize{generalSettings.worldSizeX, generalSettings.worldSizeY};
    generalSettings.worldSizeX = _width;
    generalSettings.worldSizeY = _height;
    
    _simController->newSimulation(timestep, generalSettings, parameters);

    DescriptionHelper::correctConnections(content, {_width, _height});
    if (_scaleContent) {
        DescriptionHelper::duplicate(content, origWorldSize, {_width, _height});
    }
    _simController->setClusteredSimulationData(content);
}
