# 红黑树课程设计运行与源码笔记

这个仓库现在有两条互相独立的运行路线：

1. `src/main.cpp` 负责跑 39 项数据测试，用来证明红黑树插入、删除、迭代器和红黑性质没有出错。
2. `src/easyx_frontend.cpp` 负责打开 EasyX 窗口，用图形显示当前树的父子关系和颜色。

这两条路线共用 `src/rbtree.h`，但是它们不会互相调用。测试程序不需要 EasyX，EasyX 前端也不会去调用测试程序。

`reference-projects` 只是让我对照原项目用的普通文件夹。即使删掉整个目录，主项目仍然能测试、编译和运行。

## 一、克隆之后怎么直接运行

如果只是想看图形界面，不需要先安装编译器。

```powershell
git clone https://github.com/anarchycuriosity/data_structure_course_design.git
cd data_structure_course_design
.\run_easyx.cmd
```

`run_easyx.cmd` 会启动：

```text
release/easyx_red_black_tree.exe
```

这个 exe 已经把 EasyX 和 GCC 的 C++ 运行库静态链接进去了，所以另一台 64 位 Windows 10/11 电脑不需要安装 Visual Studio、VS Code、Dev-C++ 或 MinGW。

也可以直接双击：

```text
release/easyx_red_black_tree.exe
```

## 二、几个脚本分别是干什么的

### 1. `run_easyx.cmd`

这是最简单的运行入口。

```bat
@echo off
start "" "%~dp0release\easyx_red_black_tree.exe"
```

这里的 `%~dp0` 表示这个 cmd 文件自己所在的目录。

为什么不能只写：

```bat
start release\easyx_red_black_tree.exe
```

因为用户可能从别的工作目录启动脚本。相对路径会以“当前终端所在目录”为起点，而 `%~dp0` 永远指向脚本自己的位置。这样仓库放到 D 盘、桌面或者中文目录中，脚本仍然知道 exe 在哪里。

### 2. `run_easyx.ps1`

它和 `run_easyx.cmd` 作用相同，区别是使用 PowerShell 编写。

执行顺序为：

```text
找到仓库根目录
    ↓
拼出 release/easyx_red_black_tree.exe 的绝对路径
    ↓
检查文件是否存在
    ↓
用 Start-Process 启动
```

cmd 版本更适合双击，PowerShell 版本更容易继续增加文件检查和错误提示。

### 3. `build_easyx.ps1`

这个脚本负责把后端和前端编译成 EasyX 程序。

它实际编译两个源文件：

```text
src/tree_visualization_model.cpp
src/easyx_frontend.cpp
```

先写后端、再写前端只是为了让命令更符合阅读顺序。C++ 编译器最终会分别编译两个 `.cpp`，再由链接器把它们和 EasyX 静态库合成一个 exe。

编译命令里的关键参数：

| 参数 | 作用 |
| --- | --- |
| `-std=c++14` | 使用 C++14 语法 |
| `-Wall -Wextra` | 打开常见警告 |
| `-g` | 写入 GDB 调试信息 |
| `-DUNICODE -D_UNICODE` | 让 Windows API 使用宽字符版本，中文字符串不会走窄字符接口 |
| `-static-libgcc` | 把 GCC 运行库放进 exe |
| `-static-libstdc++` | 把 C++ 标准库运行部分放进 exe |
| `-I.../include` | 告诉编译器去哪里找 `graphics.h` |
| `-L.../lib64` | 告诉链接器去哪里找 EasyX 库 |
| `-leasyxw` | 链接 Unicode 版本 EasyX |
| `-lgdi32 -lole32 -luuid` | 链接 EasyX 使用的 Windows 系统库 |

脚本不能随便拿任意 MinGW 来编译 EasyX。

当前 EasyX MinGW 库使用 MSVCRT。系统中那个 GCC 15.2 使用 UCRT，强行链接会出现：

```text
undefined reference to __imp___iob_func
```

所以脚本会读取 `g++ -v` 的结果。如果发现字符串中有 `ucrt`，就跳过这个编译器，继续寻找 TDM-GCC/MSVCRT 编译器。

编译器搜索顺序为：

```text
命令行传入的 -CompilerPath
    ↓
环境变量 EASYX_GXX
    ↓
几个常见的 Dev-C++ / TDM-GCC 安装位置
    ↓
PATH 中的 g++.exe
```

手动指定编译器：

```powershell
.\build_easyx.ps1 -CompilerPath "D:\Compiler\MinGW64\bin\g++.exe"
```

或者：

```powershell
$env:EASYX_GXX = "D:\Compiler\MinGW64\bin\g++.exe"
.\build_easyx.ps1
```

编译结果位于：

```text
bin/easyx_red_black_tree.exe
```

### 4. `build_data_tests.ps1`

这个脚本只编译：

```text
src/main.cpp
```

它和 EasyX 没有关系，也不会链接 `graphics.h`。

测试代码使用了 GNU 的语句表达式宏 `({ ... })`，所以这里必须写：

```text
-std=gnu++14
```

而不是严格的：

```text
-std=c++14
```

`-DDEBUG` 会让 `rbtree.h` 编译 `invariant()`、`toString()` 等测试接口。

原测试中还有一百万个整数的栈上数组。Windows 默认栈空间不够，因此链接参数：

```text
-Wl,--stack,33554432
```

