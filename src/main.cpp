// ---------------------------------------------------------------------
// MIT License
// Copyright (c) 2017 Henrik Peters
// See LICENSE file in the project root for full license information.
// ---------------------------------------------------------------------
#include <algorithm>
#include <iomanip>
#include <iostream>

// 这份main.cpp不是红黑树的实现，而是一套很小的“自动测试程序”。
// 可以把它理解成三层结构：
// 1. 最底层的AssertTrue、AssertFalse、AssertEquals负责判断一个条件是否满足。
// 2. 每个lambda负责构造一种具体树形，并调用断言检查结果。
// 3. main中的统一循环依次运行所有lambda，最后统计通过率。
//
// 为什么不只观察程序有没有崩溃？
// 因为红黑树即使颜色错了、父子关系错了，也可能暂时还能完成contains。
// 测试必须同时检查“红黑树性质”“保存的数据”和“少量案例的精确形状”，才能尽早发现隐蔽错误。

// DEBUG 是预处理宏。定义它后，rbtree.h 才会编译 invariant、toString 和 dumpTree 等调试接口。
// #include 发生在预处理阶段，所以必须先定义 DEBUG，再包含 rbtree.h。
#ifndef DEBUG
#define DEBUG
#endif

#include "rbtree.h"
using namespace std;

// 【参考项目警告】下面三个断言宏依赖 GNU 的“语句表达式”扩展 ({ ... })，并非标准 C++。
// 它们适合快速搭建原项目的轻量测试，但课程设计应改用普通函数或成熟测试框架。
// 宏只做文本替换，没有类型检查；参数若带副作用，还可能产生难排查的问题。
//
// 断言的共同规则是：条件正确就继续向下执行，条件错误就立刻return false。
// 因此，一个测试函数只有顺利走到最后的TestPassed，才会被判定为成功。
#define TestPassed   \
    {                \
        return true; \
    }
#define AssertEquals(exp, act)                                                                      \
    ({                                                                                              \
        if (exp != act)                                                                             \
        {                                                                                           \
            cerr << endl << "Asseration failed: Expected <" << exp << "> but was <" << act << "> "; \
            return false;                                                                           \
        }                                                                                           \
    })
#define AssertTrue(x)     \
    ({                    \
        if (!x)           \
        {                 \
            return false; \
        }                 \
    })
#define AssertFalse(x)    \
    ({                    \
        if (x)            \
        {                 \
            return false; \
        }                 \
    })

// RBTree 是类模板，RBTree<int> 才是把 T 替换成 int 后得到的具体类型。
// typedef 给这个较长类型取别名；现代 C++ 更推荐写 using IntTree = RBTree<int>。
typedef RBTree<int> IntTree;
// 如果不用typedef enum而只用enum会很长
// enum TestResult my_result = UNTESTED; // 极其冗长
// 所以我们给这个enum TestResult取个别名
typedef enum TestResult
{
    // 为什么不用一个bool保存结果？
    // 因为测试创建后还没有运行，此时既不能说成功，也不能说失败。
    // UNTESTED把“尚未执行”这个中间状态明确保存下来，避免默认值被误认为测试结果。
    SUCCESS = 0,
    FAILED = 1,
    UNTESTED = 2
} TestResult;

// TestFunc 是“无参数、返回 bool 的普通函数指针”类型。
// 无捕获 lambda 也能转换成这种函数指针，因此后面的测试表可以统一保存测试逻辑。
typedef bool (*TestFunc)();
int TestCounter = 1;

// Test 把测试描述、执行函数和结果封装成一个最小测试用例对象。
// 相比在 main 中堆叠 if，它能让测试数据进入数组，并由统一循环执行和统计。
// 这里实际上把“测试是什么”和“怎样调度测试”解耦了：
// lambda只关心自己的验证逻辑，Test和main只关心编号、运行及输出。
class Test
{
   private:
    unsigned int id;
    string description;
    TestResult result;
    TestFunc test;

   public:
    Test(string description, TestFunc test);
    void run();

    // 定义在类体内的成员函数天然隐式 inline；这里再写 inline 其实是重复强调。
    inline unsigned int getID() { return id; };
    inline unsigned int getResult() { return result; };
    inline string getDescription() { return description; };
};

