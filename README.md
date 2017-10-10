# libMujs
interface from mujs, easy use(own think)

首先 要感谢 mujs https://github.com/ccxvii/mujs
一切都是建立在这位dalao的引擎上面 小的只是作了简单的封装, 内容包括下列部分:
1. 增加了几个内建函数 require, 用来加载js模块, 直接理解为上下文就对了
2. 增加了消息队列来确保 js的执行为单线程, 
3. 增加了mujs_bind_c_function, 里面有内建一个 js call cpp的路由函数, 通过这些绑定的部分进行转发, 具体可以看源码
4. 增加了mujs_evalute_js 用来执行js片断 但是会压到消息队列的底层, 最后再执行 然后通过参数来决定是否异步
5. 增加了mujs_call_js_function, 用来主动调用js的函数, 参数暂时定为一个 vector<string>, 不要问我为什么不搞复杂点 传对象啥的, 用mujs就是为了
  开发简单, 轻装上阵
6. mujs_show_builtin_function用来看有哪几个我建立的内建函数; 
7. 然后实现了 setTimeout setInterval等几个常用的函数, 只不过有些问题, 就对了, 因为setTimeout的lambda
  没办法脱离上下文执行, 所以我增加了规则, lambda要写成字串 等我跟原作者沟通后看怎么改为标准版本的 
我会经常更新, 请粉我!

# IDE
我使用的是 vs2013, 其他版本没有测试过, 我也没有使用C++11的高级特性, 讲道理是可以兼容C++98,03的
