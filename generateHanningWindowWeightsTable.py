import math
N=4096
for i in range(N):
    print "{0:0.7e}".format((1 - math.cos(2*math.pi*i/N))*0.5)

