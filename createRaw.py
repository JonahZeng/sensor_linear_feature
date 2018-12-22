import numpy as np
#import matplotlib.pyplot as plt
import os

time_list = [0, 4000, 2000, 1000, 800, 750, 640, 500, 400, 320, 250, 200, 160, 100, 80, 40, 32, 20, 10]
loc = 1024
for time in time_list:
    if(time==0):
        a = np.random.randn(2976, 3968)*8 + loc
    else:
        a = np.random.randn(2976, 3968)*8 + 2000/time + loc
    a = a + 0.5
    pos = a<0
    a[pos] = 0;
    pos = a>4095 #12bit raw
    a[pos] = 4095
    a = np.asarray(a, dtype=np.uint16) - 768
    #f = plt.figure(0)
    #ax = f.add_subplot(111)
    #ax.hist(a, 40)
    a.tofile('./raw_file/IMX386DUALHYBRID_SU20181101_155620777587_FID_8260600c58180058610100000_EI_%06ds_17920_ISO_3500_WBOTP_8260_600c_5818_LV_8_id_1.raw'%time)
os.system('pause')
