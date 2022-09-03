### 构建工程
由于不能直接将jni中的代码逻辑用root用户执行，所以选择编译好可执行文件后用cmd调用的方式实现。
1、创建基础的android app工程
2、添加memtool目录，创建jni目录（必须是jni名称）
3、进入到jni目录中执行 ndk-build，执行完成编译出多个架构的执行文件memtool
4、将多个架构memtool拷贝到assets中（assets没有的话可以在app右键->new folder->assets创建）
5、打包运心app即可（app启动后会把memtool可执行文件放到/data/data/包名/executable下）


```
./memtool s 1235801 com.abc.myapplication01
./memtool f 1235802 com.abc.myapplication01 315343788
./memtool e 123 com.abc.myapplication01 315343788
```