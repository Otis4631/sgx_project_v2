import os


file = os.listdir("../App/include")
tmp = "#pragma once\n\n"
for f in file:
    name = str(f)
    tmp += '#include "{}"\n'.format(name) 
with open("../App/include/App.h", "w") as fp:
    fp.write(tmp)