# 项目学习总览：Red-Black Tree 课程设计

## 项目目标

本课程设计要求实现一个 Red-Black Tree ADT，并通过界面演示初始化、查找、插入、删除等基本操作。指定演示序列是：

```text
A L G O R I T H M
```

也就是先按这个顺序插入，再按同样顺序删除。

## 学习路线

本项目建议按知识点推进，而不是按文件机械阅读。

| 阶段 | 知识点 | 你需要掌握什么 | 验收标准 |
| --- | --- | --- | --- |
| 1 | 二叉搜索树 BST | 为什么有序树能加速查找 | 能手画插入序列后的 BST |
| 2 | 树高与复杂度 | 为什么退化链表会让查找变慢 | 能解释 `O(log n)` 和 `O(n)` 的差别 |
| 3 | 红黑树五条性质 | 红黑树如何限制树高 | 能检查一棵树是否合法 |
| 4 | 旋转 | 旋转如何局部改变结构但保持中序有序 | 能手动完成左旋、右旋 |
| 5 | 插入修复 | 为什么新节点通常染红 | 能画出叔叔红、叔叔黑两类修复 |
| 6 | 删除修复 | 什么是黑高亏损 | 能解释双黑节点的修复方向 |
| 7 | ADT 设计 | 数据结构和操作如何封装 | 能列出 `init/search/insert/remove` 接口 |
| 8 | 可视化演示 | 如何把树结构映射为界面坐标 | 能展示每一步操作后的树 |
| 9 | 从面向对象过渡到模板 | 构造/析构、重载知识如何连接 `template` | 能把 `T` 暂时替换成 `int` 阅读源码 |
| 10 | C++ 模板 | 一份树代码如何支持多种键类型 | 能解释 `RBTree<int>` 的实例化过程 |
| 11 | 封装与显式转换 | `private`、`friend`、`explicit` 如何阻止误用 | 能构造并验证一个隐式转换反例 |
| 12 | 迭代器与运算符重载 | `++it`、`*it` 如何映射到成员函数 | 能追踪参考项目的后序遍历状态 |
| 13 | 编译与可移植性 | 标准 C++ 与编译器扩展有何区别 | 能识别 VLA、GNU 语句表达式和过时 API |
| 14 | EasyX 最简前端 | 如何把树状态映射成线、圆、文字和按钮 | 能追踪一次点击到整帧重绘的数据流 |

## 当前建议

不要直接手推完整的 `A L G O R I T H M` 修复过程。新的第一关是只画 `10、20、15`，完成一次左旋和一次镜像右旋，并检查中间子树、根指针、父指针和中序序列。旋转通过后，再学习“叔叔颜色 + 直线/折线”的插入修复判断。

## 外部参考项目

`reference-projects/red-black-tree-template` 是以 Git 子模块引入的 MIT 许可参考实现。主项目根据当前课程实现需求，将其中的 `rbtree.h` 原样复制为 `src/rbtree.h`，并用文件哈希确认内容一致。以后阅读原理可对照子模块，实际编译则使用主项目内的副本。

## 文档索引

- `001_red_black_tree_mission_roadmap.md`：面向零基础读者的旋转详解、红黑修复入门与项目路线。
- `003_cpp_syntax_bridge_from_oop_to_template.md`：给刚学完构造函数、析构函数、重载/重写的读者准备的 C++ 语法桥，先用熟悉的面向对象概念解释模板、`explicit`、`inline`、`typename`、友元和运算符重载。
- `002_cpp_language_mechanisms_in_reference_project.md`：进阶阅读，从参考项目反推模板、`typename`、`inline`、`explicit`、友元、迭代器、编译链接和可移植性。建议先读 `003`，再读本篇。
- `004_easyx_frontend_from_zero.md`：从零解释像素、事件循环、只读快照、中序布局和 EasyX 绘图，配套当前最简窗口前端。

当前代码结构已经拆成红黑树头文件、可视化模型后端和 EasyX 前端。阅读时先看 `tree_visualization_model.h/.cpp`，最后再看 `easyx_frontend.cpp`。

EasyX MinGW 25.9.10 已放入 `third_party/easyx`。VS Code 默认构建任务会调用兼容 EasyX 的 TDM-GCC 4.9.2，而不是系统中的 UCRT MinGW 15.2。

若只需要演示，无需安装任何编译器：克隆仓库后运行 `run_easyx.cmd`，它会启动 `release/easyx_red_black_tree.exe`。该发行程序已静态链接所需 C++ 运行库。

数据测试与图形化同时保留。运行 `run_data_tests.ps1` 会构建并执行主项目 `src/main.cpp` 的 39 项测试；VS Code 中也可以运行任务 `Run Red-Black Tree Data Tests`。测试、图形界面和红黑树实现均不依赖 `reference-projects`。
