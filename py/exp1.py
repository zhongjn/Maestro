import torchvision
import torch
import torch.jit
import time

ts = []
module = torch.jit.load("../nn/rgnn.pt")

for n in [1, 2, 4, 8, 16, 32, 64, 128]:
    print("batch ", n)
    data = torch.randn(n, 2, 15, 15).cuda()
    time_start = time.time()
    for i in range(1000):
        module(data)
    time_end = time.time()
    ts.append(float(time_end - time_start) / 1000)

print(ts)
