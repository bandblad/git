#include <iostream>
#include <string>
#include <windows.h>

#define SEM_CNT 3
#define SERVE_BUF_LEN 512
#define PIPE_NAME "\\\\.\\pipe\\foo"

// Global mutex & semaphore handles
HANDLE
	ghSemaphore = NULL,
	ghMutexSyncStdout = NULL,
	ghMutexSyncProcCreation = NULL;

// Thread task
DWORD WINAPI ThreadProc(LPVOID);

// Combination of std::cout & mutex object
void stdout_mutex(HANDLE&, const std::string&);

int main(void)
{
	try {
		// Prompt user action
		std::cout << "Put 'main-proc-client.exe' in the current directory.\
		\nOtherwise programm will crash.\
		\nType number of processes to start (>0): ";

		// Read number of processess
		int numof_procs;
		std::cin >> numof_procs;

		// Catch errors in user input
		if (!std::cin.good())
			throw std::runtime_error("ERROR: Invalid data in stream!");

		if (numof_procs < 1)
			throw std::invalid_argument("ERROR: Invalid value!");

		// Create & validate semaphore handle
		ghSemaphore = CreateSemaphore(NULL, SEM_CNT, SEM_CNT, NULL);
		if (!ghSemaphore)
			throw std::runtime_error("ERROR: Failed to create semaphore!");

		// Create & validate global mutex handles
		ghMutexSyncStdout = CreateMutex(NULL, FALSE, NULL);
		if (!ghMutexSyncStdout)
			throw std::runtime_error("ERROR: Failed to create mutex!");

		ghMutexSyncProcCreation = CreateMutex(NULL, FALSE, NULL);
		if (!ghMutexSyncProcCreation)
			throw std::runtime_error("ERROR: Failed to create mutex!");
			
		// Create given number of worker threads
		HANDLE
			*hProcs = new HANDLE[numof_procs],
			*hIt = hProcs,
			*hEnd = hProcs + numof_procs;

		// Launch worker threads
		for (; hIt < hEnd; ++hIt) {
			*hIt = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
			if (*hIt == NULL)
				throw std::runtime_error("ERROR: Failed to start process!");
		}

		// Wait for all worker threads to terminate
		WaitForMultipleObjects(numof_procs, hProcs, TRUE, INFINITE);

		// Close & clear all worker threads handles
		hIt = hProcs;
		hEnd = hProcs + numof_procs;
		
		while (hIt < hEnd)
			CloseHandle(*hIt++);
		delete[] hProcs;
	}
	catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}
	
	// Close semaphore & mutex handles
	if (ghSemaphore)
		CloseHandle(ghSemaphore);
	
	if (ghMutexSyncStdout)
		CloseHandle(ghMutexSyncStdout);

	if (ghMutexSyncProcCreation)
		CloseHandle(ghMutexSyncProcCreation);	

	system("pause");
	return 0;
}

void stdout_mutex(HANDLE& hMutex, const std::string& message) {
	// Wait mutex to signal
	if (WaitForSingleObject(hMutex, INFINITE) == WAIT_OBJECT_0) {
		// Write message to stdout
		std::cout << message;
		
		// Release mutex after writing to stdout
		if (!ReleaseMutex(hMutex))
			throw std::runtime_error("ERROR: Failed to release mutex!");
	}
	else
		throw std::runtime_error("ERROR: Mutex abandoned or failed!");
}

DWORD WINAPI ThreadProc(LPVOID arg) {
	UNREFERENCED_PARAMETER(arg);
	
	// Create empty Startup Information structure
	STARTUPINFO siArg;
	ZeroMemory(&siArg, sizeof(STARTUPINFO));

	// Create empty Process Information structure
	PROCESS_INFORMATION piArg;
	ZeroMemory(&piArg, sizeof(PROCESS_INFORMATION));

	// Initialize Named Pipe handle
	HANDLE hNamedPipe = INVALID_HANDLE_VALUE;

	// Create empty buffer for incoming messages
	char buffer[SERVE_BUF_LEN];
	
	// Dangerous magic
	try {
		// Ask mutex to create process
		if (WaitForSingleObject(ghMutexSyncProcCreation, INFINITE) == WAIT_OBJECT_0) {
			// Create & validate Named Pipe
			hNamedPipe = CreateNamedPipe(
				PIPE_NAME, PIPE_ACCESS_INBOUND,
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES, SERVE_BUF_LEN,
				SERVE_BUF_LEN, 0, NULL);
			if (hNamedPipe == INVALID_HANDLE_VALUE)
				throw std::runtime_error("ERROR: Failed to create Named Pipe!");

			// Create child process in new console window
			BOOL bCreate = CreateProcess(
				NULL, (char*)"main-proc-client.exe",
				NULL, NULL, FALSE, CREATE_NEW_CONSOLE,
				NULL, NULL, &siArg, &piArg
			);

			if (bCreate) {
				// Close stdio handles
				CloseHandle(siArg.hStdError);
				CloseHandle(siArg.hStdInput);
				CloseHandle(siArg.hStdOutput);

				// Close process handles
				CloseHandle(piArg.hProcess);
				CloseHandle(piArg.hThread);
			}
			else
				throw std::runtime_error("ERROR: Can't create process!");

			// Wait for the client to connect
			if (!ConnectNamedPipe(hNamedPipe, NULL)) {
				// If client failed to connect
				// then close pipe handle and
				// generate an exception
				CloseHandle(hNamedPipe);
				throw std::runtime_error("ERROR: Client failed to connect to Named Pipe!");
			}

			// Release mutex after task
			if (!ReleaseMutex(ghMutexSyncProcCreation))
				throw std::runtime_error("ERROR: Failed to release mutex!");
		}
		else
			throw std::runtime_error("ERROR: Mutex abandoned or failed!");

		// Wait for semaphore to signal
		if (WaitForSingleObject(ghSemaphore, INFINITE) == WAIT_OBJECT_0) {
			// Receive messages from child process 
			// till escape sequence
			for (DWORD dwBRead;;) {
				// Read message from pipe
				if (ReadFile(hNamedPipe, &buffer, SERVE_BUF_LEN * sizeof(char), &dwBRead, NULL) && dwBRead) {
					// Check how long is the message & print it
					// If length != 1 then ignore message
					if (dwBRead == 1) {
						std::string message = "@ Received new message from thread #";
						message += std::to_string(GetCurrentThreadId());
						message += ':';
						message += ' ';
						message += buffer[0];
						message += '\n';

						// Write message to console
						stdout_mutex(ghMutexSyncStdout, message);
						
						// Stop waiting new messages
						// because client sent 'exit'
						if (buffer[0] == '4')
							break;
					}
				}
				else if (GetLastError() == ERROR_BROKEN_PIPE) {
					// Notify about disconnected client
					std::string message = "* Client has disconnected from thread #";
					message += std::to_string(GetCurrentThreadId());
					message += '\n';

					// Write message to console
					stdout_mutex(ghMutexSyncStdout, message);
					break;
				}
				else
					throw std::runtime_error("ERROR: Failed to read from Pipe!");
			}

			// Release the semaphore when task is finished
			if (!ReleaseSemaphore(ghSemaphore, 1, NULL))
				throw std::runtime_error("ERROR: Failed to release semaphore!");
		}
		else
			throw std::runtime_error("ERROR: Semaphore failed!");
	}
	catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}
	
	// Disconnect server side of Pipe
	// and close Pipe handle
	if (hNamedPipe != INVALID_HANDLE_VALUE) {
		DisconnectNamedPipe(hNamedPipe);
		CloseHandle(hNamedPipe);
	}

	return TRUE;
}