Test::Test(string description, TestFunc test)
{
    // this 指向“当前正在构造的 Test 对象”。当参数与成员同名时，this-> 可消除歧义。
    this->id = TestCounter++;
    this->description = description;
    this->result = UNTESTED;
    this->test = test;
}

// test保存的是函数地址，this->test()才是真正调用这个测试函数。
// 三目运算符把bool返回值转换成更明确的TestResult状态。
void Test::run() { this->result = this->test() ? SUCCESS : FAILED; }

// 下面四个辅助函数采用 Arrange（准备）—Act（操作）—Assert（验证）的测试结构。
// 最关键的策略不是只检查最终结果，而是在每一步插入/删除后检查红黑树不变量。
bool sortedInsert(int amount)
{
    // 有序插入是二叉搜索树的极端输入。
    // 普通二叉搜索树会退化成一条长度为amount的链，红黑树则必须通过换色和旋转保持近似平衡。
    IntTree* tree = new IntTree();

    for (int i = 0; i < amount; i++)
    {
        tree->insert(i);
        // 为什么每插入一个数都检查，而不是最后只检查一次？
        // 如果第7次插入已经破坏性质、第8次又偶然遮住问题，只检查终态就无法定位首次出错的位置。
        AssertTrue(tree->invariant());
    }

    // invariant只能证明结构和颜色满足红黑树规则，不能证明所有输入值都真的保存了。
    // 所以还要逐个contains，分别验证“结构正确”和“数据没有丢失”。
    for (int i = 0; i < amount; i++)
    {
        AssertTrue(tree->contains(i));
    }

    delete tree;
    TestPassed;
}

bool randomInsert(int amount, bool checkContains = true, bool invariantAfterInsert = true)
{
    // 随机插入用于覆盖有序案例没有遇到的左右旋、内侧节点和外侧节点组合。
    // 两个bool参数是在控制测试成本：
    // 小规模测试逐步检查；一百万规模测试只检查最终不变量，避免验证本身耗时过大。
    IntTree* tree = new IntTree();
    // 这是变长数组（VLA），不是标准 C++，且 amount 很大时会耗尽线程栈。
    // 课程代码应使用 std::vector<int> numbers(amount)。这里保留上游实现，仅作风险标注。
    int numbers[amount];

    for (int i = 0; i < amount; i++)
    {
        numbers[i] = i;
    }

    // 先生成0到amount-1，保证测试数据没有重复、没有遗漏；洗牌只改变插入顺序。
    // 这样最终仍然可以明确知道树中应该包含哪些键。
    // random_shuffle 在 C++17 已被移除；现代写法是 std::shuffle + 显式随机数引擎。
    // numbers 与 numbers + amount 构成左闭右开的迭代器区间 [begin, end)。
    random_shuffle(numbers, numbers + amount);

    for (int i = 0; i < amount; i++)
    {
        tree->insert(numbers[i]);

        if (invariantAfterInsert)
        {
            AssertTrue(tree->invariant());
        }
    }

    if (checkContains)
    {
        // contains检查的是集合语义，不要求树长成某个固定形状。
        // 同一批数字按不同顺序插入，合法的红黑树形状可能不同，因此随机测试不比较toString。
        for (int i = 0; i < amount; i++)
        {
            AssertTrue(tree->contains(i));
        }
    }

    // Check the invariant only for the final tree
    if (!invariantAfterInsert)
    {
        AssertTrue(tree->invariant());
    }

    delete tree;
    TestPassed;
}

bool randomRemove(int amount)
{
    // 删除测试分成三个阶段：
    // 先准备完整数据集，再随机决定删除顺序，最后每删一个值就检查结构与“该值确实消失”。
    IntTree* tree = new IntTree();
    int numbers[amount];

    for (int i = 0; i < amount; i++)
    {
        numbers[i] = i;
    }

    random_shuffle(numbers, numbers + amount);

    for (int i = 0; i < amount; i++)
    {
        tree->insert(numbers[i]);
    }

    // 多次洗牌并不会让一次合格的均匀洗牌“更随机”，这里只是原项目的测试写法。
    for (int i = 0; i < 10; i++)
    {
        random_shuffle(numbers, numbers + amount);
    }

    for (int i = 0; i < amount; i++)
    {
        tree->remove(numbers[i]);
        // 删除修复比插入修复更容易出错，因为它可能产生“双黑”并向祖先传播。
        // invariant负责捕获红红相连、根非黑色和左右黑高不同等内部错误。
        AssertTrue(tree->invariant());
        // invariant并不知道调用者原本想删除哪个键，所以还要单独检查目标值已经不存在。
        AssertFalse(tree->contains(numbers[i]));
    }

    delete tree;
    TestPassed;
}

