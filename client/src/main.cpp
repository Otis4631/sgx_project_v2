#include "parser.hpp"
#include "log.hpp"
#include <iostream>

#include "all.h"

using namespace std;


src::severity_logger< severity_level > lg;

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



int main(){

}