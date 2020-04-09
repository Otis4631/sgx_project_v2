#include <iostream>

#include "client_all.h"
using namespace std;

int main(int argc, char *argv[]) {
    src::severity_logger< severity_level > lg;
    BOOST_LOG_NAMED_SCOPE("Main");

    std::map<string, string> m;
    if(parse_cmd_line(argc, argv, m) != 0) {
        BOOST_LOG_SEV(lg, Log_Error) << "failed to parse command line args";
        return -1;
    }

    LOG_DEBUG(lg) << "successfully parse command line args";
    bpt::ptree config;


    if(parse_config(m["config"], config) != 0) {
        LOG_ERROR(lg) << "failed to parse configure file";
        return -1;
    }
    string log_path = config.get<string>("common.log_dir", "client_logs");
    init_log(log_path);

    shared_ptr<Client> client = make_shared<Client>(config);
    client->run();
    return 0;
}


