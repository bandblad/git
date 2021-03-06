#include <iostream>
#include <string>
#include <windows.h> 

#define PIPE_NAME "\\\\.\\pipe\\foo"

int main()
{
	// Initialize instance of Named Pipe handle
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	
	try {
		// Wait for Named Pipe to signal
		if (WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
			// Connect to Named Pipe
			hPipe = CreateFile(
				PIPE_NAME, GENERIC_WRITE, FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, 0, NULL
			);

			// Check for errors
			if (hPipe == INVALID_HANDLE_VALUE)
				throw std::runtime_error("ERROR: Failed to open named pipe!");
		}
		else
			throw std::runtime_error("ERROR: Named Pipe does not exist!");
		
		// Switch pipe to message mode
		DWORD dwArg = PIPE_READMODE_MESSAGE | PIPE_WAIT;
		if (!SetNamedPipeHandleState(hPipe, &dwArg, NULL, NULL))
			throw std::runtime_error("ERROR: Failed to switch pipe to message mode!");

		// Prompt user input
		std::cout << "Start sending messages to server.\
		\nType 'exit' in console to exit application.\n"
		<< std::endl;

		for (std::string el;;) {
			// Read message from stdin
			// 'exit' is the exit
			std::cout << '>' << ' ';
			std::getline(std::cin, el);
			
			// Check for empty messages
			if (el.length()) {
				if (el.compare("exit")) {
					// Send message to server
					if (!WriteFile(hPipe, el.c_str(), el.length(), &dwArg, NULL))
						throw std::runtime_error("ERROR: Failed to write to pipe!");

					// Flush if succeeded
					FlushFileBuffers(hPipe);
				}
				else break;
			}
		}
	}
	catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}

	// Close pipe handle
	if (hPipe != INVALID_HANDLE_VALUE)
		CloseHandle(hPipe);

	system("pause");
	return 0;
}
