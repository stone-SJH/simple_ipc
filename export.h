#if IPC_WIN32
#define API __stdcall
#define EXPORT __declspec(dllexport)
#elif IPC_OSX
#define API 
#define EXPORT 
#else
#define API
#define EXPORT
#endif

#if __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

EXTERN_C bool EXPORT API start_tcworker(char* tcpath, char* ipcname);
EXTERN_C void EXPORT API terminate_tcworker(char* ipcname);
EXTERN_C bool EXPORT API send_command(char* ipcname, char* cmd);
EXTERN_C bool EXPORT API read(char* ipcname, char** msg);
EXTERN_C int EXPORT API version();