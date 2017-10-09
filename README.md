# libMujs
interface from mujs, easy use(own think)

首先 要感谢 mujs 一切都是建立在这位dalao的引擎上面 小的只是作了简单的封装, 增加了几个内建函数 
增加了消息队列来确保 js的执行为单线程, 然后实现了 setTimeout setInterval等几个常用的函数, 只不过有些问题, 就对了, 因为setTimeout的lambda
没办法脱离上下文执行, 所以我增加了规则, lambda要写成字串 等我跟原作者沟通后看怎么改为标准
