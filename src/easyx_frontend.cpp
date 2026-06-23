#include <graphics.h>

#include <cwchar>
#include <string>
#include <vector>

#include "tree_visualization_model.h"

namespace
{
const int window_width = 1100;
const int window_height = 720;
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

void draw_tree(const TreeVisualizationModel& model)
{
    std::vector<VisualNode> nodes = model.visual_nodes();

    if (nodes.empty())
    {
        settextcolor(RGB(130, 135, 145));
        settextstyle(26, 0, L"Microsoft YaHei");
        const wchar_t* empty_text = L"当前是一棵空树，请点击“插入节点”";
        outtextxy((window_width - textwidth(empty_text)) / 2, 330, empty_text);
        return;
    }

    // 先画边、后画节点，连线末端会被圆覆盖，画面更干净。
    setlinecolor(RGB(115, 125, 145));
    setlinestyle(PS_SOLID, 2);

    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        int parent_index = nodes[index].parent_index;

        if (parent_index >= 0)
        {
            line(nodes[parent_index].x, nodes[parent_index].y, nodes[index].x, nodes[index].y);
        }
    }

    settextstyle(18, 0, L"Consolas");

    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        COLORREF node_color = nodes[index].is_black ? RGB(35, 38, 45) : RGB(220, 55, 60);
        setlinecolor(RGB(245, 245, 245));
        setfillcolor(node_color);
        fillcircle(nodes[index].x, nodes[index].y, node_radius);

        std::wstring key_text = std::to_wstring(nodes[index].key);
        settextcolor(WHITE);
        outtextxy(
            nodes[index].x - textwidth(key_text.c_str()) / 2,
            nodes[index].y - textheight(key_text.c_str()) / 2,
            key_text.c_str());
    }
}

void draw_scene(const TreeVisualizationModel& model, const std::wstring& status_text)
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
    draw_tree(model);

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

}

int main()
{
    initgraph(window_width, window_height);
    BeginBatchDraw();

    TreeVisualizationModel* model = new TreeVisualizationModel();
    std::wstring status_text = L"示例树已就绪：可插入、删除或重置。";
    bool is_running = true;

    while (is_running)
    {
        draw_scene(*model, status_text);
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
                bool inserted = model->insert(value);
                status_text = inserted ? L"插入成功：" + std::to_wstring(value)
                                       : L"插入失败：树中已经存在 " + std::to_wstring(value);
            }
            else if (remove_button.contains(message.x, message.y) && read_integer(L"删除节点", value))
            {
                bool removed = model->remove(value);
                status_text = removed ? L"删除成功：" + std::to_wstring(value)
                                      : L"删除失败：树中不存在 " + std::to_wstring(value);
            }
            else if (reset_button.contains(message.x, message.y))
            {
                delete model;
                model = new TreeVisualizationModel();
                status_text = L"已恢复示例树：10、5、15、3、7、12、18。";
            }
        }
    }

    delete model;
    EndBatchDraw();
    closegraph();
    return 0;
}
