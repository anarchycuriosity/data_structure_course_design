#ifndef TREE_VISUALIZATION_MODEL_H
#define TREE_VISUALIZATION_MODEL_H

#include <vector>

#include "../reference-projects/red-black-tree-template/rbtree.h"

// 这一层属于后端：负责保存树、执行操作并计算节点位置。
// 它完全不知道 EasyX、按钮、鼠标和颜色值。
struct VisualNode
{
    int key;
    bool is_black;
    int parent_index;
    int x;
    int y;
};

class TreeVisualizationModel
{
   public:
    TreeVisualizationModel();

    bool insert(int value);
    bool remove(int value);
    bool contains(int value);
    void reset_to_demo();
    std::vector<VisualNode> visual_nodes() const;

   private:
    RBTree<int> tree;
};

#endif
