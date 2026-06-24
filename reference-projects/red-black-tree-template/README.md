# Red-Black tree

> 中文学习提示：本仓库在不改变上游算法逻辑的前提下，为 `rbtree.h` 和 `main.cpp`
> 补充了面向零基础读者的中文注释。建议先阅读项目根目录下
> `learning-in-pro/002_cpp_language_mechanisms_in_reference_project.md`，再按“公开接口 →
> BST 查找/插入 → 左右旋 → 插入修复 → 删除修复 → 不变量验证”的顺序阅读源码。
> 这份参考实现含有 GNU 扩展、变长数组和已过时的 `random_shuffle`，用于学习算法与测试思路，
> 不应原样复制为课程作业。

This is an implementation of a self-balancing binary search tree. The tree is using a
generic element type. This implementation is focused on performance instead of an elegant
solution that would be easier to understand. Note that all operations are in-place destructive
and use no recursion (except for some debugging functions).

## Performance
Red–black trees guarantee search, insertion and deletion in logarithmic time. This can especially be
useful when an application has to provide worst-case guarantees, which can not be made by a lot of other
data structures. For *n* as total number of elements in the tree:

| Operation |   Average  | Worst case |
|-----------|:----------:|:----------:|
|  Space    |   O(*n*)   |   O(*n*)   |
|  Search   | O(log *n*) | O(log *n*) |
|  Insert   | O(log *n*) | O(log *n*) |
|  Delete   | O(log *n*) | O(log *n*) |

## Memory
An element will be stored in a node which means the number of nodes is equivalent to the number of elements.
Only during a delete operation, there may a double black null node which holds no element. At the moment this
implementation is using a nested class to represent a node. This is nice for encapsulation and separating
logic between a node and a tree. The downside is that the code of a node cannot change the root pointer of
the tree. To solve this issue each node contains a pointer to the tree. For further memory optimisations, it is
possible to get rid of this pointer by breaking the encapsulation of the nested node class.

## Generic elements
In order to use any type and provide type safety at the same time, the implementation is using a template. For some
applications, it makes sense to store a key-value pair in each node. This tree only stores one type, however modifying
it in a way to store a pair should not be very difficult. The functions are implemented in the header file because
a template is used. Another solution would be to keep the implementation separated and explicitly instantiate all
the template types that are needed. The second solution should be preferred for large projects.

## Visualization
The tree can be visualized with the dump function. The dump will generate a graph and png file with the `Graphviz`-Tool.
This can be useful for a better understanding of the data structure and for debugging purposes. Example of the tree visualization:
<p align="center">
    <img src="https://raw.githubusercontent.com/wiki/Henrik-Peters/Red-Black-Tree/images/tree-example.png" width="800">
</p>