bool randomIterate(int amount)
{
    // 这个测试不关心树内部颜色，而是验证迭代器能否做到两件事：
    // 1. 从begin走到end时不会漏节点或重复节点。
    // 2. 解引用*it得到的键确实来自原始输入集合。
    IntTree* tree = new IntTree();
    int act[amount];
    int exp[amount];

    for (int i = 0; i < amount; i++)
    {
        exp[i] = i;
    }

    random_shuffle(exp, exp + amount);

    for (int i = 0; i < amount; i++)
    {
        tree->insert(exp[i]);
    }

    int elemCount = 0;

    for (IntTree::iterator it = tree->begin(); it != tree->end(); ++it)
    {
        // *it 调用 iterator::operator* 取得当前键；++it 调用前置递增以移动到下一节点。
        act[elemCount++] = *it;
    }

    AssertEquals(amount, elemCount);

    // 为什么还要做双层循环？
    // elemCount相等只能证明“取出了同样多个值”，不能证明内容正确。
    // 对act中的每个值去exp中查找，才能排除迭代器返回错误键的情况。
    // 这里复杂度是O(n^2)，但最大只测试100个元素，优先保留直观写法。
    for (int i = 0; i < amount; i++)
    {
        bool found = false;
        for (int j = 0; j < amount && !found; j++)
        {
            if (act[i] == exp[j])
            {
                found = true;
            }
        }

        if (!found)
        {
            return false;
        }
    }

    delete tree;
    TestPassed;
}

