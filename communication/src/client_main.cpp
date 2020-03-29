#include <iostream>

#include "client_all.h"
using namespace std;


// int main(int argc, char *argv[]) {
//     init_log();
//     BOOST_LOG_NAMED_SCOPE("Main");

//     std::map<string, string> m;
//     if(parse_cmd_line(argc, argv, m) != 0) {
//         BOOST_LOG_SEV(lg, Log_Error) << "failed to parse command line args";
//         return -1;
//     }
//     LOG_DEBUG << "successfully parse command line args";


//     bpt::ptree config;
//     if(parse_config(m["config"], config) != 0) {
//         LOG_ERROR << "failed to parse configure file";
//         return -1;
//     }
//     cout << config.get<string>("server.addr");
//     return 0;
// }

class A {
    public:
        A(){cout << "fuck you\n";}
        A(int a){cout << "fuck you too\n";}
        void f() {cout << "member function\n";}
};

int main(){
    src::severity_logger< severity_level > lg;

    init_log();
    string addr = "127.0.0.1";
    int port = 2333;
    string uid = "ajsd";
    LOG_NAME("Main");
    shared_ptr<ClientHandler> c(new ClientHandler(addr, port));
    c->start(uid);
}