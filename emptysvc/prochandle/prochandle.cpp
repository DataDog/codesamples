// prochandle.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <iostream>

int main(int argc, char**argv)
{
    if (argc != 2) {
        printf("Usage: prchandle <pid>\n");
        return -1;
    }
    int pid = atoi(argv[1]);
    if (0 == pid) {
        printf("Invalid PID %s %d\n", argv[1], pid);
        return -2;
    }
    HANDLE h = OpenProcess(0x1010 | SYNCHRONIZE, false, pid);
    if (NULL == h) {
        printf("Failed to open process with VM_READ %d, trying w/o\n", GetLastError());
        h = OpenProcess(0x1000 | SYNCHRONIZE, false, pid);
        if (NULL == h) {
            printf("still failed to open process, giving up %d\n", GetLastError());
            return -3;
        }
    }
    printf("Process opened. waiting for process...\n");
    do
    {
        int nRc = WaitForSingleObject(h, INFINITE);
        if (WAIT_OBJECT_0 == nRc) {
            printf("process exited\n");
            break;
        }
        printf("Wait returned early, continuing (%d)...\n", nRc);
        SleepEx(2000, false);
    } while (1);
    CloseHandle(h);
    printf("Done\n");

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
