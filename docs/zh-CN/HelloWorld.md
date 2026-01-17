# 如何写一个HelloWorld程序？
首先，创建一个**.xf**文件，名字不限，例如 HelloWorld.xf 双击文件用visual studio code打开，如果没有可以用记事本代替
用visual studio code/记事本打开，写入：
```
#HelloWorld {
   fn main() {
     print("HelloWorld")
   }
}
```
然后打开命令行/powershell输入
```
.\xfawac.exe HelloWorld.xf -o HelloWorld.exe
```
等待一会输入
```
.\HelloWorld.exe
```
如果成功运行，就会出现HelloWorld，那么恭喜你，学会了xfawa的HelloWorld！快去试试吧！
