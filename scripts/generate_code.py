import string
import os
import re

BASE = "/data/lz/sgx_project_v2"
APP_SRC = BASE + "/App/src"
ENCALVE_SRC = BASE + "/Enclave/src"
EDL_SRC = BASE + "/Enclave/edls"
c_type = r"(int|void|long|float|double)"

class declaration_list(list):
    def __init__(self, l):
        if(type(l) == str):
            nl = []
            sped = l.split(" ", 1)
            nl.append(sped[0])
            lindex = sped[1].index("(")
            nl.append(sped[1][:lindex].strip())
            param = sped[1][lindex + 1:len(sped[1]) - 2]
            param = param.split(",")
            for i in range(len(param)):
                foo = param[i].split(" ")
                j = 0
                while(j < (len(foo))):
                    if(foo[j] == "" or foo[j] == "const"):
                        foo.pop(j)
                        j -= 1
                    j += 1
                assert(len(foo) == 2) 
                param[i] = foo
                
            nl.append(param)
            list.__init__(self, nl)
        else:
            assert(type(l[0]) == str and type(l[1]) == str and type(l[2]) == list)
            list.__init__(self, l)

    def __str__(self):
        param = ""
        for i, p in enumerate(self[2]):
            param += p[0]
            param += " "
            param += p[1]
            param += ", "
        param = list(param)
        param.pop()
        param.pop()


        param = "".join(param)
        foo = "{} {}({});".format(self[0], self[1], param)
        return foo
        
def c_function_regex(function_name):
    #     reg = r"""(\w[a-z]*)* # 声明
    #     \s+%s # 函数名 reg
    #    # \s*\(.*\) # 参数
    #     \s*\{\d*\} # 函数体
    #     """ % function_name
    reg = r"""%s\s+%s\s*\(.*\)(\s*|\n*)\{(.|\n|\r|\v)*?\n*((\}(\n|\s)*(?=(%s\s+\w+\(.*\))))|(\}(#\w)*$))""" % (
        c_type, function_name, c_type)
    return reg


# declare_list:
    # [0] return type
    # [1] function name
    # [2] param:
    #   [...]



class App_Searcher:
    def __init__(self, func_name):
        self.function_name = func_name
        self.app_files = os.listdir(APP_SRC)
        self.code = []
        self.code_splited = []
        self.declaration = []


        for file in self.app_files:
            try:
                suffix = file.split(".")[1]
            except:
                suffix = None
            if(suffix != "c" and suffix != "cpp"):
                continue
            res = self.search_function(
                "{}/{}".format(APP_SRC, file), func_name)
            if(res):
                res = res.group(0).strip()
                self.code.append(res)
                dlr = declaration_list(self.search_declaration(res).group(0) + ";")
                body = re.search(r"\{(.|\n)*\}", res).group(0)
                self.code_splited.append([dlr, body])
                self.declaration.append(dlr)
    
    def code_preprocess(self, code):
        pattern1 = re.compile(r'\s*//(?!sizedefination).*')
        result = re.sub(pattern1, '', code)
        pattern2 = re.compile(r'/\*(?!sizedefination)(.|\n|\s)*?\*/', re.S)
        result = re.sub(pattern2, '', result)
        return result.strip()

    def search_function(self, file_path, func_name):
        with open(file_path) as fp:
            code = self.code_preprocess(str(fp.read()))
            reg = c_function_regex(func_name)
            prog = re.compile(reg, re.I)
            result = prog.search(code)
        return result

    def search_declaration(self, code):
        reg = r"%s\s+\w+\s*\((\w|,|\s)*\)" % (c_type)
        prog = re.compile(reg, re.I | re.X)
        res = prog.search(code)
        return res


# def edl_generator():
#     app_searcher = App_Searcher()
#     list_tmp = app_searcher.forward_declaration.copy()
#     prepare_declaration("ecall_", list_tmp)
#     generate_edl_file("auto_forward.edl", list_tmp)

