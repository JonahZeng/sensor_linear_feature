# V300 BLC&NLC calibration tool user guide
------------------------------------------
## 总览
1. 原则：尽可能的提供多的功能，同时尽可能减少交互操作，数据存储前提供给用户肉眼检查确认；
2. 本工具使用windows 7 + Qt 5.11.1 + python3.5.4 + numpy + scipy开发，提供完整的运行文件和链接库，不需要安装任何运行时环境，且UI部分100%使用C++开发，raw数据后台操作一部分是C++， 一部分是C++调用python+numpy+scipy，响应速度和运行效率相比cas套件来说，更占优势；
3. 因为Qt和python都是跨平台的，所以理论上讲，本工具具备跨平台能力；

工具主界面：

![mainwindow](https://raw.githubusercontent.com/JonahZeng/sensor_linear_feature/master/image/mainwindow.PNG)

主界面只有5个操作菜单和2个about：

![open_menu](https://raw.githubusercontent.com/JonahZeng/sensor_linear_feature/master/image/open_menu0.png)

这个菜单用于打开BLC&NLC raw文件，弹出子功能对话框；

![edit_menu](https://raw.githubusercontent.com/JonahZeng/sensor_linear_feature/master/image/open_menu.png)

edit菜单用于控制图片显示缩放；

![edit_menu](https://raw.githubusercontent.com/JonahZeng/sensor_linear_feature/master/image/about_menu.png)

在关于菜单中，可以查看Qt版本信息和一些注意事项；

## BLC功能
## step 1
## step 2
## step 3



