#pragma once

#include "EngineInterface/Definitions.h"
#include "Definitions.h"

class MessageDialog
{
public:
    static MessageDialog& getInstance();

    void process();

    void show(std::string const& title, std::string const& message);

private:
    bool _sizeInitialized = false;
    bool _show = false;
    std::string _title;
    std::string _message;
};

inline void printMessage(std::string const& title, std::string const& message)
{
    MessageDialog::getInstance().show(title, message);
}
