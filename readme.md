# TinyWebServer

>本项目主要参考以及各种资料来源:
>
>​	<<linux高性能服务器>>
>
>​	[qinguoyi/TinyWebServer: :fire: Linux下C++轻量级WebServer服务器 (github.com)](https://github.com/qinguoyi/TinyWebServer)
>
>​	

简介：本项目主要是对于linux下c++轻量Web服务器的学习，和手动首先，从0-1搭建自己的本地服务。

- 使用**线程池**+**非阻塞**socket + **epoll(LT)**+**事件处理模式（Reactor和模拟proactor)**的并发模型
- 使用**状态机**解析http请求报文，支持**GET**和**post**请求
- 使用**线程池**完成用户**登录**和**注册**功能；
- 使用webbench完成压力测试，在**proactor**模式下能支持**上万**的并发连接数据交换；

最后，对于如何搭建webserver感兴趣的同学可以着重参考前面列出的参考书目和github项目；



# 快速运行

- 服务器测试环境

  - centenOS
  - mysql版本

- 浏览器测试环境

  - FireFox

- 测试前确保数据库中存在名为tinyWebServer的库

  ```mysql
  //建立yourdb库
  create database tinyWebServer;
  
  //使用
  use database tinyWebServer;
  
  //创建user表，存储用户的账户和密码
  create table user(
      username char(50) NULL,
  	password char(50) NULL,
  );
  
  
  ```

- 清理源项目编译的结果

  ```shell
  make clean
  ```

- 编译

  ```shell
  sh ./build.sh
  ```

- 启动

  ```shell
  ./server
  ```

- 浏览器端

  ```shell
  localhost:15433
  ```

  

# 关于webbench测试

> reactor模式  LT+LT，46QPS

![image-20240301115659099](C:\Users\11482\AppData\Roaming\Typora\typora-user-images\image-20240301115659099.png)



>  proactor模式 LT+LT ，17072 QPS

![image-20240301124702776](C:\Users\11482\AppData\Roaming\Typora\typora-user-images\image-20240301124702776.png)