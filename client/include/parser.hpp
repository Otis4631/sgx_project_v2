#pragma once

#include <string>
#include <iostream>
#include <boost/program_options.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "log.hpp"


using namespace std;
namespace bpo = boost::program_options;
namespace bf = boost::filesystem;
namespace bpt = boost::property_tree;

extern src::severity_logger< severity_level > lg;

inline int parse_cmd_line(int argc, char *argv[], map<string, string> m)
{
    BOOST_LOG_NAMED_SCOPE("Cmd Parser");
    string c; 
    bpo::options_description opts("options");
    bpo::variables_map vm;
    

    opts.add_options()
        ("help, h", "help info")
        ("command, m", bpo::value<string>(), "main command, must be one of train, inference")
        ("config, c", bpo::value<string>(&c) -> default_value("cfg/config"), "specify the configure file path"); // 默认值

    bpo::positional_options_description p;
    p.add("command", -1);
    try
    {
        store(bpo::command_line_parser(argc, argv).options(opts).positional(p).run(), vm); // 分析参数
    }
    catch (boost::program_options::error_with_no_option_name &ex)
    {
        BOOST_LOG_SEV(lg, Log_Error) << ex.what();
    }

    bpo::notify(vm); // 将解析的结果存储到外部变量
    if (vm.count("help"))
    {
        cout << opts << endl;
        return -1;
    }
    
    if(vm.count("command")) {
        m["command"] = vm["command"].as<string>();
    }
    else {
        BOOST_LOG_SEV(lg, Log_Error) << "command not set" ;
        cout << opts << endl;
        return -1;
    }

    m["config"] = c;
    return 0;
}

inline int parse_config(string &path, bpt::ptree &root) {
    BOOST_LOG_NAMED_SCOPE("Config Parser");
    if(!bf::exists(path)) {
        BOOST_LOG_SEV(lg, Log_Error) << "error on opening configure file";
        return -1;
    }
    bpt::ini_parser::read_ini(path, root);

    int min_server = 2, min_data = 1, min_common = 1;

    if(root.count("server") < min_server || root.count("data") < min_data || root.count("common") < min_common) {
        LOG_ERROR << "error on configure file: amount of options is insufficient";
        return -1;
    }
    return 0;
}
