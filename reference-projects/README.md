# 参考项目目录

本目录只用于阅读、比较设计和追溯来源。

主项目的编译、测试、运行均不读取本目录中的任何文件：

- 实际红黑树实现：`src/rbtree.h`
- 数据测试入口：`src/main.cpp`
- 图形界面入口：`src/easyx_frontend.cpp`

因此删除整个 `reference-projects` 目录后，主项目仍然可以正常构建和测试。
