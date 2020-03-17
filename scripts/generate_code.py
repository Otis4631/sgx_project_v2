import string
import os
import re

APP_SRC = "../App/src"
ENCALVE_SRC = "../Enclave/src"



def c_function_regex(function_name): 
#     reg = r"""(\w[a-z]*)* # 声明
#     \s+%s # 函数名 reg
#    # \s*\(.*\) # 参数
#     \s*\{\d*\} # 函数体
#     """ % function_name

    reg = r"\w*\s+(%slayer)\s*\(.*\)\s*\{.*\}" % function_name
    return reg

class EDL_Generator:
    def __init__(self):
        self.code = string.Template("""""")


class App_Searcher:
    def __init__(self):
        self.app_files = os.listdir(APP_SRC)
        for file in self.app_files:
            try:
                suffix = file.split(".")[1]
            except:
                suffix = None
            if(suffix != "c" and suffix != "cpp"):
                continue
            print(file)
            res = self.search_forward_function("{}/{}".format(APP_SRC, file))
            if(res):
                break
            
    def search_forward_function(self, file_path):
        with open(file_path) as fp:
            code = str(fp.read())
            reg = c_function_regex(r"forward\w+")
            prog = re.compile(reg, re.S | re.I)
            print(prog)
            result = prog.findall(code)
            for r in result:
                print(r)
            return result


s = App_Searcher()