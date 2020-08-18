/*
* Copyright 2016 Derek W. Brown
*/

/** @file datadogagent.cpp
 Defines the entry point for the console application.
*/
#include "stdafx.h"


SERVICE_STATUS_HANDLE gServiceStatusHandle; //!< Global handle for Windows Service
DWORD gServiceCurrentStatus;				//!< Global containing current service status

HANDLE gConsoleKillEventHandle = NULL;		//!< Global handle for stopping console service
HANDLE gServiceKillHandle = NULL;			//!< Global handle for stopping when running as service
const wchar_t* strServiceName = L"Simple Service";
const wchar_t* strDisplayName = L"Simple Do Nothing Service";
const wchar_t* strServiceDescription = L"Service that does nothing";

bool g_isConsole = false;	//!< global indicating service mode or console mode

void __cdecl
signalHandler(int sig)
{
    /**
    * only used when running as console.  Sets the termination event if
    * the user hits ctrl-c
    */
    printf("Signal %d caught: terminating threads ...\n", sig);
    if (gConsoleKillEventHandle)
    {
        SetEvent(gConsoleKillEventHandle);
    }
}

void StopService()
{
    SetEvent(gServiceKillHandle);
}

BOOL
UpdateServiceStatus(DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwServiceSpecificExitCode,
    DWORD dwCheckPoint,
    DWORD dwWaitHint)
{
    /**
    * Updates the windows service control manager with the current status
    * of the service.  Important to make updates regularly and in a timely
    * manner, otherwise the Windows SCM will become quite confused about the
    * state of the service
    */
    BOOL success;
    SERVICE_STATUS nServiceStatus;

    nServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    nServiceStatus.dwCurrentState = dwCurrentState;
    if (dwCurrentState == SERVICE_START_PENDING)
    {
        nServiceStatus.dwControlsAccepted = 0;
    }
    else
    {
        nServiceStatus.dwControlsAccepted =
            SERVICE_ACCEPT_SHUTDOWN |
            SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_PARAMCHANGE;
    }
    nServiceStatus.dwControlsAccepted |= SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_SESSIONCHANGE;

    if (dwServiceSpecificExitCode == 0)
    {
        nServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    }
    else
    {
        nServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    }

    nServiceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
    nServiceStatus.dwCheckPoint = dwCheckPoint;
    nServiceStatus.dwWaitHint = dwWaitHint;

    success = SetServiceStatus(gServiceStatusHandle, &nServiceStatus);
    if (!success)
    {
        // @todo... log and complete 
    }

    return success;
}
/**
* Callback from Windows to notify us (when running as a service) when various
* Windows events have occurred.
*/
DWORD
ServiceCtrlHandler(DWORD nControlCode, DWORD dwEventType, LPVOID /* lpEventData */, LPVOID /* lpContext */)
{
    DWORD retval = NO_ERROR;


    switch (nControlCode)
    {
    case SERVICE_CONTROL_SESSIONCHANGE:
        // Being notified of session change events (i.e. different users logging in,
        // log in via RDP, etc.  Just note it for now;
        break;

    case SERVICE_CONTROL_POWEREVENT:
        switch (dwEventType)
        {
        case PBT_APMRESUMEAUTOMATIC:
            break;

        case PBT_APMSUSPEND:
            break;
        }
        break;

        // system shutting down
    case SERVICE_CONTROL_SHUTDOWN:
    {

        gServiceCurrentStatus = SERVICE_STOP_PENDING;
        UpdateServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0, 1, 3000);
        StopService();
    }
    break;

    // stop the service
    case SERVICE_CONTROL_STOP:
    {

        gServiceCurrentStatus = SERVICE_STOP_PENDING;
        UpdateServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0, 1, 3000);
        StopService();
    }
    break;

    // this allows the service to reload configuration items
    case SERVICE_CONTROL_PARAMCHANGE:
    {
    }
    break;

    default:
        break;
    }

    UpdateServiceStatus(gServiceCurrentStatus, NO_ERROR, 0, 0, 0);
    return retval;
}
/**
* called by Windows serviceControlManager when process is started.  This
* is the equivalent of main().  This function blocks until service is stopped.
* returning from this function causes service to end.
*/
void WINAPI
ServiceMain(DWORD, LPTSTR *)
{

    BOOL success = FALSE;
    gServiceStatusHandle = RegisterServiceCtrlHandlerEx(strServiceName,
        (LPHANDLER_FUNCTION_EX)ServiceCtrlHandler, NULL);
    if (!gServiceStatusHandle)
    {
        return;
    }

    success = UpdateServiceStatus(SERVICE_START_PENDING, NO_ERROR, 0, 1, 3000);
    if (!success)
    {
        return;
    }




    gServiceCurrentStatus = SERVICE_RUNNING;
    success = UpdateServiceStatus(SERVICE_RUNNING, NO_ERROR, 0, 0, 0);
    if (!success)
    {
        return;
    }
    gServiceKillHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (gServiceKillHandle == NULL)
    {
        UpdateServiceStatus(SERVICE_STOPPED, NO_ERROR, 0, 0, 0);
        return;
    }
    WSAData wsadata;
    WSAStartup(0x0202, &wsadata);

    WaitForSingleObject(gServiceKillHandle, INFINITE);
    WSACleanup();
    UpdateServiceStatus(SERVICE_STOPPED, NO_ERROR, 0, 0, 0);




}

/**
 * called by the service control manager to start the service.
 *
 * OR, can be called on the command line (with the -console argument).  This
 * facility allows for easier development/debugging.
 */
int _tmain(int argc, _TCHAR* argv[])
{
    /**
    * main entry point.  Has two modes
    *
    * When running as a service, configures the service control manager with
    * the ServiceMain callback (above).
    *
    * When running in console mode (for debugging) allows for passing of specific
    * parameters to modify behavior, and make it easier to run in the debugger
    */
    BOOL success;
    SERVICE_TABLE_ENTRY servicetable[] =
    {
        { (LPTSTR)(strServiceName), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };
    TCHAR szPath[_MAX_PATH];

    // set the current folder to the application folder
    if (GetModuleFileName(NULL, szPath, MAX_PATH))
    {
        TCHAR * ptr;

        ptr = _tcsrchr(szPath, _T('\\'));
        if (ptr)
        {
            *ptr = '\0';
            SetCurrentDirectory(szPath);
            // put it back 
            *ptr = _T('\\');
        }
    }
    // check for option
    if (argc >= 2)
    {
        for (int i = 1; i < argc; i++)
        {
            if (_tcsicmp(TEXT("-console"), argv[i]) == 0)
            {
                g_isConsole = true;
            }
        }
    }

    if (g_isConsole)
    {
        /**
         * if running as console, start the object here, and wait to
         * be told to stop (via ctl-c).  This makes it much easier
         * to do development/debugging without the overhead of the
         * service control manager
         */
        gConsoleKillEventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
        // capture appropriate signals
        signal(SIGABRT, &signalHandler);
        signal(SIGTERM, &signalHandler);
        signal(SIGINT, &signalHandler);
        WSAData wsadata;
        WSAStartup(0x0202, &wsadata);


        WaitForSingleObject(gConsoleKillEventHandle, INFINITE);
        WSACleanup();
        return 0;
    }

    success = StartServiceCtrlDispatcher(servicetable);
    if (!success)
    {

    }

    return 0;
}


