#include <graphics.h>

#include <algorithm>
#include <cwchar>
#include <string>
#include <vector>

#include "../reference-projects/red-black-tree-template/rbtree.h"

namespace
{
const int window_width = 1100;
const int window_height = 720;
const int toolbar_height = 105;
const int status_height = 55;
const int node_radius = 22;

// Button 只保存一个矩形和文字。
// 它不知道红黑树，也不执行插入删除，因此绘图组件和算法组件没有互相缠住。
struct Button
{
    int left;
    int top;
    int right;
    int bottom;
    const wchar_t* text;

    bool contains(int x, int y) const
    {
        return x >= left && x <= right && y >= top && y <= bottom;
    }
};

struct NodePosition
{
    int x;
    int y;
    int depth;
};

const Button insert_button = {30, 50, 150, 88, L"插入节点"};
const Button remove_button = {170, 50, 290, 88, L"删除节点"};
const Button reset_button = {310, 50, 450, 88, L"重置示例"};

void draw_button(const Button& button, COLORREF fill_color)
{
    setlinecolor(RGB(90, 100, 120));
    setfillcolor(fill_color);
    fillroundrect(button.left, button.top, button.right, button.bottom, 8, 8);

    settextcolor(RGB(25, 30, 40));
    settextstyle(18, 0, L"Microsoft YaHei");
    int text_x = (button.left + button.right - textwidth(button.text)) / 2;
    int text_y = (button.top + button.bottom - textheight(button.text)) / 2;
    outtextxy(text_x, text_y, button.text);
}

// 中序遍历天然按照“左子树、当前节点、右子树”访问。
// 我们把访问序号转换成 x 坐标，同一深度转换成 y 坐标，
// 这样左孩子一定画在父节点左侧，右孩子一定画在右侧。
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
        // 分母加 1，让最左、最右节点都与窗口边缘留出空白。
        position.x =
            drawing_left + (drawing_right - drawing_left) * (position.x + 1) / (static_cast<int>(nodes.size()) + 1);
        position.y = drawing_top + (drawing_bottom - drawing_top) * position.depth / std::max(1, max_depth);
    }

    return positions;
}

void draw_tree(const RBTree<int>& tree)
{
    std::vector<RBTree<int>::VisualizationNode> nodes = tree.visualization_snapshot();

    if (nodes.empty())
    {
        settextcolor(RGB(130, 135, 145));
        settextstyle(26, 0, L"Microsoft YaHei");
        const wchar_t* empty_text = L"当前是一棵空树，请点击“插入节点”";
        outtextxy((window_width - textwidth(empty_text)) / 2, 330, empty_text);
        return;
    }

    std::vector<NodePosition> positions = calculate_positions(nodes);

    // 先画边、后画节点，连线末端会被圆覆盖，画面更干净。
    setlinecolor(RGB(115, 125, 145));
    setlinestyle(PS_SOLID, 2);

    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        int parent_index = nodes[index].parent_index;

        if (parent_index >= 0)
        {
            line(positions[parent_index].x, positions[parent_index].y, positions[index].x, positions[index].y);
        }
    }

    settextstyle(18, 0, L"Consolas");

    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        COLORREF node_color = nodes[index].is_black ? RGB(35, 38, 45) : RGB(220, 55, 60);
        setlinecolor(RGB(245, 245, 245));
        setfillcolor(node_color);
        fillcircle(positions[index].x, positions[index].y, node_radius);

        std::wstring key_text = std::to_wstring(nodes[index].key);
        settextcolor(WHITE);
        outtextxy(
            positions[index].x - textwidth(key_text.c_str()) / 2,
            positions[index].y - textheight(key_text.c_str()) / 2,
            key_text.c_str());
    }
}

void draw_scene(const RBTree<int>& tree, const std::wstring& status_text)
{
    setbkcolor(RGB(244, 247, 252));
    cleardevice();
    setbkmode(TRANSPARENT);

    settextcolor(RGB(25, 30, 40));
    settextstyle(26, 0, L"Microsoft YaHei");
    outtextxy(30, 12, L"红黑树最简可视化");

    settextcolor(RGB(105, 112, 125));
    settextstyle(16, 0, L"Microsoft YaHei");
    outtextxy(500, 62, L"黑色节点  /  红色节点");

    draw_button(insert_button, RGB(184, 225, 190));
    draw_button(remove_button, RGB(250, 205, 205));
    draw_button(reset_button, RGB(210, 220, 240));
    draw_tree(tree);

    setfillcolor(RGB(226, 232, 242));
    solidrectangle(0, window_height - status_height, window_width, window_height);
    settextcolor(RGB(45, 52, 65));
    settextstyle(17, 0, L"Microsoft YaHei");
    outtextxy(25, window_height - status_height + 17, status_text.c_str());

    FlushBatchDraw();
}

bool read_integer(const wchar_t* title, int& value)
{
    wchar_t input_buffer[32] = L"";

    if (!InputBox(input_buffer, 32, L"请输入一个整数：", title, L"", 320, 0, false))
    {
        return false;
    }

    wchar_t* parse_end = nullptr;
    long parsed_value = std::wcstol(input_buffer, &parse_end, 10);

    // wcstol 会把无法转换的位置写入 parse_end。
    // 必须同时检查“一个字符都没读到”和“数字后还有垃圾字符”。
    if (parse_end == input_buffer || *parse_end != L'\0')
    {
        MessageBox(GetHWnd(), L"请输入完整整数，例如：42 或 -7。", L"输入无效", MB_OK | MB_ICONWARNING);
        return false;
    }

    value = static_cast<int>(parsed_value);
    return true;
}

void reset_to_demo(RBTree<int>& tree)
{
    const int demo_values[] = {10, 5, 15, 3, 7, 12, 18};

    for (int value : demo_values)
    {
        tree.insert(value);
    }
}
}

int main()
{
    initgraph(window_width, window_height);
    BeginBatchDraw();

    // RBTree 禁止安全复制，所以重置时销毁旧树并重新创建。
    // 这里显式使用 new/delete，是为了让当前阶段的你能直接看见对象生命周期。
    RBTree<int>* tree = new RBTree<int>();
    reset_to_demo(*tree);
    std::wstring status_text = L"示例树已就绪：可插入、删除或重置。";
    bool is_running = true;

    while (is_running)
    {
        draw_scene(*tree, status_text);
        ExMessage message = getmessage(EX_MOUSE | EX_WINDOW);

        if (message.message == WM_CLOSE)
        {
            is_running = false;
        }
        else if (message.message == WM_LBUTTONDOWN)
        {
            int value = 0;

            if (insert_button.contains(message.x, message.y) && read_integer(L"插入节点", value))
            {
                bool inserted = tree->insert(value);
                status_text = inserted ? L"插入成功：" + std::to_wstring(value)
                                       : L"插入失败：树中已经存在 " + std::to_wstring(value);
            }
            else if (remove_button.contains(message.x, message.y) && read_integer(L"删除节点", value))
            {
                bool removed = tree->remove(value);
                status_text = removed ? L"删除成功：" + std::to_wstring(value)
                                      : L"删除失败：树中不存在 " + std::to_wstring(value);
            }
            else if (reset_button.contains(message.x, message.y))
            {
                delete tree;
                tree = new RBTree<int>();
                reset_to_demo(*tree);
                status_text = L"已恢复示例树：10、5、15、3、7、12、18。";
            }
        }
    }

    delete tree;
    EndBatchDraw();
    closegraph();
    return 0;
}
