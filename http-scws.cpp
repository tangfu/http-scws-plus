#include "http-scws.hpp"

using namespace std;

static void help(void)
{
    const char *b = "HTTP-SCWS v" VERSION " written by Yunwei Wu (http://blog.ddidd.com)\n"
              "\n"
              "-l <ip_addr>    interface to listen on, default is 0.0.0.0\n"
              "-p <num>        TCP port number to listen on (default: 2011)\n"
              "-f <path>       set dict files (example: word.txt)\n"
              "-a <path>       add dict files (example: word.txt or word.xdb)\n"
              "-t <second>     timeout for an http request (default: 60)\n"
              "-c <utf-8/gbk>  set character encoding (utf-8 or gbk default:utf-8)\n"
              "-r <path>       set rules file (example: rules.ini)\n"
              "-d              run as a daemon\n"
              "-h              print this help and exit\n"
              "\n";
    fprintf(stderr, b, strlen(b));
}

void parseHttpParam(struct evhttp_request *req, struct httpParam &hp)
{
    struct evkeyvalq httpQuery;
    evhttp_parse_query(evhttp_request_get_uri(req), &httpQuery);

    const char *debug = evhttp_find_header(&httpQuery, "debug");
    if(debug != NULL) {
        hp.debug = atoi(debug);
    }

    const char *ignore = evhttp_find_header(&httpQuery, "ignore");
    if(ignore != NULL) {
        hp.ignore = atoi(ignore);
    }

    const char *multi = evhttp_find_header(&httpQuery, "multi");
    if(multi != NULL) {
        hp.multi = atoi(multi);
    }

    const char *duality = evhttp_find_header(&httpQuery, "duality");
    if(duality != NULL) {
        hp.duality = atoi(duality);
    } 

    const char *gettype = evhttp_find_header(&httpQuery, "get");
    if(gettype != NULL) {
        hp.gettype = atoi(gettype);
    } 

    const char *topnum = evhttp_find_header(&httpQuery, "tn");
    if(hp.gettype == HS_GET_TOPS) {
        if(topnum != NULL) {
            hp.topnum = atoi(topnum);
        }
    } 

    const char *attr = evhttp_find_header(&httpQuery, "attr");
    if(attr != NULL) {
        hp.attr.append(attr, strlen(attr));
    } 

    const char *text = evhttp_find_header(&httpQuery, "text");
    if(text != NULL) {
        hp.text.append(text, strlen(text));
    } 

    evhttp_clear_headers(&httpQuery);
}

void sendToClient(struct evhttp_request *req, Json::Value &output)
{
    struct evbuffer *buf;
    buf = evbuffer_new();

    evhttp_add_header(req->output_headers, "Server", "HTTP-SCWS/2.0");
    if(character == "utf-8") {
        evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
    } else if(character == "gbk") {
        evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=GBK");
    }

    evhttp_add_header(req->output_headers, "Connection", "close");
    evbuffer_add_printf(buf, "%s", output.toStyledString().c_str());
    evhttp_send_reply(req, HTTP_OK, "OK", buf);

    evbuffer_free(buf);
}

void getParamJson(struct httpParam &hp, Json::Value &output)
{
    output["status"]["msg"] = "OK";

    for(size_t i = 0; i < dictFile.size(); ++i) {
        output["result"]["dict"].append(dictFile[i]);
    }
    output["result"]["rules"] = rulesFile;
    output["result"]["character"] = character;
    output["result"]["ignore"] = hp.ignore;
    output["result"]["multi"] = hp.multi;
    output["result"]["duality"] = hp.duality;
    output["result"]["gettype"] = hp.gettype;
    output["result"]["topnum"] = hp.topnum;
    output["result"]["attr"] = hp.attr;
    output["result"]["text"] = hp.text;
    output["result"]["version"] = VERSION;
}

void getTops(struct httpParam &hp, Json::Value &output)
{
    char runTime[100];
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    output["status"]["msg"] = "OK";
    output["status"]["type"] = "top fenci";

    scws_send_text(httpscws, hp.text.c_str(), hp.text.size());
    scws_top_t top, xtop;
    if((top = xtop = scws_get_tops(httpscws, hp.topnum, const_cast<char *>(hp.attr.c_str()))) != NULL) {
        while(xtop != NULL) {
            Json::Value oValue;
            oValue["word"] = xtop->word;
            oValue["weight"] = xtop->weight;
            oValue["attr"] = xtop->attr;
            output["result"].append(oValue);
            xtop = xtop->next;
        }
        scws_free_tops(top);
    }

    gettimeofday(&t2, NULL);
    sprintf(runTime, "%f sec", (t2.tv_sec - t1.tv_sec) + (float)(t2.tv_usec - t1.tv_usec) / 1000000);
    output["status"]["time"] = runTime;
}

