#include <iostream>
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define PIPE_NAME "\\\\.\\pipe\\foo"
#define MSG "Hello, World!\0"
#define MSG_LEN 15

int main()
{
	try {
		// Connect to Named Pipe
		HANDLE hPipe = CreateFile(
			PIPE_NAME, GENERIC_WRITE, FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL
		);

		// Check for errors
		if (hPipe == INVALID_HANDLE_VALUE)
			throw std::runtime_error("ERROR: Failed to open named pipe!");

		// Check for errors
		if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER))
			throw std::runtime_error("ERROR: Failed to connect to named pipe!");

		// Switch pipe to message mode
		DWORD dwArg = PIPE_READMODE_MESSAGE;
		if (!SetNamedPipeHandleState(hPipe, &dwArg, NULL, NULL))
			throw std::runtime_error("ERROR: Failed to switch pipe to message mode!");

		// Send message to server
		BOOL bWriteFile = WriteFile(hPipe, MSG, MSG_LEN, &dwArg, NULL);

		// Check for errors
		if (!bWriteFile)
			throw std::runtime_error("ERROR: Failed to write to pipe!");

		// Close pipe handle
		CloseHandle(hPipe);
	}
	catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}

	system("pause");
	return 0;
}
