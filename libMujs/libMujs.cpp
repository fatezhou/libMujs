#include "libMujs.h"
#include "mujs/mujs.h"
#include "mujs/jsi.h"

#include <map>
#include <string>

#include <windows.h>

#define MaxCallbackParamCount 32
#define JS_MSG_EVALUTE			WM_USER + 0x0100
#define JS_MSG_CALL_FUNCTION	WM_USER + 0x0101
#define JS_MSG_SET_TIMEOUT			WM_USER + 0x0102
#define JS_MSG_SET_INTERVAL			WM_USER + 0x0103

js_State* g_jss = nullptr;

void CALLBACK TimerProc(HWND hWnd, UINT nIdEvent, UINT_PTR nId, DWORD dwTimer);

namespace Neko{
	extern std::string GetProcessDir();
	extern std::string GetDataFromFile(const std::string& strPath);

	struct BindFunctionInfo{
		pCallCFunctionFromJs ptr;
		int nParamCount;
	};

	struct CallJsFunctionInfo{
	public:
		CallJsFunctionInfo(const std::string& _strFunctionName, const std::vector<std::string>& _vecParam) : vecParam(_vecParam), strFunctionName(_strFunctionName){};
		const std::vector<std::string>& vecParam;
		const std::string& strFunctionName;
		std::string strResult;
		bool bSucc;
	};

	struct SIntervalStruct{
		HANDLE* pHandle;
		int nMillisecond;
		std::string strScript;
		int nId;
	};

	struct STimerStruct{
		int nId;
		std::string strScript;
		bool bOnce;
	};
	std::map<int, STimerStruct> mapTimerParam;

	bool inner_mujs_call_js_function(const std::string& strFunctionName, const std::vector<std::string>& vecParam, std::string& strReturn)
	{
		js_getglobal(g_jss, strFunctionName.c_str());
		js_pushnull(g_jss);
		int nLen = vecParam.size();
		for (int i = 0; i < nLen; i++){
			js_pushstring(g_jss, vecParam[i].c_str());
		}
		if (js_pcall(g_jss, 1)){
			strReturn = "call fail";
			return false;
		}
		else{
			strReturn = js_tostring(g_jss, -1);
			return true;
		}
	}

	std::map<std::string, BindFunctionInfo> g_bindFuncMap;
	static DWORD g_thread_id = 0;

	class _Key{
	public:
		_Key(){
			::InitializeCriticalSection(&cs);
		}
		~_Key(){
			::DeleteCriticalSection(&cs);
		}
		CRITICAL_SECTION cs;
	};

	_Key _gKey;

	class _Lock{
	public:
		_Lock(_Key& key) : m_key(key){
			::EnterCriticalSection(&m_key.cs);
		}
		~_Lock(){
			::LeaveCriticalSection(&m_key.cs);
		};
	private:
		_Key& m_key;
	};

	int MessageLoop(){
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)){
			switch (msg.message){
				case JS_MSG_EVALUTE:{
					char* pString = (char*)msg.wParam;
					js_dostring(g_jss, pString);
					HANDLE* pEvent = (HANDLE*)msg.lParam;
					delete pString;
					if (pEvent){
						SetEvent(*pEvent);
					}
				}break;
				case JS_MSG_CALL_FUNCTION:{
					CallJsFunctionInfo* pInfo = (CallJsFunctionInfo*)msg.wParam;
					HANDLE* pEvent = (HANDLE*)msg.lParam;
					pInfo->bSucc = inner_mujs_call_js_function(pInfo->strFunctionName, pInfo->vecParam, pInfo->strResult);
					SetEvent(*pEvent);
				}break;
				case WM_TIMER:{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}break;
			}
		}
		return 0;
	}
}

using namespace Neko;

void CALLBACK TimerProc(HWND hWnd, UINT nIdEvent, UINT_PTR nId, DWORD dwTimer){
	_Lock lock(_gKey);
	auto iter = mapTimerParam.find(nId);
	if (iter == mapTimerParam.end()){
		return;
	}
		
	mujs_evalute_js(iter->second.strScript, false);
	if (iter->second.bOnce){
		KillTimer(NULL, nId);
		mapTimerParam.erase(iter);
	}
};
void mujs_CallbackRouter(js_State* jss){
	const std::string strFunctionName = js_tostring(jss, 1);
	_Lock lock(_gKey);

	auto iter = g_bindFuncMap.find(strFunctionName);
	if (iter == g_bindFuncMap.end()){
		return;
	}

	std::vector<std::string> vecParam;
	for (int i = 2; i < iter->second.nParamCount + 1; i++){
		vecParam.push_back(js_tostring(jss, i));
	}
	
	std::string strReturn = iter->second.ptr(strFunctionName, vecParam);
	if (strReturn.empty() == false){
		js_pushstring(jss, strReturn.c_str());
	}
	else{
		js_pushundefined(jss);
	}
}

void mujs_print(js_State* jss){
	std::string strText = js_tostring(jss, 1);
	std::string strName(jss->filename);
	int nLen = printf("[%s]:%s\n", strName.c_str(), strText.c_str());
	std::string strOutputDebugString;
	strOutputDebugString.resize(nLen + 1);
	sprintf((char*)strOutputDebugString.c_str(), "[%s]:%s\n", strName.c_str(), strText.c_str());
	OutputDebugStringA(strOutputDebugString.c_str());
	js_pushnumber(jss, nLen);
}

