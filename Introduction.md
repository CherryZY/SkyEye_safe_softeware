
项目总开发模块：

	·UCONabc数据库搭建（新型访问控制模型搭建数据库)，
	·SFilter文件过滤驱动（按照文件绝对路径对文件进行执行，读，写，加密权限操作)，
	·NDIS网络驱动（按照IP,Protocol,Port,URL,MAC等对数据进行过滤)，
	·WFP网络驱动（按照用户进程号和进程名对网络数据进行过滤)，
	·SSDT用户进程保护驱动（按照提前提供的App进程名，对固定进程进行权限保护)，
	·桌面RW数据库应用程序（依照不同用户需要对某文件,网址,IP,端口赋予一定权限并写入数据库).

安装说明：

  软件运行测试平台：windows 7 x86

  开发平台：windows 8.1 x64（需wdk8.1+VS2013） & windows 7 x64

  开发工具：QtCreator + WDK + VS2013 + VritualBox

  1.使用mysql,创建数据库，名为ucon,然后导入表(/uconsql/ucon.sql),可以使用目录下的ucon文件进行。

  2.打开Cp文件夹，先安装CredentialProvider,再安装CredentialProviderFilter(注：分别将libmysql.dll，MCredentialProvider.dll，MCredentialProviderFilter.dll放入c:/windows/system32目录下分别双击Register.reg和RegisterF.reg进行安装)

  3.双击LoadDriver.exe,进行驱动安装(注：NDIS驱动需要单独安装，打开网络和共享中心->更改适配器设置->以太网->右键->安装->服务->寻找NDIS驱动所在目录并进行安装)

  4.启动SkyEye程序,第一次启动需要注册信息，dbname是数据库名，输入”ucon“，完成后重启;

  注：1.安装前关闭杀软！！！  
      2.开机凭据有时候会输入正确密码和正确账户不能正确登陆，此处有bug，请多试几次。
      3.NDIS驱动在/Ndis目录下
      4.开机出现猫头鹰界面，若点击Introduction请记得点击对话框按钮关闭,此处有bug，若出现其它问题请重启。
      5.因为该软件是两个模块的结合体，文件系统驱动和网络系统驱动互相交互通过pipe传递信息，会出现bug。但是各自单独运行都不会有问题。

--------------------------------------------------------------------------

·关于sfilter project （R0文件系统驱动+R3应用）出现的不能编译等问题请联系：

	开发者（steven.du）：2544661922（QQ）

·关于其它压缩文件出现的问题请联系：

	开发者（cherry.zhang）：591748932（QQ）
