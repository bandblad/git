#include <iostream>
#include <windows.h>
#include <stdio.h>

#define SEM_CNT 3

#define SERVE_BUF_MAX 128
#define SERVE_BUF_LEN SERVE_BUF_MAX - 1

DWORD WINAPI ThreadProc(LPVOID);

int main(void)
{
	try {
		// Prompt user action
		std::cout << "Type number of processes to start (>0): ";

		// Read number of processess
		int numof_procs;
		std::cin >> numof_procs;

		// Catch errors in user input
		if (!std::cin.good())
			throw std::runtime_error("ERROR: Invalid data in stream!");

		if (numof_procs < 1)
			throw std::invalid_argument("ERROR: Invalid value!");

		// Create semaphore handle
		HANDLE 
			hSemaphore = CreateSemaphore(NULL, SEM_CNT, SEM_CNT, NULL);

		// Is the semaphore initialized?
		if (!hSemaphore)
			throw std::runtime_error("ERROR: Failed to initialize semaphore!");

		// Create given number of worker threads
		HANDLE
			*hProcs = new HANDLE[numof_procs],
			*hIt = hProcs,
			*hEnd = hProcs + numof_procs;

		// Launch worker threads
		while (hIt < hEnd) {
			*hIt = CreateThread(
				NULL, 0, 
				(LPTHREAD_START_ROUTINE)ThreadProc, 
				&hSemaphore, 
				0, NULL
			);

			if (*hIt == NULL)
				throw std::runtime_error("ERROR: Failed to start process!");
			++hIt;
		}

		// Wait for all worker threads to terminate
		WaitForMultipleObjects(numof_procs, hProcs, TRUE, INFINITE);

		// Close & clear all worker threads handles
		hIt = hProcs;
		hEnd = hProcs + numof_procs;
		
		while (hIt < hEnd)
			CloseHandle(*hIt++);
		delete[] hProcs;

		// Close semaphore handle
		CloseHandle(hSemaphore);
	}
	catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}
	
	system("pause");
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID arg)
{
	// Create pipe & semaphore handles
	HANDLE
		hReadPipe = NULL,
		hWritePipe = NULL,
		*hSemaphore = (HANDLE*)arg;

	// We will be inheriting STDIO handles
	SECURITY_ATTRIBUTES saArg;
	saArg.nLength = sizeof(SECURITY_ATTRIBUTES);
	saArg.bInheritHandle = TRUE;
	saArg.lpSecurityDescriptor = NULL;

	// Create anonimous IO pipe
	if (!CreatePipe(&hReadPipe, &hWritePipe, &saArg, 0))
		throw std::runtime_error("ERROR: Failed to create pipe!");

	// Ensure the read handle to the pipe is not inherited
	if (!SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0))
		throw std::runtime_error("Stdout SetHandleInformation");

	// Define process startup attributes
	STARTUPINFO siArg;
	ZeroMemory(&siArg, sizeof(STARTUPINFO));

	siArg.cb = sizeof(STARTUPINFO);

	// Redirect stderr to pipe
	siArg.dwFlags = STARTF_USESTDHANDLES;
	siArg.hStdError = hWritePipe;

	siArg.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	siArg.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	// Create empty Process Information structure
	PROCESS_INFORMATION piArg;
	ZeroMemory(&piArg, sizeof(PROCESS_INFORMATION));

	// Create child process in new console window
	BOOL bCreate = CreateProcess(
		NULL, (char*)"proc-child.exe",
		NULL, NULL, TRUE, CREATE_NEW_CONSOLE,
		NULL, NULL, &siArg, &piArg
	);

	// Close process & thread handles if succeeded
	// We do not need them
	if (bCreate) {
		CloseHandle(piArg.hProcess);
		CloseHandle(piArg.hThread);
	}
	else
		throw std::runtime_error("ERROR: Can't create process!");

	// Create empty buffer for incoming messages
	char buffer[SERVE_BUF_MAX];

	DWORD 
		dwWait, 
		dwBCnt;

	// Try to enter the semaphore gate
	while (dwWait = WaitForSingleObject(*hSemaphore, 0) == WAIT_TIMEOUT);

	// If we successfully entered semaphore then perform task
	if (dwWait == WAIT_OBJECT_0) {
		// Receive messages from child process 
		// till escape sequence
		for (;;) {
			// Clear buffer
			ZeroMemory(buffer, SERVE_BUF_MAX);

			// Read message from pipe
			if (!ReadFile(hReadPipe, buffer, SERVE_BUF_LEN, &dwBCnt, NULL))
				throw std::runtime_error("ERROR: Can't read from pipe!");

			// Check how long is the message & print it
			// If length is not == 1 then ignore message
			if (dwBCnt == 1) {
				std::cout << "Received new message: " << buffer[0] << std::endl;
				if (buffer[0] == '4')
					break;
			}
		}

		// Release the semaphore when task is finished
		if (!ReleaseSemaphore(*hSemaphore, 1, NULL))
			throw std::runtime_error("ERROR: Failed to release semaphore!");
	}
	else
		throw std::runtime_error("ERROR: Failed to enter semaphore after timeout!");

	// Close pipe handles
	CloseHandle(hReadPipe);
	CloseHandle(hWritePipe);

	return TRUE;
}