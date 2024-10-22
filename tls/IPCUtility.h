#include "../ipc/ipc.h"
#include <exception>
#include <vector>
#include <string>

struct IPCData
{
	IPCData() : IPCData(false) {}
	explicit IPCData(bool isTimeout) : IPCData(isTimeout, 0, 0, 0) {}
	IPCData(bool isTimeout, int errorID, size_t completed, size_t total)
		: m_TimeOut(isTimeout)
		, m_ErrorID(errorID)
		, m_BytesCompleted(completed)
		, m_BytesTotal(total)
	{}

	size_t m_BytesCompleted;
	size_t m_BytesTotal;
	int m_ErrorID;
	bool m_TimeOut;
};

class IPCException : public std::exception
{
public:
	explicit IPCException(const std::string& about)
		: m_What(about)
		, m_Data(false)
	{}
	IPCException(const std::string& about, const IPCData& data)
		: m_What(about)
		, m_Data(data)
	{}

	~IPCException() throw () {}
	virtual const char* what() const throw () { return m_What.c_str(); }
	const IPCData& GetData() const { return m_Data; }
private:
	std::string m_What;
	IPCData m_Data;
};

static const size_t kMaxWriteSize = 65536;

bool Write(IPC_Stream* ipcStream, const void* buffer, size_t numBytes, IPCData& data, uint32_t writeTimeoutMS = 600000);
bool Read(IPC_Stream* ipcStream, void* buffer, size_t numBytes, IPCData& data, uint32_t readTimeoutMS = 600000);
void WriteData(IPC_Stream* ipcStream, const void* buffer, size_t numBytes, IPCData& data);
void WriteLine(IPC_Stream* ipcStream, const std::string& msg);
void ReadLine(IPC_Stream* ipcStream, std::string& outmsg);
void WriteBuffer(IPC_Stream* ipcStream, const void* data, size_t length);
void ReadBuffer(IPC_Stream* ipcStream, std::vector<uint8_t>& buffer);
void WriteString(IPC_Stream* ipcStream, const std::string& s);
void WriteUInt32(IPC_Stream* ipcStream, uint32_t i);
void WriteInt(IPC_Stream* ipcStream, int i);
void WriteBool(IPC_Stream* ipcStream, bool b);
std::string ReadString(IPC_Stream* ipcStream);
int ReadInt(IPC_Stream* ipcStream);
uint32_t ReadUInt32(IPC_Stream* ipcStream);
bool ReadBool(IPC_Stream* ipcStream);

// return true && emptry msg -> IPC connection Closed.
// return false && empty msg -> IPC reading error.
// return true && non-empty msg -> Normal.
bool SafeRead(IPC_Stream* ipcStream, std::string& msg);
bool SafeRead(IPC_Stream* ipcStream, char** data);