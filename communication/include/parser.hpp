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
typedef bpt::ptree ptree;
inline int parse_cmd_line(int argc, char *argv[], map<string, string>& m)
{
    src::severity_logger< severity_level > lg;
    BOOST_LOG_NAMED_SCOPE("Cmd Parser");
    string c; 
    bpo::options_description opts("options");
    bpo::variables_map vm;
    opts.add_options()
        ("help,h", "help info")
        ("command,m", bpo::value<string>(), "main command, must be one of train, inference")
        ("config,c", bpo::value<string>(&c) -> default_value("cfg/config"), "specify the configure file path"); // 默认值

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

inline int parse_cmd_line_s(int argc, char *argv[], map<string, string>& m)
{
    src::severity_logger< severity_level > lg;
    BOOST_LOG_NAMED_SCOPE("Cmd Parser");
    string c; 
    bpo::options_description opts("options");
    bpo::variables_map vm;
    opts.add_options()
        ("help,h", "help info")
        ("config,c", bpo::value<string>(&c) -> default_value("cfg/config"), "specify the configure file path"); // 默认值
    try
    {
        store(bpo::command_line_parser(argc, argv).options(opts).run(), vm); // 分析参数
    }
    catch (boost::program_options::error_with_no_option_name &ex)
    {
        cout << opts << endl;
        BOOST_LOG_SEV(lg, Log_Error) << ex.what();
    }

    bpo::notify(vm); // 将解析的结果存储到外部变量
    if (vm.count("help"))
    {
        cout << opts << endl;
        return -1;
    }
    m["config"] = c;
    return 0;
}

inline int parse_config(const string &path, bpt::ptree &root) {
    src::severity_logger< severity_level > lg;
    BOOST_LOG_NAMED_SCOPE("Config Parser");
    boost::filesystem::path p(path);
    if(!bf::exists(p)) {
        BOOST_LOG_SEV(lg, Log_Error) << "file is non-existent：" << path;
        return -1;
    }
    bpt::ini_parser::read_ini(path, root);
    return 0;
}
