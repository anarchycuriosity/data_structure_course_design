#include "tree_visualization_model.h"

#include <algorithm>

namespace
{
const int window_width = 1100;
const int window_height = 720;
const int toolbar_height = 105;
const int status_height = 55;

struct NodePosition
{
    int x;
    int y;
    int depth;
};

void assign_inorder_position(
    const std::vector<RBTree<int>::VisualizationNode>& nodes,
    int node_index,
    int depth,
    int& visit_order,
    std::vector<NodePosition>& positions)
{
    if (node_index < 0)
    {
        return;
    }

    assign_inorder_position(nodes, nodes[node_index].left_index, depth + 1, visit_order, positions);

    positions[node_index].x = visit_order;
    positions[node_index].depth = depth;
    ++visit_order;

    assign_inorder_position(nodes, nodes[node_index].right_index, depth + 1, visit_order, positions);
}

std::vector<NodePosition> calculate_positions(const std::vector<RBTree<int>::VisualizationNode>& nodes)
{
    std::vector<NodePosition> positions(nodes.size(), {0, 0, 0});

    if (nodes.empty())
    {
        return positions;
    }

    int visit_order = 0;
    assign_inorder_position(nodes, 0, 0, visit_order, positions);

    const int drawing_left = 45;
    const int drawing_right = window_width - 45;
    const int drawing_top = toolbar_height + 45;
    const int drawing_bottom = window_height - status_height - 35;
    int max_depth = 0;

    for (const NodePosition& position : positions)
    {
        max_depth = std::max(max_depth, position.depth);
    }

    for (NodePosition& position : positions)
    {
        position.x =
            drawing_left + (drawing_right - drawing_left) * (position.x + 1) / (static_cast<int>(nodes.size()) + 1);
        position.y = drawing_top + (drawing_bottom - drawing_top) * position.depth / std::max(1, max_depth);
    }

    return positions;
}
}

TreeVisualizationModel::TreeVisualizationModel()
{
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

    for (int value : demo_values)
    {
        tree.insert(value);
    }
}

std::vector<VisualNode> TreeVisualizationModel::visual_nodes() const
{
    std::vector<RBTree<int>::VisualizationNode> snapshot = tree.visualization_snapshot();
    std::vector<NodePosition> positions = calculate_positions(snapshot);
    std::vector<VisualNode> result;
    result.reserve(snapshot.size());

    for (std::size_t index = 0; index < snapshot.size(); ++index)
    {
        result.push_back(
            {snapshot[index].key, snapshot[index].is_black, snapshot[index].parent_index, positions[index].x,
             positions[index].y});
    }

    return result;
}
