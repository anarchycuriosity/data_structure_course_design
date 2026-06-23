#include <cassert>
#include <vector>

#include "../src/tree_visualization_model.h"

int main()
{
    TreeVisualizationModel model;
    std::vector<VisualNode> initial_nodes = model.visual_nodes();

    assert(initial_nodes.size() == 7);
    assert(initial_nodes[0].parent_index == -1);
    assert(initial_nodes[0].is_black);

    for (std::size_t index = 0; index < initial_nodes.size(); ++index)
    {
        assert(initial_nodes[index].x > 0);
        assert(initial_nodes[index].y > 0);

        if (initial_nodes[index].parent_index >= 0)
        {
            assert(initial_nodes[index].parent_index < static_cast<int>(initial_nodes.size()));
        }
    }

    assert(model.insert(6));
    assert(model.contains(6));
    assert(model.remove(6));
    assert(!model.contains(6));
    return 0;
}