void getWord(struct httpParam &hp, Json::Value &output, bool flag)
{
    char runTime[100];
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    output["status"]["msg"] = "OK";
    output["status"]["type"] = "attr fenci";

    scws_send_text(httpscws, hp.text.c_str(), hp.text.size());

    if(flag) {
        scws_top_t top, xtop;
        if((top = xtop = scws_get_words(httpscws, const_cast<char *>(hp.attr.c_str()))) != NULL) {
            while(xtop != NULL) {
                Json::Value oValue;
                oValue["word"] = xtop->word;
                oValue["weight"] = xtop->weight;
                oValue["attr"] = xtop->attr;
                output["result"].append(oValue);
                xtop = xtop->next;
            }
            scws_free_tops(top);
        }
    } else {
        output["status"]["type"] = "judge word(attr)";
        if(scws_has_word(httpscws, const_cast<char *>(hp.attr.c_str())) == 1) {
            output["result"]["status"] = "1";
        } else {
            output["result"]["status"] = "0";
        }
    }

    gettimeofday(&t2, NULL);
    sprintf(runTime, "%f sec", (t2.tv_sec - t1.tv_sec) + (float)(t2.tv_usec - t1.tv_usec) / 1000000);
    output["status"]["time"] = runTime;
}

void getResult(struct httpParam &hp, Json::Value &output)
{
    char runTime[100];
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    output["status"]["msg"] = "OK";
    output["status"]["type"] = "simple fenci";

    scws_send_text(httpscws, hp.text.c_str(), hp.text.size());
    scws_top_t top, xtop;
    scws_res_t res, cur;
    while(res = cur = scws_get_result(httpscws)) {
        while(cur != NULL) {
            Json::Value oValue;
            string temp;
            temp.assign(hp.text, cur->off, cur->len);
            oValue["word"] = temp;
            oValue["idf"] = cur->idf;
            oValue["attr"] = cur->attr;
            output["result"].append(oValue);
            cur = cur->next;
        }
        scws_free_result(res);
    }

    gettimeofday(&t2, NULL);
    sprintf(runTime, "%f sec", (t2.tv_sec - t1.tv_sec) + (float)(t2.tv_usec - t1.tv_usec) / 1000000);
    output["status"]["time"] = runTime;
}

void doFenci(struct httpParam &hp, Json::Value &output)
{
    scws_set_multi(httpscws, hp.multi << 12);
    scws_set_duality(httpscws, hp.duality);
    scws_set_ignore(httpscws, hp.ignore);
    switch(hp.gettype) {
        case HS_GET_TOPS:
            getTops(hp, output);
            break;
        case HS_GET_HAS_WORD:
            getWord(hp, output, false);
            break;
        case HS_GET_WORD:
            getWord(hp, output, true);
            break;
        default:
            getResult(hp, output);
            break;
    }
}

void requestHandler(struct evhttp_request *req, void *arg)
{
    struct httpParam hp;
    Json::Value output;

    parseHttpParam(req, hp);

    if(hp.debug == 1) {
        getParamJson(hp, output);
    } else {
        doFenci(hp, output);
    }

    sendToClient(req, output);
}

void helpHandler(struct evhttp_request *req, void *arg)
{
    struct httpParam hp;
    Json::Value output;
   
    Json::Value temp1;
    temp1["supported method"] = "get, post";
    Json::Value temp2;
    temp2["param"]["debug"]["1"] = "显示参数信息";
    temp2["param"]["ignore"]["0"] = "不忽略";
    temp2["param"]["ignore"]["1"] = "忽略标点符号(默认值)";
    temp2["param"]["ignore"]["0"] = "不进行复合切分(默认值)";
    temp2["param"]["ignore"]["1"] = "复合短语切分";
    temp2["param"]["ignore"]["2"] = "复合二元切分";
    temp2["param"]["ignore"]["4"] = "复合主要单词切分";
    temp2["param"]["ignore"]["8"] = "复合全部单词切分";
    temp2["param"]["duality"]["0"] = "不进行聚合(默认值)";
    temp2["param"]["duality"]["1"] = "闲散单字进行二元聚合";
    temp2["param"]["gettype"]["0"] = "简单分词(默认值)";
    temp2["param"]["gettype"]["1"] = "获取高频词";
    temp2["param"]["gettype"]["2"] = "判断是否有指定词性的词";
    temp2["param"]["gettype"]["3"] = "获取指定词性的词";
    temp2["param"]["topnum"] = "高频词数(默认值10)";
    temp2["param"]["attr"] = "指定属性(默认空)";
    temp2["param"]["text"] = "待分词的文本(默认空)";

    output.append(temp1);
    output.append(temp2);
    sendToClient(req, output);
}

