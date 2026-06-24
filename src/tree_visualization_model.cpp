#include "tree_visualization_model.h"

#include <algorithm>

namespace
{
// 这些尺寸必须和easyx_frontend.cpp保持一致。
// 因为后端需要知道真正可以画树的区域有多大，否则算出来的点可能压到按钮或者状态栏上。
const int window_width = 1100;
const int window_height = 720;
const int toolbar_height = 105;
const int status_height = 55;

struct NodePosition
{
    int x;      // 第一次递归时，x暂时保存中序遍历次序；最后才换成像素坐标
    int y;      // 最后计算出来的像素纵坐标
    int depth;  // 根是0层，孩子是1层，孙子是2层
};

// 这里不是在画图，而是在决定节点从左到右的顺序。
// 为什么用中序遍历？
// 因为二叉搜索树按照“左、自己、右”访问后，键值天然从小到大。
// 那么访问得越早，节点就应该越靠左；访问得越晚，节点就应该越靠右。
void assign_inorder_position(
    const std::vector<RBTree<int>::VisualizationNode>& nodes,
    int node_index,
    int depth,
    int& visit_order,
    std::vector<NodePosition>& positions)
{
    if (node_index < 0)
    {
        // -1代表这个方向没有孩子，走到空位置就返回。
        return;
    }

    // 先把左子树全部排好，所以左孩子一定会比父亲更早获得横向序号。
    assign_inorder_position(nodes, nodes[node_index].left_index, depth + 1, visit_order, positions);

    // 注意这里的x还不是屏幕像素，它只是“这是中序遍历遇到的第几个节点”。
    positions[node_index].x = visit_order;
    positions[node_index].depth = depth;
    ++visit_order;

    // 最后排右子树，所以右孩子一定会出现在父亲右边。
    assign_inorder_position(nodes, nodes[node_index].right_index, depth + 1, visit_order, positions);
}

// assign_inorder_position只得到“顺序”和“深度”。
// calculate_positions再把这两个抽象数字压进窗口真正的绘图区。
std::vector<NodePosition> calculate_positions(const std::vector<RBTree<int>::VisualizationNode>& nodes)
{
    std::vector<NodePosition> positions(nodes.size(), {0, 0, 0});

    if (nodes.empty())
    {
        // 空树没有任何坐标需要计算。
        return positions;
    }

    int visit_order = 0;
    assign_inorder_position(nodes, 0, 0, visit_order, positions);

    const int drawing_left = 45;
    const int drawing_right = window_width - 45;
    const int drawing_top = toolbar_height + 45;
    const int drawing_bottom = window_height - status_height - 35;
    int max_depth = 0;

    // 先找最深层数，因为纵坐标缩放需要知道整棵树一共有多少层。
    for (const NodePosition& position : positions)
    {
        max_depth = std::max(max_depth, position.depth);
    }

    for (NodePosition& position : positions)
    {
        // 为什么分母是nodes.size()+1？
        // 如果直接除以nodes.size()-1，最左和最右节点会贴着边框。
        // 左右各留一个虚拟空位之后，每个节点都会和窗口边缘保持距离。
        position.x =
            drawing_left + (drawing_right - drawing_left) * (position.x + 1) / (static_cast<int>(nodes.size()) + 1);

        // max_depth可能为0，比如树里只有一个根节点。
        // std::max(1, max_depth)保证分母至少为1，不会发生除以0。
        position.y = drawing_top + (drawing_bottom - drawing_top) * position.depth / std::max(1, max_depth);
    }

    return positions;
}
}

TreeVisualizationModel::TreeVisualizationModel()
{
    // 构造时直接准备一棵小树，这样窗口第一次打开不会是一片空白。
    reset_to_demo();
}

bool TreeVisualizationModel::insert(int value)
{
    return tree.insert(value);
}

bool TreeVisualizationModel::remove(int value)
{
    return tree.remove(value);
}

bool TreeVisualizationModel::contains(int value)
{
    return tree.contains(value);
}

void TreeVisualizationModel::reset_to_demo()
{
    const int demo_values[] = {10, 5, 15, 3, 7, 12, 18};

    // 这里依然调用正常insert，而不是手工拼节点。
    // 所以示例树和用户自己插入得到的树走的是同一套红黑修复逻辑。
    for (int value : demo_values)
    {
        tree.insert(value);
    }
}

std::vector<VisualNode> TreeVisualizationModel::visual_nodes() const
{
    // 第一步拿到树的只读快照。
    // snapshot里面有键值、颜色和父子下标，但是还没有屏幕坐标。
    std::vector<RBTree<int>::VisualizationNode> snapshot = tree.visualization_snapshot();

    // 第二步根据中序次序和深度计算坐标。
    std::vector<NodePosition> positions = calculate_positions(snapshot);

    // 第三步把“树的数据”和“坐标数据”合成前端最终需要的VisualNode。
    std::vector<VisualNode> result;
    result.reserve(snapshot.size());

    // reserve只预留容量，不会真的创建元素。
    // 这样下面push_back时通常不需要反复申请更大的内存。
    for (std::size_t index = 0; index < snapshot.size(); ++index)
    {
        result.push_back(
            {snapshot[index].key, snapshot[index].is_black, snapshot[index].parent_index, positions[index].x,
             positions[index].y});
    }

    return result;
}
