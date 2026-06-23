# 007 从官网安装 EasyX 并配置 VS Code MinGW

## 问题

用户要求不使用 Visual Studio 2022，而是在 VS Code 中直接编译 EasyX，同时要求把参考项目的红黑树头文件原封不动复制到主项目 `src` 下使用。

## 形态分析

VS Code 只是编辑器和任务入口，真正决定能否链接 EasyX 的是背后的编译器与 C 运行时。EasyX 官网提供 MinGW 静态库，但明确说明其兼容 MSVCRT，不支持 UCRT。当前系统默认 `C:\Program Files\mingw64` 是 GCC 15.2、win32 线程、UCRT 运行时。

## 对立转化

没有把 EasyX 文件复制进系统 MinGW 目录，而是放入项目的 `third_party/easyx`，通过 `-I` 和 `-L` 显式引用。这样项目自包含，也不会污染其他 C++ 项目。

第一次使用默认 MinGW 链接时出现 `__imp___iob_func` 未定义，验证了 UCRT 不兼容。随后发现电脑已有 Dev-C++ 附带的 TDM-GCC 4.9.2。它符合 EasyX 官方支持范围，因此 VS Code 改为直接调用这套编译器，整个过程无需打开 Dev-C++。

## 状态设计

- EasyX 版本：25.9.10。
- 官方压缩包 SHA-256：`5345C113DA1DB139097D9AEF7DF2D4B414C557AEDC49D3F4407124F48B91A8FD`。
- EasyX 头文件：`third_party/easyx/include`。
- EasyX 64 位 Unicode 库：`third_party/easyx/lib64/libeasyxw.a`。
- 编译器：`C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\g++.exe`。
- C++ 标准：C++14。
- VS Code 构建入口：`Ctrl+Shift+B`。
- VS Code 调试入口：F5，配置名为 `Debug EasyX Red-Black Tree`。

## 头文件复制验证

`reference-projects/red-black-tree-template/rbtree.h` 被完整复制为 `src/rbtree.h`。复制后两者 SHA-256 均为：

```text
709DAD928802B6C084F18BE1D63B49E1068AC26379D1D01769524BBC8203CA20
```

主项目的模型头文件改为 `#include "rbtree.h"`，测试也改为验证 `src/rbtree.h`。

## 编译验证

构建脚本成功生成 `bin/easyx_red_black_tree.exe`。程序启动后持续运行，说明 EasyX 窗口初始化和静态库加载成功。测试结束后由验证脚本关闭进程。
