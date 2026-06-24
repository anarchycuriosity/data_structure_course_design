#ifndef TREE_VISUALIZATION_MODEL_H
#define TREE_VISUALIZATION_MODEL_H

#include <vector>

#include "rbtree.h"

// 为什么需要单独再套一层TreeVisualizationModel？
// 因为RBTree只应该关心插入、删除、颜色和旋转，它不应该知道屏幕坐标是什么。
// EasyX前端也不应该拿到树内部的真实指针，它只需要拿到已经计算好的绘图数据。
// 所以这一层就是桥梁：左边接RBTree，右边给EasyX提供可以直接画的节点。
struct VisualNode
{
    int key;           // 圆里面需要显示的数字
    bool is_black;     // true画黑色，false画红色
    int parent_index;  // 父亲在vector里的下标，根节点没有父亲，所以根是-1
    int x;             // 最后换算出来的屏幕横坐标
    int y;             // 最后换算出来的屏幕纵坐标
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
    // 真正维护红黑性质的是它，前端不会直接接触它。
    RBTree<int> tree;
};

#endif
