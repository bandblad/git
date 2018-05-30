#include <windows.h> 
#include <iostream>
#include <string>

#define SERVE_BUF_LEN 512
#define PIPE_NAME "\\\\.\\pipe\\foo"

// Global mutex handle
HANDLE ghMutex;

// Combination of stdout & mutex object
void stdout_mutex(HANDLE&, const std::string&);

// Thread task
DWORD WINAPI ThreadProc(LPVOID);

int main()
{
	try {
		std::cout << "* Server started on pipe 'foo'\
		\n# Waiting for clients to connect..." << std::endl;
		
		// Create global mutex handle
		// Default security attributes, not owned, no name
		ghMutex = CreateMutex(NULL, FALSE, NULL);

		if (!ghMutex)
			throw std::runtime_error("ERROR: Failed to create mutex!");

		for (;;) {
			// Create Named Pipe to handle client
			HANDLE hNamedPipe = CreateNamedPipe(
				PIPE_NAME, PIPE_ACCESS_INBOUND,
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES, SERVE_BUF_LEN,
				SERVE_BUF_LEN, 0, NULL);

			// Validate Pipe
			if (hNamedPipe == INVALID_HANDLE_VALUE)
				throw std::runtime_error("ERROR: Failed to create Named Pipe!");

			// Wait for the client to connect
			if (ConnectNamedPipe(hNamedPipe, NULL)) {
				std::cout << "* New client has connected!\n# Starting new thread..." << std::endl;
				
				// Create thread for new client
				HANDLE hNewThread = CreateThread(
					NULL, 0,
					ThreadProc,
					(LPVOID)hNamedPipe,
					0, NULL
				);

				if (hNewThread == NULL)
					throw std::runtime_error("ERROR: Failed to start thread!");
			}
			else {
				// If client failed to connect
				// then close pipe handle and
				// generate an exception
				CloseHandle(hNamedPipe);
				throw std::runtime_error("ERROR: Client failed to connect to Named Pipe!");
			}
		}
	}
	catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}

	// Close mutex handle
	CloseHandle(ghMutex);

	system("pause");
	return 0;
}

void stdout_mutex(HANDLE& hMutex, const std::string& message) {
	// Wait for mutex response
	if (WaitForSingleObject(hMutex, INFINITE) == WAIT_OBJECT_0) {
		// Mutex signalled
		// Write to stdio
		std::cout << message;

		// Release mutex after write operation
		if (!ReleaseMutex(hMutex))
			throw std::runtime_error("ERROR: Failed to release mutex!");
	}
	else
		throw std::runtime_error("ERROR: Mutex abandoned or failed!");
}

DWORD WINAPI ThreadProc(LPVOID arg) {
	// Cast argument to Pipe pointer
	HANDLE hNamedPipe = (HANDLE)arg;

	// Allocate buffer for incoming messages
	char msg_ptr[SERVE_BUF_LEN];

	// Loop throught user messages
	for (DWORD dwBRead;;) {
		// Read message from pipe
		if (ReadFile(hNamedPipe, &msg_ptr, SERVE_BUF_LEN * sizeof(char), &dwBRead, NULL) && dwBRead) {
			// Form message
			std::string message = "@ Message is: ";
			message += std::string(msg_ptr, dwBRead);
			message += '\n';

			// Show message in console
			stdout_mutex(ghMutex, message);
		}
		else if (GetLastError() == ERROR_BROKEN_PIPE) {
			// Notify about disconnected client
			stdout_mutex(ghMutex, "* Client has disconnected!\n");
			break;
		}
		else
			throw std::runtime_error("ERROR: Failed to read from Pipe!");
	}

	// Disconnect server side of Pipe
	// and close Pipe handle
	DisconnectNamedPipe(hNamedPipe);
	CloseHandle(hNamedPipe);

	return TRUE;
}