#include "export.h"
#include "ipc/ipc.h"
#include "tls/Process.h"
#include "tls/IPCUtility.h"

#include <string>
#include <unordered_map>


struct WorkerInfo {
	IPC_Stream* stream;
	std::string* ipcname;
	Process* proc;
};

static Mutex mtx;
static std::unordered_map<std::string, WorkerInfo*> workers;
const int max_worker_count = 7;

EXTERN_C bool EXPORT API start_tcworker(char* tcpath, char* ipcname) {
	Mutex::AutoLock lock(mtx);
	if (workers.size() >= max_worker_count) {
		return false;
	}

	if (workers.count(ipcname) > 0) {
		return false;
	}

	WorkerInfo* wi = new WorkerInfo();
	wi->ipcname = new std::string(ipcname);

	//launch process
	std::vector<std::string>args;
	args.push_back(ipcname);
	Process* p = new Process(tcpath, args);
	p->Launch();
	wi->proc = p;

	//establish ipc 
	IPC_Stream* listenIPCStream;
	IPC_ErrorCode error = IPC_Succeeded;
	listenIPCStream = IPC_Create(&error);
	if (error != IPC_Succeeded) {
		p->Shutdown();
		return false;
	}
	IPC_Listen(listenIPCStream, ipcname, &error);
	if (error != IPC_Succeeded){
		p->Shutdown();
		return false;
	}
	IPC_Stream* ipcStream;
	ipcStream = IPC_AcceptSynchronous(listenIPCStream, 15000, &error);
	if (error != IPC_Succeeded) {
		p->Shutdown();
		return false;
	}
	wi->stream = ipcStream;
	workers[ipcname] = wi;

	//destory listen stream
	IPC_Destroy(listenIPCStream, &error);

	return true;
}

EXTERN_C void EXPORT API terminate_tcworker(char* ipcname) {
	Mutex::AutoLock lock(mtx);
	if (workers.count(ipcname) == 0)
		return;
	WorkerInfo* wi = workers[ipcname];
	
	IPC_ErrorCode error = IPC_Succeeded;
	IPC_Disconnect(wi->stream, &error);
	IPC_Destroy(wi->stream, &error);
	if (wi->proc->IsRunning())
		wi->proc->Shutdown();
	workers.erase(ipcname);
}

EXTERN_C bool EXPORT API send_command(char* ipcname, char* cmd) {
	if (workers.count(ipcname) == 0)
		return false;
	if (!workers[ipcname]->proc->IsRunning())
		return false;
	try {
		WriteString(workers[ipcname]->stream, cmd);
	}
	catch (IPCException& ex) {
		return false;
	}
	return true;
}

EXTERN_C bool EXPORT API read(char* ipcname, char** data) {
	if (workers.count(ipcname) == 0)
		return false;
	if (!workers[ipcname]->proc->IsRunning())
		return false;
	bool ok;
	std::string msg;
	try {
		ok = SafeRead(workers[ipcname]->stream, msg);
		*data = new char[msg.size()];
		strcpy(*data, msg.c_str());
	}
	catch (IPCException& ex) {
		return false;
	}
	return ok;
}

EXTERN_C int EXPORT API version() {
	return 10001;
}