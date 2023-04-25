#include "GettingStartedWindow.h"

#include <imgui.h>
#include <Fonts/IconsFontAwesome5.h>

#include "GlobalSettings.h"
#include "StyleRepository.h"
#include "AlienImGui.h"

#ifdef _WIN32
#include <windows.h>
#endif

_GettingStartedWindow::_GettingStartedWindow()
    : _AlienWindow("Getting started", "windows.getting started", true)
{
    _showAfterStartup = _on;
}


_GettingStartedWindow::~_GettingStartedWindow()
{
    _on = _showAfterStartup;
}

void _GettingStartedWindow::processIntern()
{
    ImGui::PushFont(StyleRepository::getInstance().getMediumFont());
    ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)Const::HeadlineColor);
    ImGui::Text("What is (A)rtificial (LI)fe (EN)vironment?");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    AlienImGui::Separator();

    if (ImGui::BeginChild("##", ImVec2(0, ImGui::GetContentRegionAvail().y - 50), false)) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x);
        ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)Const::HeadlineColor);
        ImGui::Text("Introduction");
        ImGui::PopStyleColor();

        ImGui::Text("ALIEN is an artificial life and physics simulation tool based on a specialized 2D particle engine written in CUDA for soft bodies and fluids.");
        ImGui::Text("Each particle can be equipped with higher-level functions including sensors, muscles, neurons, constructors, etc. that allow to "
                    "mimic certain functionalities of (a group of) biological cells or of robotic components. Multi-cellular organisms are simulated as networks of "
                    "particles that exchange energy and information over their connections. The engine encompasses a genetic system capable of encoding the "
                    "blueprints of organisms in genomes which are stored in individual particles. This approach allows to simulate entire ecosystems inhabited "
                    "by different populations where every object (regardless of whether it is a plant, a herbivore or a pure physical structure) is composed of "
                    "interacting particles with specific functions.");

        AlienImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)Const::HeadlineColor);
        ImGui::Text("First steps");
        ImGui::PopStyleColor();

        ImGui::Text("The easiest way to get to know the ALIEN simulator is to load and run an existing simulation file.");
        ImGui::Text(ICON_FA_CARET_RIGHT);
        ImGui::SameLine();
        ImGui::Text("You find various demos in ./examples/* demonstrating capabilities of the "
                    "engine ranging from physics examples, self-deploying structures, replicators to small "
                    "ecosystems. To this end, please invoke Simulation " ICON_FA_ARROW_RIGHT
                    " Open in the menu and select a file.");
        ImGui::Text(ICON_FA_CARET_RIGHT);
        ImGui::SameLine();
        ImGui::Text("Simulations from other users can be downloaded by using the in-game browser that is connected to a server. "
                    "For this purpose, please click on 'Browser' in the 'Network' menu. In order to upload own simulations to the server and rate other "
                    "simulations, you need to create a new user, which can be accomplished in the login dialog.");
            
        ImGui::Text("For the beginning, however, you can use the evolution example already loaded. At the beginning it is recommended to get familiar with "
                    "the windows for temporal and spatial controls. The handling should be intuitive and requires no deeper knowledge.");
        ImGui::Text(ICON_FA_CARET_RIGHT);
        ImGui::SameLine();
        ImGui::Text("In the temporal control window, a simulation can be started or paused. The execution speed "
                    "may be regulated if necessary. In addition, it is possible to calculate and revert single time steps as "
                    "well as to make snapshots of a simulation to which one can return at any time without having "
                    "to reload the simulation from a file.");
        ImGui::Text(ICON_FA_CARET_RIGHT);
        ImGui::SameLine();
        ImGui::Text("The spatial control window combines zoom information and settings on the one hand, and "
                    "scaling functions on the other hand. A quite useful feature in the dialog for "
                    "scaling/resizing is the option 'Scale content'. If activated, periodic spatial copies of "
                    "the original world can be made.");
        ImGui::Text("There are basically two modes of how the user can operate in the view where the simulation is "
                    "shown: a navigation mode and an edit mode. You can switch between these two modes by invoking "
                    "the edit button at the bottom left of the screen or in the menu via Editor " ICON_FA_ARROW_RIGHT " Activate.");
        ImGui::Text(ICON_FA_CARET_RIGHT);
        ImGui::SameLine();
        ImGui::Text("The navigation mode is enabled by default and allows you to zoom in (holding the left mouse "
                    "button) and out (holding the right mouse button) continuously. By holding the middle mouse "
                    "button and moving the mouse, you can pan the visualized section of the world.");
        ImGui::Text(ICON_FA_CARET_RIGHT);
        ImGui::SameLine();
        ImGui::Text("In the edit mode, it is possible to apply forces to bodies in a running simulation by holding and moving the right mouse button."
                    "With the left mouse button you can drag and drop objects. Please try this out. It can make a lot of fun! The editing mode also allows you "
                    "to activate various editing windows (Pattern editor, Creator, Multiplier, etc.) whose possibilities can be explored over time.");

        ImGui::Text("To be able to experiment with existing simulation files, it is important to know and change the "
                    "simulation parameters. This can be accomplished in the window 'Simulation parameters'. For example, "
                    "the radiation intensity can be increased or the friction can be adjusted. Explanations to the "
                    "individual parameters can be found in the tooltip next to them.");
        ImGui::Text("ALIEN also uses a simple color system consisting of 7 different colors for the cells. It is possible to restrict simulation parameters to "
                    "cells with a certain color. This allows to create special conditions for different populations populating a world together. For example, "
                    "plant-like organisms may have a higher absorption rate for radiation particles, so they can get their energy from that.");

        AlienImGui::Separator();

        ImGui::Text(
            "IMPORTANT: On older graphics cards or when using a high resolution (e.g. 4K), it is recommended to reduce the rendered frames per second, "
            "as this significantly increases the simulation speed (time steps per second). This adjustment can be made in the display settings.");

        AlienImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)Const::HeadlineColor);
        ImGui::Text("Further steps");
        ImGui::PopStyleColor();

        //ImGui::Text("There is a lot to explore. ALIEN features an extensive graph and particle editor in order to build custom worlds with desired "
        //            "environmental structures and machines. A documentation with tutorial-like introductions to various topics can be found at");

        //ImGui::Dummy(ImVec2(0.0f, 20.0f));

        //ImGui::PushFont(StyleRepository::getInstance().getMonospaceMediumFont());
        //auto windowWidth = ImGui::GetWindowSize().x;
        //auto weblink = "https://alien-project.gitbook.io/docs";
        //auto textWidth = ImGui::CalcTextSize(weblink).x;
        //ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        //if(AlienImGui::Button(weblink)) {
        //    openWeblink(weblink);
        //}
        //ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        ImGui::PopTextWrapPos();
    }
    ImGui::EndChild();

    AlienImGui::Separator();
    AlienImGui::ToggleButton(AlienImGui::ToggleButtonParameters().name("Show after startup"), _showAfterStartup);
}

void _GettingStartedWindow::openWeblink(std::string const& link)
{
#ifdef _WIN32
    ShellExecute(NULL, "open", link.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
}
 