把测试程序栈空间提高到 32 MiB。如果删掉这个参数，程序可能不是算法错误，而是直接因为栈空间不足退出。

### 5. `run_data_tests.ps1`

它不是另一套测试逻辑，只是把“构建”和“运行”串起来：

```text
调用 build_data_tests.ps1
    ↓
检查 bin/red_black_tree_data_tests.exe 是否生成
    ↓
运行测试程序
    ↓
检查退出码是否为 0
```

直接运行：

```powershell
.\run_data_tests.ps1
```

正确结果最后应看到：

```text
Tests passed: 100%
```

## 三、VS Code 怎么使用这些脚本

### 编译 EasyX

按：

```text
Ctrl + Shift + B
```

VS Code 会读取 `.vscode/tasks.json`，执行默认任务：

```text
Build EasyX Red-Black Tree
```

这个任务本身不写编译参数，而是调用 `build_easyx.ps1`。这样终端手动编译和 VS Code 编译走的是同一套逻辑，不会出现两份命令慢慢变得不一致。

### 调试 EasyX

按：

```text
F5
```

`.vscode/launch.json` 会先执行 EasyX 构建任务，然后用 GDB 启动：

```text
bin/easyx_red_black_tree.exe
```

### 运行数据测试

打开命令面板：

```text
Tasks: Run Task
```

选择：

```text
Run Red-Black Tree Data Tests
```

## 四、EasyX 绘图到底是怎么工作的

EasyX 本身不认识红黑树。它只认识：

```text
坐标
颜色
直线
圆
文字
鼠标消息
```

所以必须先把“红黑树节点”翻译成“屏幕上的节点”。

完整数据流：

```text
RBTree<int>
    ↓ visualization_snapshot()
键值、颜色、父子下标
    ↓ calculate_positions()
键值、颜色、父节点下标、x、y
    ↓ draw_tree()
line() 画父子边
fillcircle() 画节点
outtextxy() 画键值
```

### 1. 为什么需要 `VisualizationNode`

`RBTreeNode` 是红黑树的私有节点，里面有真实的父子指针和颜色。

如果为了画图直接把它改成 public，前端就有能力写出：

```text
root->left = root
```

这种错误会直接破坏树。

所以 `visualization_snapshot()` 只复制绘图需要的数据：

```text
key
is_black
parent_index
left_index
right_index
```

这里用数组下标代替真实指针。前端能知道父子关系，却不能改动红黑树。

### 2. 横坐标为什么用中序遍历

二叉搜索树中序遍历的结果从小到大排列。

如果访问顺序是：

```text
3 5 7 10 12 15 18
```

就把访问序号变成横坐标：

```text
第 0 个节点在最左边
第 1 个节点在它右边
……
最后一个节点在最右边
```

这样可以自然保证：

```text
左孩子画在父亲左边
右孩子画在父亲右边
```

不需要在绘图阶段重新判断键值大小。

### 3. 纵坐标为什么用 depth

根节点深度为 0，孩子深度为 1，孙子深度为 2。

所以纵坐标可以写成：

```text
y = 绘图区顶部 + 每层高度 × depth
```

树越深，节点越靠下。

`std::max(1, max_depth)` 是为了处理只有根节点的情况。如果 `max_depth == 0` 还直接做除法，就会除以 0。

### 4. 为什么先画线再画圆

父子边使用：

```cpp
line(parent_x, parent_y, child_x, child_y);
```

这条线会一直画到圆心。

如果先画圆再画线，线会穿过圆和数字。先画线、后画圆，圆会把线靠近圆心的部分盖住，看起来就像连线停在节点边缘。

### 5. 为什么使用批量绘图

程序启动后调用：

```cpp
BeginBatchDraw();
```

每一帧最后调用：

```cpp
FlushBatchDraw();
```

如果每执行一次 `line()`、`fillcircle()` 都立刻显示到屏幕，用户会看到窗口不断被擦除、重画，产生闪烁。

批量绘图先在后台缓冲区画完整一帧，再一次显示。

### 6. 一次点击经历了什么

以插入 6 为例：

```text
getmessage() 等到鼠标左键消息
    ↓
Button::contains() 判断是否点中插入按钮
    ↓
InputBox() 读取字符串 "6"
    ↓
wcstol() 把字符串转换成整数 6
    ↓
model->insert(6)
    ↓
RBTree<int>::insert(6)
    ↓
下一轮 draw_scene()
    ↓
重新读取整棵树、重新计算坐标、重新绘制
```

删除可能引发旋转和换色，所以不能只擦掉被删除的一个圆。每次操作后重新读取并画整棵树，才能保证图形与真实结构一致。

## 五、源码阅读顺序

```text
src/rbtree.h
    ↓
src/tree_visualization_model.h
    ↓
src/tree_visualization_model.cpp
    ↓
src/easyx_frontend.cpp
```

先看红黑树提供了什么数据，再看坐标如何计算，最后看 EasyX 如何消费这些坐标。

## 六、目录说明

```text
src/rbtree.h                       主项目红黑树实现
src/main.cpp                       39 项数据测试入口
src/tree_visualization_model.h     前后端之间的数据结构和接口
src/tree_visualization_model.cpp   快照转换和坐标计算
src/easyx_frontend.cpp             EasyX 窗口、按钮、输入、绘图
third_party/easyx                  官网 EasyX MinGW 文件
release/easyx_red_black_tree.exe   克隆后可直接运行的版本
reference-projects                 只用于观看和参照
```
