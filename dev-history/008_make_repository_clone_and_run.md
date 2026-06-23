# 008 保证仓库克隆后可直接运行

## 问题

上一版已经能在当前电脑的 VS Code 中编译，但构建脚本仍依赖本机已有的编译器。另一台电脑即使完整克隆源码，也可能没有 MinGW，或者只有 EasyX 不支持的 UCRT MinGW，因此不能把“源码已上传”等同于“克隆后能运行”。

## 形态分析

这里有两个不同的交付目标：

1. 使用者只想查看课程设计：不应被要求先安装编译器。
2. 使用者要修改并重新编译：必须具备兼容 EasyX 的 MSVCRT MinGW。

若强迫两类使用者都先配置编译环境，会把简单的运行需求变成工具链安装问题。

## 对立转化

仓库新增 `release/easyx_red_black_tree.exe`。该文件使用 TDM-GCC 构建，并静态链接 GCC 与 C++ 运行库。使用者克隆后可以直接双击，或运行 `run_easyx.ps1`，不需要安装 Visual Studio、VS Code、Dev-C++ 或 MinGW。

需要重新编译时，`build_easyx.ps1` 不再写死单一电脑路径。它依次检查：

- 参数 `-CompilerPath`；
- 环境变量 `EASYX_GXX`；
- 常见 Dev-C++、TDM-GCC 安装目录；
-系统 `PATH` 中的 `g++.exe`。

脚本读取编译器版本信息并跳过包含 `ucrt` 的工具链，避免再次出现 `__imp___iob_func` 链接错误。

## 状态设计

- 直接运行入口：`run_easyx.cmd`，不受 PowerShell 执行策略影响。
- 预编译程序：`release/easyx_red_black_tree.exe`。
- 发行文件 SHA-256：`BE38ACBFE77DDF520AAF7464331A2F9398F4A5B31A253B779117B638237FDA93`。
- 运行时依赖：仅 Windows 系统 DLL。
- 重新构建入口：`build_easyx.ps1`。
- 编译结果：`bin/easyx_red_black_tree.exe`，不纳入 Git。

## 验证方案

1. 使用 `objdump -p` 检查发行程序导入表，确认不存在 `libgcc`、`libstdc++`、`libwinpthread` 等外部 DLL。
2. 启动发行程序并保持三秒，确认窗口进程没有异常退出。
3. 显式把 UCRT GCC 15.2 传给构建脚本，确认脚本跳过它，并自动选择 TDM-GCC 4.9.2。
4. 提交后克隆到一个新的临时目录，只通过 `run_easyx.cmd` 启动发行程序，模拟另一台干净 Windows 电脑的使用方式。
