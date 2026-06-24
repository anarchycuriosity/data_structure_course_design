#include <cwchar>
#include <string>
#include <vector>

#include "tree_visualization_model.h"

// 为什么graphics.h要放在tree_visualization_model.h后面？
// 因为EasyX自己定义了RED、BLACK这种全局宏，而rbtree.h内部也有同名的颜色枚举。
// 宏就是粗暴的文本替换，如果先包含graphics.h，rbtree.h里的RED会先被替换成数字，枚举直接无法编译。
// 所以先让编译器完整看完红黑树，再引入EasyX，这样宏就影响不到前面已经解析好的代码。
#include <graphics.h>

namespace
{
const int window_width = 1100;
const int window_height = 720;
const int status_height = 55;
const int node_radius = 22;

// 按钮没有使用EasyX自带控件，因为这里最简方案只需要一个矩形点击区域。
// left、top、right、bottom围成按钮，text是按钮中间显示的文字。
// 它不保存红黑树，也不负责插入删除，只回答“这个坐标有没有落在我里面”。
struct Button
{
    int left;
    int top;
    int right;
    int bottom;
    const wchar_t* text;

    bool contains(int x, int y) const
    {
        // 鼠标点同时落在左右边界和上下边界之间，才算点中按钮。
        return x >= left && x <= right && y >= top && y <= bottom;
    }
};

const Button insert_button = {30, 50, 150, 88, L"插入节点"};
const Button remove_button = {170, 50, 290, 88, L"删除节点"};
const Button reset_button = {310, 50, 450, 88, L"重置示例"};

void draw_button(const Button& button, COLORREF fill_color)
{
    // 先设置线条颜色和填充颜色，再画圆角矩形。
    // EasyX的绘图状态会一直保留，所以每个函数最好在自己画之前重新设置需要的颜色。
    setlinecolor(RGB(90, 100, 120));
    setfillcolor(fill_color);
    fillroundrect(button.left, button.top, button.right, button.bottom, 8, 8);

    settextcolor(RGB(25, 30, 40));
    settextstyle(18, 0, L"Microsoft YaHei");
    // 文字居中的本质不是EasyX自动布局，而是自己计算左上角。
    // 按钮中心减去文字宽高的一半，就得到文字左上角坐标。
    int text_x = (button.left + button.right - textwidth(button.text)) / 2;
    int text_y = (button.top + button.bottom - textheight(button.text)) / 2;
    outtextxy(text_x, text_y, button.text);
}

void draw_tree(const TreeVisualizationModel& model)
{
    // 前端每次重画都重新向model要数据，不保存上一次的节点坐标。
    // 因为一次删除可能引发旋转和换色，旧坐标很可能已经不代表当前树。
    std::vector<VisualNode> nodes = model.visual_nodes();

    if (nodes.empty())
    {
        // 空树不能继续访问nodes[0]，这里直接画提示并返回。
        settextcolor(RGB(130, 135, 145));
        settextstyle(26, 0, L"Microsoft YaHei");
        const wchar_t* empty_text = L"当前是一棵空树，请点击“插入节点”";
        outtextxy((window_width - textwidth(empty_text)) / 2, 330, empty_text);
        return;
    }

    // 为什么先画边再画圆？
    // line是从父亲圆心直接连到孩子圆心的。
    // 后画的圆会盖住线的两端，看起来就像线只连接到圆边；反过来画，线会穿过数字。
    setlinecolor(RGB(115, 125, 145));
    setlinestyle(PS_SOLID, 2);

    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        int parent_index = nodes[index].parent_index;

        if (parent_index >= 0)
        {
            // 根节点parent_index是-1，所以不会给根画一条不存在的父边。
            line(nodes[parent_index].x, nodes[parent_index].y, nodes[index].x, nodes[index].y);
        }
    }

    settextstyle(18, 0, L"Consolas");

    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        // 红黑树内部保存bool，前端在这里把逻辑颜色转换成真正的RGB颜色。
        COLORREF node_color = nodes[index].is_black ? RGB(35, 38, 45) : RGB(220, 55, 60);
        setlinecolor(RGB(245, 245, 245));
        setfillcolor(node_color);
        fillcircle(nodes[index].x, nodes[index].y, node_radius);

        // outtextxy需要字符串，所以先把int键值转换成宽字符串。
        settextcolor(WHITE);
        std::wstring key_text = std::to_wstring(nodes[index].key);

