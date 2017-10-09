#include "../libMujs/libMujs.h"
#pragma comment(lib, "../output/debug/libmujs.lib")
#include <windows.h>

int main(){
	mujs_show_builtin_function();
	mujs_init();
	mujs_evalute_js("function add(str){print('Hello '+str);return 'Hello ' + str;};");
	mujs_evalute_js("clearTimeout(setTimeout(\'function(){print(123456);return 10;}\', 1000));");
	std::vector<std::string> vec;
	vec.push_back("Mujs");
	std::string strRes;
	mujs_call_js_function("add", vec, strRes);

	WaitForSingleObject(CreateEvent(NULL, false, false, NULL), INFINITE);
}