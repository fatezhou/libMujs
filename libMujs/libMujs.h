#ifndef __LIB_MUJS_H__
#define __LIB_MUJS_H__

#define OUT_PARAM
#include <string>
#include <vector>

typedef std::string(*pCallCFunctionFromJs)(const std::string& strFunctionName, const std::vector<std::string>& vecParams);//the results char* maybe free in this lib after the return as the callback's return value

void mujs_init(std::string strJsFile = "");
bool mujs_bind_c_function(const std::string& strFunctionName, pCallCFunctionFromJs ptr, int nParamCount);
bool mujs_evalute_js(const std::string& strScript, bool bWaitForFinish = true);
bool mujs_call_js_function(const std::string& strFunctionName, const std::vector<std::string>& vecParam, OUT_PARAM std::string& strReturn);
std::string mujs_show_builtin_function();
#endif