        // 节点坐标表示圆心，而outtextxy接收文字左上角。
        // 所以同样要减去文字宽高的一半，让数字落在圆心。
        outtextxy(
            nodes[index].x - textwidth(key_text.c_str()) / 2,
            nodes[index].y - textheight(key_text.c_str()) / 2,
            key_text.c_str());
    }
}

void draw_scene(const TreeVisualizationModel& model, const std::wstring& status_text)
{
    // 每一帧都先清空整张画布，再按照固定顺序重画。
    // 这种做法比“只擦掉变化的节点”简单，因为旋转可能让很多节点同时移动。
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

    // 窗口底部单独画一条状态栏，用来说明上一次操作成功还是失败。
    setfillcolor(RGB(226, 232, 242));
    solidrectangle(0, window_height - status_height, window_width, window_height);
    settextcolor(RGB(45, 52, 65));
    settextstyle(17, 0, L"Microsoft YaHei");
    outtextxy(25, window_height - status_height + 17, status_text.c_str());

    // BeginBatchDraw之后，前面的绘图先进入后台缓冲。
    // FlushBatchDraw在这里一次显示完整画面，避免用户看到逐条线、逐个圆刷新的闪烁过程。
    FlushBatchDraw();
}

bool read_integer(const wchar_t* title, int& value)
{
    wchar_t input_buffer[32] = L"";

    if (!InputBox(input_buffer, 32, L"请输入一个整数：", title, L"", 320, 0, false))
    {
        // 用户按取消时不应该执行插入或删除，所以返回false。
        return false;
    }

    wchar_t* parse_end = nullptr;
    long parsed_value = std::wcstol(input_buffer, &parse_end, 10);

    // wcstol从左向右读数字，并把停止位置交给parse_end。
    // parse_end等于开头，说明第一个字符就不是数字。
    // parse_end没有走到字符串结尾，说明输入类似12abc，后面还有不能转换的字符。
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
    // initgraph建立真正的Windows图形窗口。
    initgraph(window_width, window_height);

    // 从这里开始进入批量绘图模式，直到程序结束前调用EndBatchDraw。
    BeginBatchDraw();

    // model持有真正的RBTree<int>。
    // 前端只调用它公开的insert、remove和visual_nodes，不接触树节点指针。
    TreeVisualizationModel* model = new TreeVisualizationModel();
    std::wstring status_text = L"示例树已就绪：可插入、删除或重置。";
    bool is_running = true;

    while (is_running)
    {
        // 当前版本没有单独的动画线程，所以每轮先按最新状态画完整一帧，再等待下一条消息。
        draw_scene(*model, status_text);

        // getmessage会阻塞等待，不会让while循环空转占满CPU。
        // EX_MOUSE接收鼠标消息，EX_WINDOW接收关闭窗口等窗口消息。
        ExMessage message = getmessage(EX_MOUSE | EX_WINDOW);

        if (message.message == WM_CLOSE)
        {
            // 点击右上角关闭按钮后跳出事件循环。
            is_running = false;
        }
        else if (message.message == WM_LBUTTONDOWN)
        {
            int value = 0;

            if (insert_button.contains(message.x, message.y) && read_integer(L"插入节点", value))
            {
                // insert返回false代表这个值已经存在，因为当前RBTree实现的是集合，不保存重复值。
                bool inserted = model->insert(value);
                status_text = inserted ? L"插入成功：" + std::to_wstring(value)
                                       : L"插入失败：树中已经存在 " + std::to_wstring(value);
            }
            else if (remove_button.contains(message.x, message.y) && read_integer(L"删除节点", value))
            {
                // remove返回false代表树里根本没有这个值。
                bool removed = model->remove(value);
                status_text = removed ? L"删除成功：" + std::to_wstring(value)
                                      : L"删除失败：树中不存在 " + std::to_wstring(value);
            }
            else if (reset_button.contains(message.x, message.y))
            {
                // RBTree内部拥有一整棵动态节点树，不应该直接做浅拷贝。
                // 所以重置时销毁整个model，再由构造函数建立一棵新的示例树。
                delete model;
                model = new TreeVisualizationModel();
                status_text = L"已恢复示例树：10、5、15、3、7、12、18。";
            }
        }
    }

    // 退出前释放model，RBTree析构函数会继续递归释放所有节点。
    delete model;
    EndBatchDraw();
    closegraph();
    return 0;
}