class EDL_Generator:
    def __init__(self, file_prefix, declaration):
        self.declaration = declaration
        self.prefix = file_prefix
        self.file_name = self.prefix + "_auto_generated.edl"

    def generate(self, _type = "ecall_"):
        self.edlify_declaration = self.declaration.copy()
        self.prepare_declaration(_type, self.edlify_declaration)
        self.generate_edl_file(self.file_name, self.edlify_declaration)

    def prepare_declaration(self, prefix, declare_list):
        for i in range(len(declare_list)):
            if(prefix == "ecall_"):
                declare_list[i][0] = "public " + declare_list[i][0]
            declare_list[i][1] = prefix + declare_list[i][1]
            for j in range(len(declare_list[i][2])): # param iter
                if(not re.match(c_type, declare_list[i][2][j][0])):
                    if(re.match(r".*_layer", declare_list[i][2][j][0])):
                        declare_list[i][2][j][0] = "layer"
                    declare_list[i][2][j][0] = "[in, count = 1] struct " + prefix + declare_list[i][2][j][0] + "*"


    def generate_edl_file(self, file_name, declare_list):
        path = EDL_SRC + '/' + file_name
        with open(path, "w") as fp:
            code = string.Template(
"""
// AUTO-GENERATED, DO NOT EDIT IT
enclave{
    from "types.edl" import *;
    trusted{
        $ecall_functions
    };
    untrusted{
        $ocall_functions
    };
};
""")
            data = {
                "ecall_functions": "\n\t".join([str(d) for d in declare_list]),
                "ocall_functions":""
            }
            code = code.safe_substitute(**data)
            fp.write(code)


def enclave_code_generator(prefix, code_splited, edlify_declaration):
    file_name = prefix + "_auto_generated.cpp"
    code_t = """
#include "enclave.h"
    """ 
    with open(ENCALVE_SRC + "/" + file_name, "w") as fp:
        for i, code in enumerate(code_splited):
            n_code = ""
            edlify_declaration[i][0] = edlify_declaration[i][0].replace("public ", "")
            edlify_declaration[i][0] = edlify_declaration[i][0].replace("\t", "")
            declaration = str(edlify_declaration[i]).replace(";", "")
            n_code = declaration + code[1]
            code_t += (n_code + "\n")
        fp.write(code_t)

            
def ecall_warpper_generator(file , code_splited):
    for declaration, body in code_splited:
        prog = re.compile(r"(?<=l\.)\w+")
        layers_element = set(prog.findall(body))
        net_element = set(re.findall(r"(?<=net\.)\w+", body))
        tmp = ""
        tmp2 = ""
        for elm in layers_element: # layer
            size = re.search(r"(?<=(%s_len=))\s*((\w|.))+?;" % elm, body)
            tmp += "nl.{0} = l.{0}; \n".format(elm)
            if(size):
                tmp += "nl.{}_len = {}\n".format(elm, size)
        
        for enm in net_element:

            size = re.search(r"(?<=(%s_len=))\s*((\w|.))+?;" % elm, body)
            tmp2 += "en.{0} = net.{0}; \n".format(elm)
            if(size):
                tmp2 += "en.{}_len = {}\n".format(elm, size)

        t = string.Template("""
int e_$function_name(layer l, network net) {
    ecall_layer nl;
    ecall_network en;
    $ecall_layer
    $ecall_network
    sgx_status_t ret = ecall_$function_name(&el, &en);
    if(ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }
}
""")
        data = {
            "function_name": declaration[1],
            "ecall_layer": tmp,
            "ecall_network":tmp2
    }
        code_t = t.safe_substitute(**data)
        print(code_t)


forward = App_Searcher(r"forward_\w+_layer")
ecall_warpper_generator("e_forward", forward.code_splited)
e = EDL_Generator("forward", forward.declaration)
e.generate();
enclave_code_generator("forward", forward.code_splited, e.edlify_declaration)