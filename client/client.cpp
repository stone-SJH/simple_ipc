#include "../ipc/ipc.h"
#include "../tls/IPCUtility.h"
#include "../tls/ThreadSpecificValue.h"
#include "../tls/Process.h"

#include <string>
#include <chrono>
#include <thread>
#include <iostream>

const std::string ipcStreamName = "worker-1234-5678";
#if IPC_WIN32
//const std::string workerAppPath = "C:\\Users\\wy301\\source\\repos\\ipc\\worker\\out\\build\\x64-Release\\ipc-worker.exe";
//const std::string workerAppPath = "C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\YaTCompress.exe";
const std::string workerAppPath = "C:/Users/wy301/source/repos/ipc/TextureCompressionV1/bin/YaTCompress.exe";
#elif IPC_OSX
//const std::string workerAppPath = "/Users/stone/work/c++/ipc/worker/cmake-build-release/ipc-worker";
const std::string workerAppPath = "../../TextureCompressionV1/bin/YaTCompress";
#endif


struct DataPerThread {
	IPC_Stream* stream;
	std::string ipcname;
	size_t threadid;
    Process* proc;
};

static ThreadSpecificValue<DataPerThread*> data;

void working(std::string ipcname) {
	DataPerThread* currentThreadData = new DataPerThread();
	data = currentThreadData;

    //start worker process with args
    std::vector<std::string> args;
    args.push_back(ipcname);
    Process* p = new Process(workerAppPath, args);
    data->proc = p;
    data->proc->Launch();

    IPC_Stream* listenIPCStream;
	IPC_ErrorCode error = IPC_Succeeded;
	listenIPCStream = IPC_Create(&error);
	if (error != IPC_Succeeded)
		return;
	IPC_Listen(listenIPCStream, ipcname.c_str(), &error);
	if (error != IPC_Succeeded)
		return;

	IPC_Stream* ipcStream;	
	ipcStream = IPC_AcceptSynchronous(listenIPCStream, 15000 * 1000, &error);

	data->ipcname = ipcname;
	data->threadid = std::hash<std::thread::id>()(std::this_thread::get_id());
	data->stream = ipcStream;

	if (error != IPC_Succeeded)
		return;

	std::cout << "working thread #" << data->ipcname << " starting with thread id: " << data->threadid << std::endl;
	try {
		//int cnt = 0;
		//while (1) {
		//	{
		//		IPC_Stream* stream = data->stream;
		//		WriteString(stream, "Hello, world");
		//		cnt++;
  //              std::string resp;
  //              while (!SafeRead(stream, resp)){
  //                  std::this_thread::sleep_for(std::chrono::seconds(1));
  //              }
  //              std::cout << "working thread [" << data->ipcname << "] with thread id #" << data->threadid << " says: " << resp << std::endl;

  //              if (cnt > 10) {
		//			WriteString(stream, "shutdown");
		//			break;
		//		}
		//	}
		//}

		IPC_Stream* stream = data->stream;
#if IPC_WIN32
		WriteString(stream, "tc bc 4 C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\input.png C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\output.dds");
		WriteString(stream, "tc bc 7 C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\input.png C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\output.dds");
		WriteString(stream, "shutdown");
#elif IPC_OSX
        WriteString(stream, "tc astc 4x4 ../../TextureCompressionV1/bin/input.png ../../TextureCompressionV1/bin/output.astc");
#endif
        char* resp = "123";
        while (1){
			bool ok = SafeRead(stream, &resp);
			if (!ok) {
				// ipc reading error 
				break;
			}
			//if (resp.empty()) {
			//	// ipc connection closed
			//	break;
			//}
			std::cout << "working thread [" << data->ipcname << "] with thread id #" << data->threadid << " says: " << resp << std::endl;
        }
	}
	catch (IPCException& e) {
		std::cout << "Unhandled working thread exception: " << e.what() << " (error " << (uint32_t)(e.GetData().m_ErrorID) << ", transferred " << e.GetData().m_BytesCompleted << '/' << e.GetData().m_BytesTotal << ')' << std::endl;
	}

	std::cout << "working thread #" << data->ipcname << " stoping with thread id: " << data->threadid << std::endl;

	error = IPC_Succeeded;
	IPC_Disconnect(ipcStream, &error);
	IPC_Destroy(ipcStream, &error);
    if (data->proc->IsRunning()){
        //should never go here
        data->proc->Shutdown();
    }
}

int main()
{
	size_t mtid = std::hash<std::thread::id>()(std::this_thread::get_id());
	std::cout << "main threadid: " << mtid << std::endl;

	//start a working thread and run it
	std::thread t([]() {working("yatc-001"); });
	t.detach();

	//std::thread t2([]() {working("yatc-002");});
	//t2.detach();

 //   std::thread t3([]() {working("yatc-003");});
 //   t3.detach();

 //   std::thread t4([]() {working("yatc-004");});
 //   t4.detach();
	try {
		int cnt = 0;
		while (1) {
			{
				// keep mt working
				std::this_thread::sleep_for(std::chrono::seconds(10));
				std::cout << "main threadid: " << mtid << std::endl;
			}
		}
	}
	catch (IPCException& e) {
		std::cout << "Unhandled main thread exception: " << e.what() << " (error " << (uint32_t)(e.GetData().m_ErrorID) << ", transferred " << e.GetData().m_BytesCompleted << '/' << e.GetData().m_BytesTotal << ')' << std::endl;
	}

	int i;
	std::cin >> i;
	return 0;
}