import string
import os
import re

APP_SRC = "../App/src"
ENCALVE_SRC = "../Enclave/src"
EDL_SRC = "../Enclave/edls"
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
    def __init__(self):
        self.app_files = os.listdir(APP_SRC)
        self.forward_code = []
        self.forward_declaration = []
        self.backward_code = []
        self.backward_declaration = []

        for file in self.app_files:
            try:
                suffix = file.split(".")[1]
            except:
                suffix = None
            if(suffix != "c" and suffix != "cpp"):
                continue
            res = self.search_function(
                "{}/{}".format(APP_SRC, file), r"forward_\w+_layer")
            if(res):
                res = res.group(0).strip()
                self.forward_code.append(res)
                dlr = declaration_list(self.search_declaration(res).group(0) + ";")
                self.forward_declaration.append(dlr)

            res = self.search_function(
                "{}/{}".format(APP_SRC, file), r"backward_\w+_layer")
            if(res):
                res = res.group(0).strip()
                self.backward_code.append(res)
                dlr = declaration_list(self.search_declaration(res).group(0) + ";")
                self.backward_declaration.append(dlr)
    
    def code_preprocess(self, code):
        pattern1 = re.compile(r'\s*//.*')
        result = re.sub(pattern1, '', code)
        pattern2 = re.compile(r'/\*(.|\n|\s)*?\*/', re.S)
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
    def __init__(self):
        self.app_searcher = App_Searcher()

    def generate_forward(self, file_name="forward_auto_generated.edl"):
        list_tmp = self.app_searcher.forward_declaration.copy()
        self.prepare_declaration("ecall_", list_tmp)
        self.generate_edl_file(file_name, list_tmp)

    def prepare_declaration(self, prefix, declare_list):
        for i in range(len(declare_list)):
            if(prefix == "ecall_"):
                declare_list[i][0] = "public " + declare_list[i][0]
            declare_list[i][1] = prefix + declare_list[i][1]
            for j in range(len(declare_list[i][2])): # param iter
                if(not re.match(c_type, declare_list[i][2][j][0])):
                    if(re.match(r".*_layer", declare_list[i][2][j][0])):
                        declare_list[i][2][j][0] = "layer"
                    declare_list[i][2][j][0] = prefix + declare_list[i][2][j][0]



    def generate_edl_file(self, file_name, declare_list):
        path = EDL_SRC + '/' + file_name
        with open(path, "w") as fp:
            code = string.Template(
"""
// AUTO-GENERATED, DO NOT EDIT IT
enclave{
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


e = EDL_Generator()
e.generate_forward();
def enclave_code_generator():
    pass