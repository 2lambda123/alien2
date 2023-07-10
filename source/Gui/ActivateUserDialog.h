#pragma once

#include "Definitions.h"

class _ActivateUserDialog
{
public:
    _ActivateUserDialog(BrowserWindow const& browserWindow, NetworkController const& networkController);
    ~_ActivateUserDialog();

    void registerCyclicReferences(CreateUserDialogWeakPtr const& createUserDialog);

    void process();

    void show(std::string const& userName, std::string const& password);

private:
    void onActivateUser();

    BrowserWindow _browserWindow;
    NetworkController _networkController;
    CreateUserDialogWeakPtr _createUserDialog;

    bool _show = false;
    std::string _userName;
    std::string _password;
    std::string _confirmationCode;
};