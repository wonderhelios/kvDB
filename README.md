#KVDB

## 1.简介
* 这个项目是使用C++实现的简单key-value数据库,支持字符串(String),列表(List),哈希(Hash),集合(Set),有序集合(ZSet)五种基本类型。

## 2.特性
* 使用Reactor单线程模型,利用非阻塞IO+Epoll(LT)实现网络通信
* 实现了用户态缓冲区,提高服务器反应速度
* 使用bind+function实现回调函数,基于事件驱动
* 参考Redis,实现了简单的RDB持久化

## 3.支持命令
* 字符串(String)
  * set
  * get
* 列表(List)
  * rpush
  * rpop
* 哈希(Hash)
  * hset
  * hget
  * hgetall
* 集合(Set)
  * sadd
  * smembers
* 有序集合(HSet)
  * zadd
  * zcard
  * zrange
  * zcount
  * zgetall
  
## 4.测试

  * 测试环境
    * 硬件: Intel® Core™ i9-10900K CPU @ 3.70GHz × 20 
    * 软件: Ubuntu 18.04.5 LTS
    
  * 运行截图

    ![image --basic](https://github.com/WonderLiu96/kvDB/img/img.png)

  * 测试结果

    | 请求数 | redis/set | kvDB/set | redis/get | kvDB/get |
    | :----: | :-------: | :------: | :-------: | :------: |
    |   1k   |   0.02    |   0.01   |   0.02    |   0.01   |
    |   1w   |   0.15    |   0.11   |   0.16    |   0.09   |
    |  10w   |   1.57    |   1.06   |   1.52    |   0.96   |
    |  100w  |   15.50   |  10.78   |   15.32   |   9.54   |