void set_interval(js_State* jss, bool bOnce){
	std::string strFunction;
	strFunction = js_tostring(jss, 1);

	std::string strDelayMilliSecond = js_tostring(jss, 2);
	int nDelayMilliSecond = atoi(strDelayMilliSecond.c_str());		
	std::string strDeal = "(";
	strDeal += strFunction;
	strDeal += ")();";
	if (nDelayMilliSecond <= 0){
		js_dostring(g_jss, strDeal.c_str());
		js_pushnumber(jss, 0);
	}
	else{
		int nId = SetTimer(NULL, 0, nDelayMilliSecond, TimerProc);
		mapTimerParam[nId] = { nId, strDeal, bOnce };//单线程不必加锁
		js_pushnumber(jss, nId);
	}
}

void mujs_setTimeout(js_State* jss){
#ifdef 立马回调
	std::string strFunction;	
	js_copy(jss, 1);
	const char* pFunctionHandle = js_ref(jss);
	js_getregistry(jss, pFunctionHandle);
	js_pushnull(jss);
	js_pcall(jss, 2);
	js_pop(jss, 1);	
	printf(strFunction.c_str());
#else
	set_interval(jss, true);
#endif
}

void mujs_setInterval(js_State* jss){
	set_interval(jss, false);
}

void clear_interval(js_State* jss){
	int nId = js_tonumber(jss, 1);
	KillTimer(NULL, nId);
	js_pushundefined(jss);
}

void mujs_clearInterval(js_State* jss){
	clear_interval(jss);
}

void mujs_clearTimeout(js_State* jss){
	clear_interval(jss);
}

void mujs_require(js_State* jss){
	std::string strModule = js_tostring(jss, 1);
	std::string strJavascript;
	if (strModule.find(":") != -1){
		//real path
		strJavascript = GetDataFromFile(strModule.c_str());
	}
	else{
		std::string strJsFile = GetProcessDir() + "\\" + strModule;
		strJavascript = GetDataFromFile(strJsFile.c_str());
	}
	if (strJavascript.empty() == false){
		js_dostring(jss, strJavascript.c_str());
	}
}

#define BIND_FUNC(name, argc) \
	js_newcfunction(g_jss, mujs_##name, #name, argc);\
	js_setglobal(g_jss, #name);

void mujs_init(std::string strJsFile)
{
	g_jss = js_newstate(0, 0, JS_STRICT);
	//js_newcfunction(g_jss, mujs_CallbackRouter, "call_router", MaxCallbackParamCount);
	//js_setglobal(g_jss, "call_router");
	//js_newcfunction(g_jss, mujs_print, "print", 1);
	//js_setglobal(g_jss, "print");
	//js_newcfunction(g_jss, mujs_setTimeout, "setTimeout", 2);
	//js_setglobal(g_jss, "setTimeout");

	BIND_FUNC(CallbackRouter, MaxCallbackParamCount);
	BIND_FUNC(print, 1);
	BIND_FUNC(setTimeout, 2);
	BIND_FUNC(require, 1);
	BIND_FUNC(setInterval, 2);
	BIND_FUNC(clearInterval, 1);
	BIND_FUNC(clearTimeout, 1);
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MessageLoop, 0, 0, &Neko::g_thread_id);

	if (strJsFile.empty() == false){
		std::string strData = GetDataFromFile(strJsFile);
		js_dostring(g_jss, strData.c_str());
	}
}

bool mujs_bind_c_function(const std::string& strFunctionName, pCallCFunctionFromJs ptr, int nParamCount)
{
	_Lock lock(_gKey);
	auto iter = g_bindFuncMap.find(strFunctionName);
	if (iter != g_bindFuncMap.end()){
		return false;
	}
	else{
		g_bindFuncMap.insert(std::make_pair(strFunctionName, BindFunctionInfo{ptr, nParamCount}));
		return true;
	}
}

bool mujs_evalute_js(const std::string& strScript, bool bWaitForFinish /*= true*/)
{
	HANDLE* pEvent = 0;
	if (bWaitForFinish){
		pEvent = new HANDLE;
		*pEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	int nLen = strScript.length();
	char* pScript = new char[nLen + 1]{0};
	strcpy_s(pScript, nLen + 1, strScript.c_str());
	while (PostThreadMessage(Neko::g_thread_id, JS_MSG_EVALUTE, (WPARAM)pScript, (LPARAM)pEvent) == FALSE){
		Sleep(100);
	}
	if (bWaitForFinish){
		WaitForSingleObject(*pEvent, INFINITE);
		CloseHandle(*pEvent);
		delete pEvent;
	}
	return true;
}

bool mujs_call_js_function(const std::string& strFunctionName, const std::vector<std::string>& vecParam, std::string& strReturn)
{
	HANDLE* pEvent = new HANDLE;
	*pEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	CallJsFunctionInfo* pInfo = new CallJsFunctionInfo(strFunctionName, vecParam);
	while (PostThreadMessage(Neko::g_thread_id, JS_MSG_CALL_FUNCTION, (WPARAM)pInfo, (LPARAM)pEvent) == FALSE){
		Sleep(100);
	}
	WaitForSingleObject(*pEvent, INFINITE);
	CloseHandle(*pEvent);
	delete pEvent;
	strReturn = pInfo->strResult;
	bool bSucc = pInfo->bSucc;
	delete pInfo;
	return bSucc;
}

std::string mujs_show_builtin_function()
{
	static std::string strFunctions = "{\n"
		"	setTimeout(\"function(){}\", millisecond);\n"
		"	clearTimeout(id);\n"
		"	setInterval(\"function(){}\", millisecond);\n"
		"	clearInterval(id);\n"
		"	print(xxx);\n"
		"	require(xxx);\n"
		"}\n\n\n"
		"";
	printf("%s", strFunctions.c_str());
	return strFunctions;
}