void apiHandler(struct evhttp_request *req, void *arg)
{
    struct httpParam hp;
    Json::Value output;
    
    output["fenci_url"] = "http://domain url/fenci";
    output["help_url"] = "http://domain url/help";

    sendToClient(req, output);
}

void unknownHandler(struct evhttp_request *req, void *arg)
{
    struct httpParam hp;
    Json::Value output;
    output["message"] = "Not Found Api";
    sendToClient(req, output);
}

int main(int argc, char **argv)
{
    int i;
    string ip = "0.0.0.0";
    int port = 2011;        
    bool daemon = false;
    int timeout = 60; 

    while((i = getopt(argc, argv, "l:p:f:a:r:t:c:dh")) != -1) {
        switch(i) {
            case 'l': {
                ip = optarg;
                break;
            }
            case 'p': {
                port = atoi(optarg);
                break;
            }
            case 'f': {
                dictFile.push_back(optarg);
                break;
            }
            case 'a': {
                dictFile.push_back(optarg);
                break;
            }
            case 'r': {
                rulesFile = optarg;
                break;
            }
            case 't': {
                timeout = atoi(optarg);
                break;
            }
            case 'c': {
                character = optarg;
                break;
            }
            case 'd': {
                daemon = true;
                break;
            }
            case 'h':
            default: {
                help();
                return 1;
            }
        }
    }

    //检查编码
    if(!character.compare("utf-8") && !character.compare("gbk")) {
        help();
        fprintf(stderr, "Attention: Please Set Character Encoding Argument: -c <utf-8/gbk>\n\n");
        exit(1);
    }

    //初始化scws
    httpscws = scws_new();
    if(httpscws == NULL) {
        fprintf(stderr, "ERROR: Can Not Init\n\n");
        exit(1);
    }

    //设置编码
    scws_set_charset(httpscws, character.c_str());

    //加载规则文件
    if(rulesFile.empty()) {
        fprintf(stderr, "must assign rulesFile");
    } else {
        fprintf(stderr, "Loading rules file '%s', please waiting ......\n", rulesFile.c_str());
        scws_set_rule(httpscws, rulesFile.c_str());
    }
    //加载字典文件到内存
    if(dictFile.empty()) {
        fprintf(stderr, "dictFile empty");
    } else {
        fprintf(stderr, "Loading dict file '' into memory, please waiting ......\n");
        for(vector<string>::iterator it=dictFile.begin(); it != dictFile.end(); ++it) {
            fprintf(stderr, "%s\n", it->c_str());
        }
        int iRet = 0;
        for(vector<string>::iterator it=dictFile.begin(); it != dictFile.end(); ++it) {
            if(it->rfind(".txt") == string::npos) {
                iRet = scws_add_dict(httpscws, it->c_str(), SCWS_XDICT_XDB | SCWS_XDICT_MEM);
            } else {
                iRet = scws_add_dict(httpscws, it->c_str(), SCWS_XDICT_TXT | SCWS_XDICT_MEM);
            }
        }
        if(iRet != 0) {
            fprintf(stderr, "set dict error\n");
            exit(1);
        }
        printf("OK! dict file has loaded into memory.\n\n");
    }


    printf("Listen:%s:%d\n", ip.c_str(), port);
    printf("Timeout:%ds\n", timeout);
    printf("Character:%s\n", character.c_str());
    for(size_t i = 0; i < dictFile.size(); ++i) {
        printf("Dict File:%s\n", dictFile[i].c_str());
    }
    printf("Rules File:%s\n", rulesFile.c_str());

    //后台运行
    if(daemon == true) {
        pid_t pid;

        pid = fork();
        if(pid < 0) {
            exit(EXIT_FAILURE);
        }

        if(pid > 0) {
            exit(EXIT_SUCCESS);
        }
    }

    struct evhttp *httpd;

    event_init();
    httpd = evhttp_start(ip.c_str(), port);
    if(httpd == NULL) {
        fprintf(stderr, "ERROR: Can Not Start Server\n\n");
        exit(1);
    }

    evhttp_set_timeout(httpd, timeout);
    evhttp_set_cb(httpd, "/fenci", requestHandler, NULL);
    evhttp_set_cb(httpd, "/help", helpHandler, NULL);
    evhttp_set_cb(httpd, "/", apiHandler, NULL);
    evhttp_set_gencb(httpd, unknownHandler, NULL);
    event_dispatch();
    evhttp_free(httpd);

    return 0;
}
