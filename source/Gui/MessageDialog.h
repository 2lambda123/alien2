#pragma once

#include "EngineInterface/Definitions.h"
#include "Definitions.h"

class MessageDialog
{
public:
    static MessageDialog& getInstance();

    void process();

    void information(std::string const& title, std::string const& message);
    void yesNo(std::string const& title, std::string const& message, std::function<void()> const& yesFunction);

private:
    void processInformation();
    void processYesNo();

    bool _sizeInitialized = false;
    bool _show = false;

    enum class DialogType
    {
        Information, YesNo
    };
    DialogType _dialogType = DialogType::Information;
    std::string _title;
    std::string _message;
    std::function<void()> _execFunction;
};

inline void showMessage(std::string const& title, std::string const& message)
{
    MessageDialog::getInstance().information(title, message);
}
