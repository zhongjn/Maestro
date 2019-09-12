import os
exe_path = "../out/build/x64-Release/"
exe_name = "Exp3.exe"
mod_path = "../nn/gnn.pt"

os.chdir(exe_path)
for n in [100, 200, 500, 1000, 2000, 5000]:
    print("nsim = %d"%(n))
    os.system(r"%s %d %s"%(exe_name, n, mod_path))