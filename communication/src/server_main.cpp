

#include "server_all.h"

int main(int argc, char** argv) {

    init_log("server_logs");
    LOG_NAME("Main");
    src::severity_logger<log_level> log;
    ptree config;
    map<string, string> m;
    parse_cmd_line_s(argc, argv, m);

    string path = m["config"];

    if(parse_config(path, config) != 0){
        LOG_ERROR(log) << "Error occurred when parse config file: " ;
        return -1;

    }
    LOG_DEBUG(log) << "Parse config file successfully";
    shared_ptr<Server> server(new Server(config));
    server->run();


}