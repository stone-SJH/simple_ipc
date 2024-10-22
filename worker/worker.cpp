#include "../ipc/ipc.h"
#include "../tls/IPCUtility.h"

#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <random>

const char* ipcStreamName = "worker-1234-5678";


int main(int argc, char* argv[])
{
    if (argc == 2)
        ipcStreamName = argv[1];

	IPC_Stream* ipcStream;
	IPC_ErrorCode error = IPC_Succeeded;
	ipcStream = IPC_Create(&error);
	if (error != IPC_Succeeded)
		return 1;
	bool connected = false;
	int retry = 0;
	
	while (!connected && retry < 100) {
		IPC_ConnectTo(ipcStream, ipcStreamName, &error);
		if (error == IPC_Succeeded ||
			error == IPC_AlreadyConnected)
		{
			connected = true;
		}
		else {
			retry++;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "retry " << retry << " time..." << std::endl;
		}
	}

	if (!connected) {
		IPC_Destroy(ipcStream, &error);
		return 2;
	}

	std::string msg;
	int ret = -1;

	try {
		while (1) {
			{
				bool ok = SafeRead(ipcStream, msg);
				if (!ok) {
					// ipc reading error 
				}
				if (msg.empty()) {
					// ipc connection closed
					ret = 3;
				}
				if (msg == "shutdown") {
					// clean shutdown
					std::cout << "shutdown" << std::endl;
					ret = 0;
					break;
				}
				//do something special here
                if (msg == "Hello, world"){
					//wait random seconds to simulate job execution
					std::random_device rd;
					std::default_random_engine engine(rd());
					std::uniform_int_distribution<int> distribution(0, 10000);
					int randomMilliseconds = distribution(engine);
					std::chrono::milliseconds duration(randomMilliseconds);
					std::this_thread::sleep_for(duration);

                    WriteString(ipcStream, "Copy that");
                }
			}
		}
	}
	catch (IPCException& e) {
		std::cout << "Unhandled exception: " << e.what() << " (error " << (uint32_t)(e.GetData().m_ErrorID) << ", transferred " << e.GetData().m_BytesCompleted << '/' << e.GetData().m_BytesTotal << ')' << std::endl;
	}

	error = IPC_Succeeded;
	IPC_Disconnect(ipcStream, &error);
	IPC_Destroy(ipcStream, &error);

	int v;
	std::cin >> v;
	return ret;
}