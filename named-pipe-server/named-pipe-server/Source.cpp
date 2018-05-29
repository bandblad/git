#include <windows.h> 
#include <iostream>

#define SERVE_BUF_MAX 512
#define SERVE_BUF_LEN SERVE_BUF_MAX - 1

#define PIPE_NAME "\\\\.\\pipe\\foo"
#define LOG(arg) std::cout << "LOG: " << arg << std::endl

int main()
{
	try {
		// Create Named Pipe to handle 
		// only ONE client at the time
		HANDLE hNamedPipe = CreateNamedPipe(
			PIPE_NAME, PIPE_ACCESS_INBOUND, 
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
			PIPE_UNLIMITED_INSTANCES, SERVE_BUF_LEN, 
			SERVE_BUF_LEN, 0, NULL);

		if (hNamedPipe == INVALID_HANDLE_VALUE)
			throw std::runtime_error("ERROR: Failed to create Named Pipe!");
		LOG("Pipe was created!");

		// Wait for the client to connect
		if (ConnectNamedPipe(hNamedPipe, NULL)) {
			LOG("Client was connected!");
			char msg_ptr[SERVE_BUF_LEN];
			
			DWORD dwBytesRead;
			BOOL bResult = 
				ReadFile(hNamedPipe, &msg_ptr, sizeof(msg_ptr), &dwBytesRead, NULL);

			if (!(bResult && dwBytesRead))
				if (GetLastError() == ERROR_BROKEN_PIPE)
					throw std::runtime_error("ERROR: Client disconnected!");
				else
					throw std::runtime_error("ERROR: Failed to read from Pipe!");
			LOG("Received message from pipe!");

			std::cout << "Message is: ";
			for (int i = -1; ++i < dwBytesRead;) std::cout << msg_ptr[i];
			std::cout << std::endl;

			// Flush Named Pipe buffer and
			// disconnect client
			FlushFileBuffers(hNamedPipe);
			DisconnectNamedPipe(hNamedPipe);
		} else {
			// If client failed to connect
			// then close pipe handle and
			// generate an exception
			CloseHandle(hNamedPipe);
			throw std::runtime_error("ERROR: Client failed to connect to Named Pipe!");
		}

		// Close pipe handle
		CloseHandle(hNamedPipe);
	}
	catch (std::exception e) {
		std::cerr << e.what() << std::endl;
	}

	system("pause");
	return 0;
}