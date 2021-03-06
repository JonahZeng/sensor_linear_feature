BLC&NLC calibration tool user guide
------------------------------------------
## 总览
1. 原则：尽可能的提供多的功能，同时尽可能减少交互操作，数据存储前提供给用户肉眼检查确认；
2. 本工具使用windows 7 + Qt 5.11.1 + python3.5.4 + numpy + scipy开发，可提供完整的运行文件和链接库，也可由用户自行安装python环境和Qt runtime env，UI部分100%使用C++开发，raw数据后台操作一部分是C++， 一部分是C++调用python+numpy+scipy，响应速度和运行效率相比cas套件来说，更占优势；
3. 因为Qt和python都是跨平台的，所以理论上讲，本工具具备跨平台能力；

工具主界面：

![mainwindow](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/mainwindow.png?raw=true)

主界面只有5个操作菜单和2个about：

![open_menu](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/about_menu.png?raw=true)

这个菜单用于打开BLC&NLC raw文件，弹出子功能对话框；

![edit_menu](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/edit_menu.png?raw=true)

edit菜单用于控制图片显示缩放；

![edit_menu](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/about_menu.png?raw=true)

在关于菜单中，可以查看Qt版本信息和一些注意事项；

## BLC标定功能
### step 1
使用菜单打开文件对话框，选择blc raw

![open_blc_raw](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/blc_menu.png?raw=true)
![select_blc_raw](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/blc_menu.png?raw=true)
### step 2
工具侦测到选择的是raw文件，弹出对话框，如果是4:3的16bit raw，这里会自动计算出长宽，只需要输入bit位宽和bayer模式；如果不是这种类型的raw，则输入raw长宽和bit位宽，bayer模式，点击ok

![set_raw_info](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/set_raw_infopng.png?raw=true)
### step 3
工具开始后台计算，并给出进度：

![calc_blc_progress](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/calc_blc_progress_dlg.png?raw=true)

计算完成以后，最终的结果显示：

![blc_dlg](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/blc_dlg.jpg?raw=true)

点击“保存xml”按钮把结果保存到xml即可，如果要继续进行NLC标定，这个xml请保留给NLC标定导入；

## NLC标定功能
### step 1
使用菜单打开NLC raw:

![open_nlc_menu](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/nlc_menu.png?raw=true)

![select_nlc_raw](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/select_nlc_raw.png?raw=true)

设置raw info:
![set_raw_info](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/set_nlc_rawinfo.png?raw=true)
### step 2
为所有的raw选择选区：

![set_nlc_roi](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/nlc_roi_set.jpg?raw=true)

![sync_roi](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/sync_roipng.jpg?raw=true)

**note**:界面上显示的图像是经过downscale 1/4的，右侧的roi坐标恢复到原始raw坐标

### step 3
nlc对话框操作：

![nlc_dlg_1](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/nlc_dlg_1.jpg?raw=true)

![nlc_dlg_2](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/nlc_dlg_2.jpg?raw=true)

![nlc_dle_3](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/nlc_dlg_3.jpg?raw=true)


## 彩蛋
导出roi信息到json文件，方便python等其他程序使用；

![](https://github.com/JonahZeng/sensor_linear_feature/blob/master/image/export_roi.png?raw=true)

格式信息如下：
```json
[
    {
        "ISO": 50,
        "name": "E:/yqzeng/beijing_20180613/mate10_algo3_D65/d65_raw/IMX386DUALHYBIRD_SU20180810_173652325406_FID_006bc03861442413595301300_EI_000020s_256_ISO_50_WBOTP_c038_6144_2413_LV_85_id_0.raw",
        "roi": {
            "bottom": 1720,
            "left": 2272,
            "right": 2314,
            "top": 1610
        },
        "shutter(ms)": 50
    },
    {
        "ISO": 50,
        "name": "E:/yqzeng/beijing_20180613/mate10_algo3_D65/d65_raw/IMX386DUALHYBIRD_SU20180810_173650206741_FID_006bc03861442413595301300_EI_000030s_256_ISO_50_WBOTP_c038_6144_2413_LV_85_id_0.raw",
        "roi": {
            "bottom": 1720,
            "left": 2272,
            "right": 2314,
            "top": 1610
        },
        "shutter(ms)": 33.333333333333336
    },
    ......
]
```
