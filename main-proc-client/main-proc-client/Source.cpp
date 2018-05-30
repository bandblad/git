#include <iostream>
#include <string>
#include <windows.h> 

#define PIPE_NAME "\\\\.\\pipe\\foo"

int main()
{
	// Initialize instance of Named Pipe handle
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	
	try {
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
		\nMessages must be numbers from 0 to 4.\
		\nNumbers from 0 to 3 is regular events.\
		\nNumber 4 is special event - 'exit'.\n"
		<< std::endl;

		for (std::string el;;) {
			// Read message from stdin
			// 'exit' is the exit
			std::cout << '>' << ' ';
			std::getline(std::cin, el);

			if (std::cin.good()) {
				// Check length of message
				// If length() != 1 then ignore message
				if (el.length() == 1) {
					char msg = el[0];
					if (msg >= '0' && msg <= '4') {
						// If message is '4' then
						// disconnect from pipe
						if (msg == '4')
							break;

						// Send message to server
						if (!WriteFile(hPipe, &msg, 1, &dwArg, NULL))
							throw std::runtime_error("ERROR: Failed to write to pipe!");

						// Flush Named Pipe
						FlushFileBuffers(hPipe);
					}
				}
			}
			else
				throw std::runtime_error("ERROR: stdin failed!");
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
