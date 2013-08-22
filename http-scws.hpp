#include <stdlib.h>
#include <string.h>
#include <event.h>
#include <evhttp.h>
#include <json/value.h>
#include <scws/scws.h>
#include <vector>
#include <string>

#define VERSION "2.0"


//忽略标点等特殊符号, 不忽略标点等特殊符号
enum SCWS_IGNORE_HS { HS_SYMBOL_IGNORE=1, HS_SYMBOL_NO_IGNORE=0 };

// 不复合分词，复合短语分词，符合二元分词，复合主要单字分词，复合全部单字分词
enum SCWS_MULTI_HS { HS_MULTI_NONE=SCWS_MULTI_NONE, HS_MULTI_SHORT=SCWS_MULTI_SHORT, HS_MULTI_DUALITY=SCWS_MULTI_DUALITY,
    HS_MULTI_ZMAIN=SCWS_MULTI_ZMAIN, HS_MULTI_ZALL=SCWS_MULTI_ZALL };

//闲散文字是否执行二分聚合
enum SCWS_DUALITY_HS { HS_DUALITY=1, HS_NO_DUALITY=0 };

// 0:简单分词 1:获取高频词 2:是否有指定词性的词 3:获取指定词性的词
enum SCWS_WORDS_HS { HS_GET_RESULT=0, HS_GET_TOPS=1, HS_GET_HAS_WORD=2, HS_GET_WORD=3 };


#define HS_ATTR "*"				//词性
#define HS_TEXT ""

scws_t httpscws;
std::vector<std::string> dictFile;    /*字典文件*/
std::string rulesFile;
std::string character = "utf-8";

struct httpParam
{
    httpParam() : 
        debug(0), ignore(HS_SYMBOL_IGNORE), multi(HS_MULTI_NONE), duality(HS_NO_DUALITY),
        gettype(HS_GET_RESULT), topnum(10) {}; 
	int debug;		//测试用
	int ignore;		//符号
	int multi;		//复合切分
	int duality;	//闲散文字二分聚合
	int gettype;	//分词类型
	int topnum;		//高频词数
    std::string attr;    //词性
    std::string text;    //待分词文本
};
