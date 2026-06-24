# 红黑树课程设计

## 克隆后直接运行

环境要求：64 位 Windows 10 或 Windows 11。

```powershell
git clone https://github.com/anarchycuriosity/data_structure_course_design.git
cd data_structure_course_design
.\run_easyx.cmd
```

`release/easyx_red_black_tree.exe` 已静态链接 EasyX 和 GCC C++ 运行库，不要求另一台电脑安装 Visual Studio、VS Code、Dev-C++ 或 MinGW。

也可以直接双击 `run_easyx.cmd` 或 `release/easyx_red_black_tree.exe`。仓库同时保留了 PowerShell 入口 `run_easyx.ps1`。

## 在 VS Code 中修改和重新编译

重新编译需要一套与 EasyX 兼容的 64 位 MSVCRT MinGW。当前脚本会自动寻找常见位置中的 TDM-GCC，也支持通过环境变量指定：

```powershell
$env:EASYX_GXX = "D:\Compiler\MinGW64\bin\g++.exe"
.\build_easyx.ps1
```

VS Code 操作：

- `Ctrl+Shift+B`：执行默认构建任务。
- `F5`：启动调试配置。
- 命令面板选择 `Tasks: Run Task`，再选择 `Run Red-Black Tree Data Tests`：运行主项目自己的 39 项数据测试。

也可以在终端运行：

```powershell
.\run_data_tests.ps1
```

图形化入口和数据测试入口彼此独立，并且都不依赖 `reference-projects`：

- `src/easyx_frontend.cpp`：EasyX 图形界面。
- `src/main.cpp`：从参考测试迁入主项目的数据测试。
- `src/rbtree.h`：主项目实际使用的红黑树实现。

`reference-projects/red-black-tree-template` 只是普通参考目录。它不参与编译、链接、测试或运行，删除该目录也不会影响主项目。

注意：EasyX 官方 MinGW 25.9.10 不支持 UCRT MinGW。若脚本提示跳过 UCRT 编译器，需要改用 TDM-GCC/MSVCRT 工具链。

## 目录

```text
src/rbtree.h                       红黑树头文件副本
src/tree_visualization_model.*     后端模型与坐标计算
src/easyx_frontend.cpp             EasyX 前端
third_party/easyx                  官网 EasyX MinGW 文件
release/easyx_red_black_tree.exe   克隆后可直接运行的版本
```
