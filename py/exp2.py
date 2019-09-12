import os
exe_path = "../out/build/x64-Release/"
exe_name = "Exp2.exe"
mod_path = "../nn/gnn.pt"

os.chdir(exe_path)
os.system(r"%s 0 %s"%(exe_name, mod_path))
os.system(r"%s 1 %s"%(exe_name, mod_path))