int main()
{
    // 聚合初始化一个测试用例数组。每个 {描述, [](){...}} 都构造一个 Test。
    // [] 是 lambda 捕获列表；为空表示不捕获外部变量，因此可转换为 TestFunc 函数指针。
    // lambda 让测试代码紧贴描述，不必为每个小案例单独命名函数。
    //
    // 测试顺序是刻意从局部到整体安排的：
    // 1. 先验证1到3个节点的基础插入。
    // 2. 再用固定输入命中插入、删除修复的各个分支。
    // 3. 然后用大量随机数据做压力验证。
    // 4. 最后独立验证迭代器。
    Test testSuite[] = {
        // ==================== 第一组：基础插入 ====================
        // 固定小案例同时检查invariant、contains和toString。
        // toString相当于给内部树形拍快照：键、颜色或父子位置只要有一个不同，字符串就不会相等。
        {"Inserting 1 element into empty tree",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(5);

             // 第一节点必须成为根，而红黑树要求根节点为黑色。
             AssertTrue(tree->invariant());
             AssertEquals(true, tree->contains(5));
             AssertEquals("└── 5 (B)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Inserting 2 elements",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(5);
             AssertTrue(tree->invariant());

             tree->insert(7);
             AssertTrue(tree->invariant());

             // 7大于5，所以它应位于右侧；新节点保持红色不会改变任何路径的黑高。
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(7));
             AssertEquals("└── 5 (B)\n    └── 7 (R)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Inserting 3 elements",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(5);
             AssertTrue(tree->invariant());

             tree->insert(7);
             AssertTrue(tree->invariant());

             tree->insert(3);
             AssertTrue(tree->invariant());

             // 3和7分别落在根的左右两边，二者都是红色，但它们的父节点5是黑色，因此没有红红冲突。
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(3));
             AssertEquals("└── 5 (B)\n    ├── 3 (R)\n    └── 7 (R)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        // ==================== 第二组：插入修复分支 ====================
        // 这些输入不是随便挑的，而是在主动制造父红、叔叔颜色不同以及内外侧方向不同的局面。
        // case编号对应rbtree.h中adjustInsert的判断顺序。
        {"Inserting adjust case 1",
         []()
         {
             // 最终修复过程会向上走到根。
             // 一旦当前冲突节点成为根，只需把根染黑，所有根到叶子的路径会同时增加一个黑节点。
             IntTree* tree = new IntTree();
             tree->insert(1);
             AssertTrue(tree->invariant());

             tree->insert(3);
             AssertTrue(tree->invariant());

             tree->insert(4);
             AssertTrue(tree->invariant());

             tree->insert(2);
             AssertTrue(tree->invariant());

             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(3));
             AssertTrue(tree->contains(4));
             AssertTrue(tree->contains(2));
             AssertEquals("└── 3 (B)\n    ├── 1 (B)\n    │   └── 2 (R)\n    └── 4 (B)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Inserting adjust case 2",
         []()
         {
             // 父节点为黑色时，新插入的红节点不会形成“红父红子”。
             // 新节点又是红色，不增加路径黑高，所以修复可以立即结束。
             IntTree* tree = new IntTree();
             tree->insert(2);
             AssertTrue(tree->invariant());

             tree->insert(4);
             AssertTrue(tree->invariant());

             tree->insert(1);
             AssertTrue(tree->invariant());

             AssertTrue(tree->contains(2));
             AssertTrue(tree->contains(4));
             AssertTrue(tree->contains(1));
             AssertEquals("└── 2 (B)\n    ├── 1 (R)\n    └── 4 (R)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Inserting adjust case 3",
         []()
         {
             // 父亲和叔叔都为红色时，不能只旋转某一边，否则容易破坏左右黑高。
             // 正确处理是父亲、叔叔染黑，祖父染红，再把问题提升到祖父继续检查。
             IntTree* tree = new IntTree();
             tree->insert(5);
             AssertTrue(tree->invariant());

             tree->insert(2);
             AssertTrue(tree->invariant());

             tree->insert(7);
             AssertTrue(tree->invariant());

             tree->insert(1);
             AssertTrue(tree->invariant());

             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(2));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(1));
             AssertEquals("└── 5 (B)\n    ├── 2 (B)\n    │   └── 1 (R)\n    └── 7 (B)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Inserting adjust case 4 [right child, no uncle]",
         []()
         {
             // NULL叔叔按黑色处理。当前节点与父节点方向形成“折线”时，
             // 先围绕父节点旋转，把折线转成直线，再交给下一种情况处理。
             IntTree* tree = new IntTree();
             tree->insert(3);
             AssertTrue(tree->invariant());

             tree->insert(1);
             AssertTrue(tree->invariant());

             tree->insert(7);
             AssertTrue(tree->invariant());

             tree->insert(9);
             AssertTrue(tree->invariant());

             tree->insert(8);
             AssertTrue(tree->invariant());

             AssertTrue(tree->contains(3));
             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(9));
             AssertTrue(tree->contains(8));
             AssertEquals("└── 3 (B)\n    ├── 1 (B)\n    └── 8 (B)\n        ├── 7 (R)\n        └── 9 (R)\n",
                          tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Inserting adjust case 4 [right child, with uncle]",
         []()
         {
             // 这里叔叔不是NULL，但颜色仍是黑色。
             // 算法判断真正关心的是叔叔颜色；专门保留此案例，是为了防止代码只正确处理NULL叔叔。
             IntTree* tree = new IntTree();
             tree->insert(5);
             AssertTrue(tree->invariant());

             tree->insert(10);
             AssertTrue(tree->invariant());

             tree->insert(6);
             AssertTrue(tree->invariant());

             tree->insert(17);
             AssertTrue(tree->invariant());

             tree->insert(18);
             AssertTrue(tree->invariant());

             tree->insert(7);
             AssertTrue(tree->invariant());

             tree->insert(8);
             AssertTrue(tree->invariant());

             // 插入14时，目标局部结构中的叔叔节点真实存在。
             tree->insert(14);
             AssertTrue(tree->invariant());

             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(10));
             AssertTrue(tree->contains(6));
             AssertTrue(tree->contains(17));
             AssertTrue(tree->contains(18));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(8));
             AssertTrue(tree->contains(14));
             AssertEquals(
                 "└── 8 (B)\n    ├── 6 (R)\n    │   ├── 5 (B)\n    │   └── 7 (B)\n    └── 17 (R)\n        ├── 10 (B)\n "
                 "       │   └── 14 (R)\n        └── 18 (B)\n",
                 tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Inserting adjust case 5 [left child, no uncle]",
         []()
         {
             // 当前节点、父节点和祖父已经位于同一方向，形成“直线”。
             // 此时围绕祖父旋转并交换父亲与祖父的颜色，就能消除红红冲突且保持黑高。
             IntTree* tree = new IntTree();
             tree->insert(5);
             AssertTrue(tree->invariant());

             tree->insert(3);
             AssertTrue(tree->invariant());

             tree->insert(7);
             AssertTrue(tree->invariant());

             tree->insert(1);
             AssertTrue(tree->invariant());

             tree->insert(2);
             AssertTrue(tree->invariant());

             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(3));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(2));
             AssertEquals("└── 5 (B)\n    ├── 2 (B)\n    │   ├── 1 (R)\n    │   └── 3 (R)\n    └── 7 (B)\n",
                          tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Inserting adjust case 5 [left child, with uncle]",
         []()
         {
             // 与上一个测试的修复机制相同，但叔叔是一个真实存在的黑节点。
             // 它验证实现依据颜色决策，而不是错误地依据叔叔指针是否为NULL决策。
             IntTree* tree = new IntTree();
             tree->insert(16);
             AssertTrue(tree->invariant());

             tree->insert(18);
             AssertTrue(tree->invariant());

             tree->insert(19);
             AssertTrue(tree->invariant());

             tree->insert(2);
             AssertTrue(tree->invariant());

             tree->insert(3);
             AssertTrue(tree->invariant());

             tree->insert(8);
             AssertTrue(tree->invariant());

             tree->insert(11);
             AssertTrue(tree->invariant());

             // 插入15时才真正命中“有叔叔且叔叔为黑”的目标场景。
             tree->insert(15);
             AssertTrue(tree->invariant());

             AssertTrue(tree->contains(16));
             AssertTrue(tree->contains(18));
             AssertTrue(tree->contains(19));
             AssertTrue(tree->contains(2));
             AssertTrue(tree->contains(3));
             AssertTrue(tree->contains(8));
             AssertTrue(tree->contains(11));
             AssertTrue(tree->contains(15));
             AssertEquals(
                 "└── 11 (B)\n    ├── 3 (R)\n    │   ├── 2 (B)\n    │   └── 8 (B)\n    └── 18 (R)\n        ├── 16 "
                 "(B)\n        │   └── 15 (R)\n        └── 19 (B)\n",
                 tree->toString());

             delete tree;
             TestPassed;
         }},
        // ==================== 第三组：插入压力测试 ====================
        // 固定案例适合定位分支，批量案例适合发现某些长路径组合后的累积错误。
        {"Inserting 20 elements (sorted)", []() { return sortedInsert(20); }},
        {"Inserting 20 elements (random)", []() { return randomInsert(20); }},
        {"Inserting 50 elements (sorted)", []() { return sortedInsert(50); }},
        {"Inserting 50 elements (random)", []() { return randomInsert(50); }},
        {"Inserting 100 elements (random)", []() { return randomInsert(100); }},
        {"Inserting 1000 elements (random)", []() { return randomInsert(1000); }},
        // 一百万元素主要验证规模和最终状态。
        // false、false表示不逐个contains，也不每次insert后检查，以免测试辅助操作掩盖真实性能。
        {"Inserting 1 Mio elements (random)", []() { return randomInsert(1000000, false, false); }},

        // ==================== 第四组：基础删除 ====================
        // 删除案例先覆盖节点拥有0、1、2个孩子的结构差异，再进入颜色修复的具体分支。
        {"Removing the root node",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(5);
             tree->remove(5);
             AssertTrue(tree->invariant());

             // 唯一根节点删除后，树必须同时满足“不含5”和“根为空”。
             AssertFalse(tree->contains(5));
             AssertEquals("empty tree", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing a red leaf [0 childs]",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(5);
             tree->insert(1);
             tree->insert(7);

             tree->remove(7);
             AssertTrue(tree->invariant());

             // 红叶不贡献黑高，直接删除不会改变其他路径上的黑节点数量，因此不需要复杂修复。
             AssertFalse(tree->contains(7));
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(1));
             AssertEquals("└── 5 (B)\n    └── 1 (R)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing a black node with red child [1 child]",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(5);
             tree->insert(1);
             tree->insert(7);
             tree->insert(3);

             tree->remove(1);
             AssertTrue(tree->invariant());

             // 黑节点被删会少一个黑色，但它唯一的红孩子可以接替位置并染黑，补回缺失的黑高。
             AssertFalse(tree->contains(1));
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(3));
             AssertEquals("└── 5 (B)\n    ├── 3 (B)\n    └── 7 (B)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing black leaf [0 childs]",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(1);
             tree->insert(2);
             tree->insert(3);
             tree->insert(4);

             // 先删除3，把树准备成目标形状；真正要观察的是随后删除黑叶4的修复结果。
             tree->remove(3);
             AssertTrue(tree->invariant());

             tree->remove(4);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(3));
             AssertFalse(tree->contains(4));
             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(2));
             AssertEquals("└── 2 (B)\n    └── 1 (R)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing node with 1 child [red child]",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(1);
             tree->insert(2);
             tree->insert(3);
             tree->insert(4);

             tree->remove(3);
             AssertTrue(tree->invariant());

             // 删除有一个孩子的节点时，孩子必须被正确接到原节点的父亲上。
             // 这个测试不仅查颜色，还通过toString检查父子指针是否接对。
             AssertFalse(tree->contains(3));
             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(2));
             AssertTrue(tree->contains(4));
             AssertEquals("└── 2 (B)\n    ├── 1 (B)\n    └── 4 (B)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing node with 2 childs [root node]",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(1);
             tree->insert(2);
             tree->insert(3);
             tree->insert(4);

             // 先删除4，使根的两个孩子都成为黑色，构造更严格的双孩子删除场景。
             tree->remove(4);
             AssertTrue(tree->invariant());

             tree->remove(2);
             AssertTrue(tree->invariant());

             // 删除有两个孩子的节点通常不会直接搬走整个节点，
             // 而是寻找后继节点替换键，再转化为删除至多一个孩子的节点。
             AssertFalse(tree->contains(4));
             AssertFalse(tree->contains(2));
             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(3));
             AssertEquals("└── 3 (B)\n    └── 1 (R)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        // ==================== 第五组：删除修复分支 ====================
        // 删除黑节点可能让某一条路径少一个黑色。实现中用DOUBLE_BLACK表示这个“欠一个黑色”的中间状态。
        // adjustRemove会观察父亲、兄弟和侄子颜色，通过换色或旋转逐步消除这笔黑高欠账。
        {"Removing adjust case 1 [0 childs, root node]",
         []()
         {
             // 如果双黑位置已经是根，就不再与其他分支比较黑高，直接恢复成普通黑色即可结束。
             IntTree* tree = new IntTree();
             tree->insert(7);
             tree->remove(7);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(7));
             AssertEquals("empty tree", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing adjust case 2 [0 childs, left node]",
         []()
         {
             // 这一组让待修复位置位于父节点左侧，用来覆盖“兄弟在右边”的处理方向。
             // 删除后的精确快照还能检查旋转时祖父、父亲和孩子的连接是否全部更新。
             IntTree* tree = new IntTree();
             tree->insert(3);
             tree->insert(2);
             tree->insert(5);
             tree->insert(7);
             tree->insert(8);
             tree->insert(9);

             tree->remove(2);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(2));
             AssertTrue(tree->contains(3));
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(8));
             AssertTrue(tree->contains(9));
             AssertEquals("└── 7 (B)\n    ├── 3 (B)\n    │   └── 5 (R)\n    └── 8 (B)\n        └── 9 (R)\n",
                          tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing adjust case 2 [0 childs, right node]",
         []()
         {
             // 红黑树的很多删除规则左右完全对称，但代码中的left和right指针是两套分支。
             // 因此右侧情况不能靠上一项推测正确，必须用镜像输入单独测试。
             IntTree* tree = new IntTree();
             tree->insert(8);
             tree->insert(6);
             tree->insert(7);
             tree->insert(1);
             tree->insert(4);
             tree->insert(3);

             // 删除8后，内部会转化为目标修复场景；描述关注的是最终命中的adjustRemove分支。
             tree->remove(8);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(8));
             AssertTrue(tree->contains(6));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(4));
             AssertTrue(tree->contains(3));
             AssertEquals("└── 4 (B)\n    ├── 1 (B)\n    │   └── 3 (R)\n    └── 7 (B)\n        └── 6 (R)\n",
                          tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing adjust case 3 [0 childs, symmetric]",
         []()
         {
             // symmetric表示测试的是同一颜色关系的镜像方向。
             // 即使数学规则对称，实际代码也可能把某个left误写为right，所以镜像测试很有必要。
             IntTree* tree = new IntTree();
             tree->insert(2);
             tree->insert(19);
             tree->insert(3);
             tree->insert(6);
             tree->insert(7);
             tree->insert(10);
             tree->insert(11);
             tree->insert(18);
             tree->insert(17);
             tree->insert(20);

             // 删除6能够构造目标局部颜色关系，删除其他双孩子节点也可能转化到同一分支。
             tree->remove(6);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(6));
             AssertTrue(tree->contains(2));
             AssertTrue(tree->contains(19));
             AssertTrue(tree->contains(3));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(10));
             AssertTrue(tree->contains(11));
             AssertTrue(tree->contains(18));
             AssertTrue(tree->contains(17));
             AssertTrue(tree->contains(20));

             delete tree;
             TestPassed;
         }},
        {"Removing adjust case 4 [0 childs, symmetric]",
         []()
         {
             // 这一分支通常通过给兄弟换色，把双黑问题向父节点上推。
             // 测试重点是问题向上移动后，整棵树仍然满足相同黑高。
             IntTree* tree = new IntTree();
             tree->insert(7);
             tree->insert(8);
             tree->insert(3);
             tree->insert(4);
             tree->insert(5);
             tree->insert(2);

             tree->remove(8);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(8));
             AssertTrue(tree->contains(7));
             AssertTrue(tree->contains(3));
             AssertTrue(tree->contains(4));
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(2));
             AssertEquals("└── 4 (B)\n    ├── 3 (B)\n    │   └── 2 (R)\n    └── 7 (B)\n        └── 5 (R)\n",
                          tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing adjust case 5 [0 childs, right sibling child red]",
         []()
         {
             // 兄弟的“近侄子”为红、“远侄子”为黑时，先围绕兄弟旋转，
             // 把局面转成最终可一次消除双黑的case 6。
             IntTree* tree = new IntTree();
             tree->insert(5);
             tree->insert(1);
             tree->insert(7);
             tree->insert(2);

             tree->remove(7);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(7));
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(2));
             AssertEquals("└── 2 (B)\n    ├── 1 (B)\n    └── 5 (B)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing adjust case 5 [0 childs, left sibling child red]",
         []()
         {
             // 这是上一项的镜像：兄弟位于另一侧，红色近侄子也随之换边。
             // 两项一起验证左右方向判断没有写反。
             IntTree* tree = new IntTree();
             tree->insert(5);
             tree->insert(1);
             tree->insert(10);
             tree->insert(7);
             tree->insert(12);
             tree->insert(11);

             tree->remove(7);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(7));
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(10));
             AssertTrue(tree->contains(12));
             AssertTrue(tree->contains(11));
             AssertEquals("└── 5 (B)\n    ├── 1 (B)\n    └── 11 (R)\n        ├── 10 (B)\n        └── 12 (B)\n",
                          tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing adjust case 6 [0 childs, left node]",
         []()
         {
             // 兄弟的远侄子为红时，可以围绕父节点旋转并重新着色，
             // 在当前局部直接补齐缺少的黑高，删除修复到此结束。
             IntTree* tree = new IntTree();
             tree->insert(5);
             tree->insert(1);
             tree->insert(7);
             tree->insert(8);
             tree->insert(9);
             tree->insert(10);

             tree->remove(7);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(7));
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(1));
             AssertTrue(tree->contains(8));
             AssertTrue(tree->contains(9));
             AssertTrue(tree->contains(10));
             AssertEquals("└── 5 (B)\n    ├── 1 (B)\n    └── 9 (R)\n        ├── 8 (B)\n        └── 10 (B)\n",
                          tree->toString());

             delete tree;
             TestPassed;
         }},
        {"Removing adjust case 6 [0 childs, right node]",
         []()
         {
             // case 6的镜像版本。最终三个节点全部为黑色，
             // 表明旋转后红色远侄子已经承担了原父节点的颜色调整职责。
             IntTree* tree = new IntTree();
             tree->insert(5);
             tree->insert(3);
             tree->insert(7);
             tree->insert(2);

             tree->remove(7);
             AssertTrue(tree->invariant());

             AssertFalse(tree->contains(7));
             AssertTrue(tree->contains(5));
             AssertTrue(tree->contains(3));
             AssertTrue(tree->contains(2));
             AssertEquals("└── 3 (B)\n    ├── 2 (B)\n    └── 5 (B)\n", tree->toString());

             delete tree;
             TestPassed;
         }},
        // ==================== 第六组：删除压力测试 ====================
        // 固定树形只能覆盖设计者想到的路径；随机删除顺序用于组合出更多连续修复场景。
        {"Removing 20 elements (random)", []() { return randomRemove(20); }},
        {"Removing 50 elements (random)", []() { return randomRemove(50); }},
        {"Removing 100 elements (random)", []() { return randomRemove(100); }},
        {"Removing 1000 elements (random)", []() { return randomRemove(1000); }},
        // ==================== 第七组：迭代器 ====================
        // 迭代器是红黑树对外暴露的另一条访问路径，不能因为insert/remove正确就默认它也正确。
        {"Iterator test [empty tree]",
         []()
         {
             IntTree* tree = new IntTree();
             bool foundElement = false;

             // 空树的begin必须等于end，因此循环体一次也不应该进入。
             for (IntTree::iterator it = tree->begin(); it != tree->end(); ++it)
             {
                 foundElement = true;
             }

             if (tree->begin() != tree->end())
             {
                 // 这是对边界对象本身的直接检查，与foundElement形成双重验证。
                 return false;
             }

             AssertFalse(foundElement);

             delete tree;
             TestPassed;
         }},
        {"Iterator test [1 element]",
         []()
         {
             IntTree* tree = new IntTree();
             tree->insert(1);

             int elemCount = 0;
             int number = 0;

             for (IntTree::iterator it = tree->begin(); it != tree->end(); ++it)
             {
                 number = *it;
                 elemCount++;
             }

             // 一个元素既不能漏掉，也不能因为递增逻辑错误而被访问两次。
             AssertEquals(1, elemCount);
             AssertEquals(1, number);

             delete tree;
             TestPassed;
         }},
        {"Iterator random values [0..100 elements]", []()
         {
             // 连续测试0到99种规模，0覆盖空树边界，其余规模覆盖不同高度。
             // randomIterate内部同时检查访问数量和访问值集合。
             for (int i = 0; i < 100; i++)
             {
                 if (!randomIterate(i))
                 {
                     return false;
                 }
             }

             TestPassed;
         }}};

    unsigned int passed = 0;
    unsigned int failed = 0;

    // sizeof(testSuite)得到整个数组占用的字节数，sizeof(testSuite[0])得到一个元素的字节数。
    // 二者相除就是数组元素个数。这里testSuite仍是本地数组，所以尚未退化成指针。
    for (unsigned int i = 0; i < sizeof(testSuite) / sizeof(testSuite[0]); i++)
    {
        // 这里会复制出一个Test对象。
        // 因此run修改的是副本t的result，不会写回testSuite[i]；当前程序马上读取t，所以结果仍然正确。
        Test t = testSuite[i];
        // setfill('0')与setw(2)把编号显示为01、02等形式。
        // setw只影响紧随其后的一个输出值，setfill则会持续生效。
        cout << "Running Test " << setfill('0') << setw(2) << t.getID() << ": ";
        t.run();

        if (t.getResult() == FAILED)
        {
            failed++;
            // \033[...]是ANSI终端颜色控制序列：31表示红色，32表示绿色，0表示恢复默认样式。
            cout << "\033[1;31mFailed\033[0m";
            cout << " (" << t.getDescription() << ")" << endl;
        }
        else
        {
            passed++;
            cout << "\033[1;32mPassed\033[0m";
            cout << " (" << t.getDescription() << ")" << endl;
        }
    }

    // 整数除法会舍弃小数部分，例如39/40会显示97而不是97.5。
    // 测试数组非空，所以这里不会出现除以0；若测试集可能为空，就必须先保护分母。
    unsigned int sucessRate = (passed * 100) / (passed + failed);

    cout << "--------------------" << endl;
    cout << "Tests passed: ";
    if (sucessRate == 100)
    {
        cout << "\033[1;32m" << sucessRate << "%\033[0m" << endl;
    }
    else
    {
        cout << "\033[1;31m" << sucessRate << "%\033[0m" << endl;
    }
    cout << "--------------------" << endl;
    return 0;
}
