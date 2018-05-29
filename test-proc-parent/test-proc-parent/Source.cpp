#include <iostream>
#include <windows.h>

int main() {
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	HANDLE 
		hReadPipe = NULL, 
		hWritePipe = NULL;

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0))
		throw std::runtime_error("StdoutRd CreatePipe");

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0))
		throw std::runtime_error("Stdout SetHandleInformation");

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = hWritePipe;
	siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	siStartInfo.dwFlags = STARTF_USESTDHANDLES;
	
	BOOL result = CreateProcess(
		NULL,
		(char*)"proc-child.exe",
		NULL,
		NULL,
		TRUE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&siStartInfo,
		&piProcInfo
	);

	if (result) {
		// Close process/thread handles
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	} else 
		throw std::runtime_error("Can't create process!");

	for (;;)
	{
		DWORD nBytesRead;
		const unsigned int
			buf_size = 127,
			buf_max = 128;
		
		char buf[buf_max];
		
		if (!ReadFile(hReadPipe, buf, buf_size, &nBytesRead, NULL))
			throw std::runtime_error("Pipe error!");
		
		if (nBytesRead) {
			std::cout << nBytesRead << " bytes received from pipe: ";
			for (int i = -1; ++i < nBytesRead;)
				std::cout << buf[i];
			std::cout << std::endl;
		} else break;
	}

	CloseHandle(hReadPipe);
	CloseHandle(hWritePipe);

	return 0;
}