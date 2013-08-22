原版2.1:    http://code.google.com/p/http-scws
scws主页:   http://www.ftphp.com/scws/index.php
修改版:     https://github.com/tangfu/http-scws

差异:
*.修复原版吐html页面时比较utf8和gbk的错误
*.仿造github api的提供api的json列表(例如不带参数访问时: http://url/)
*.兼容txt和xdb两种字典(以后缀来区分),支持多个词库
*.使用c++编写,去掉无用接口,简化代码
*.依赖库libevent,jsoncpp,scws
