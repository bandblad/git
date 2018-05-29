#include <iostream>
#include <string>

#define MSG_INVALID "# Invalid message!"
#define DT_INVALID "# Invalid data in stream!"

int main() {
	try {
		std::cerr.clear();
		std::cerr.flush();

		std::cout << "Hello, i am child process!\n\
		Type characters from 0 to 3 to send message.\n\
		Type characters 4 to send message and terminate.\n\
		Please try not to break something!" << std::endl;
		
		// Read user input from console
		for (std::string el; true;) {
			// Read user message
			std::cout << "> ";
			std::getline(std::cin, el);
			
			// Check for errors & send message
			if (std::cin.good())
				if (el.length() == 1) {
					if (el[0] >= '0' && el[0] <= '4') {
						std::cerr << el[0];
						if (el[0] == '4')
							break;
					}
					else
						std::cout << MSG_INVALID << std::endl;
				}	
				else
					std::cout << MSG_INVALID << std::endl;
			else
				throw std::runtime_error(DT_INVALID);
		}
	}
	catch (std::exception e) {
		std::cout << e.what() << std::endl;
		
		// End of session
		std::cerr << '4';
	}	

	return 0;
}