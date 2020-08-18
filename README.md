# codesamples

This repository contains sample code for simulating DataDog agents.


## emptysvc

The emptysvc project contains two targets, _emptysvc_ and _prochandle_

### emptysvc

Emptysvc is a Windows service shell.  It starts and answers requests from the Windows Service Control Manager.  It contains no active code.

### prochandle

Prochandle opens a handle for a given process (specified by process id).  It keeps the handle open, and waits on the handle to become  signalled (Windows will signal the handle when the process is no longer valid).