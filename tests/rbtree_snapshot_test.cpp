#include <cassert>
#include <vector>

#include "../src/rbtree.h"

int main()
{
    RBTree<int> tree;
    const int values[] = {10, 5, 15, 3, 7, 12, 18};

    for (int value : values)
    {
        assert(tree.insert(value));
    }

    std::vector<RBTree<int>::VisualizationNode> snapshot = tree.visualization_snapshot();
    assert(snapshot.size() == 7);
    assert(snapshot[0].parent_index == -1);
    assert(snapshot[0].is_black);

    for (std::size_t index = 0; index < snapshot.size(); ++index)
    {
        if (snapshot[index].left_index >= 0)
        {
            int left_index = snapshot[index].left_index;
            assert(snapshot[left_index].parent_index == static_cast<int>(index));
            assert(snapshot[left_index].key < snapshot[index].key);
        }

        if (snapshot[index].right_index >= 0)
        {
            int right_index = snapshot[index].right_index;
            assert(snapshot[right_index].parent_index == static_cast<int>(index));
            assert(snapshot[right_index].key > snapshot[index].key);
        }
    }

    assert(tree.remove(10));
    assert(tree.visualization_snapshot().size() == 6);
    return 0;
}
