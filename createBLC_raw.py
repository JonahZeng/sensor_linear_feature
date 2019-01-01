import numpy as np
#import matplotlib.pyplot as plt
import os

iso_list = [50, 100, 200, 400, 800, 1600, 3200] 
loc = 256
for idx, iso in enumerate(iso_list):
    a = np.random.randn(2976, 3968)*(2*idx) + loc
    a = a + 0.5
    pos = a<0
    a[pos] = 0;
    pos = a>4095 #12bit raw
    a[pos] = 4095
    a = np.asarray(a, dtype=np.uint16)
    #f = plt.figure(0)
    #ax = f.add_subplot(111)
    #ax.hist(a, 40)
    a.tofile('./raw_file/IMX386DUALHYBRID_SU20181101_155620777587_FID_8260600c58180058610100000_EI_003000s_17920_ISO_%05d_WBOTP_8260_600c_5818_LV_8_id_1.raw'%iso)
os.system('pause')