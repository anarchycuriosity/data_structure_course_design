// ---------------------------------------------------------------------
// MIT License
// Copyright (c) 2017 Henrik Peters
// See LICENSE file in the project root for full license information.
// ---------------------------------------------------------------------
#ifndef RBTREE_H
#define RBTREE_H

#include <iterator>
#include <vector>

// 【阅读导航】
// 这是一个“类模板 + 头文件实现”的红黑树。建议按下面顺序阅读：
// 1. 先看 RBTree 的公开接口 contains/insert/remove；
// 2. 再看普通二叉搜索树部分 lookup/insert；
// 3. 然后看旋转 leftRotate/rightRotate；
// 4. 最后看颜色修复 adjustInsert/adjustRemove。
// template <typename T> 表示“先写一份树的结构，使用时再把 T 替换成 int、char 等具体类型”。
// 模板的完整定义通常必须放在头文件中，因为编译器实例化 RBTree<int> 时需要看到全部实现。

#ifdef DEBUG
#include <assert.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
#endif

template <typename T>
class RBTree
{
   private:
    // 节点作为私有嵌套类：树的使用者只操作 RBTree，不应直接修改节点颜色和指针。
    // 这是一种封装设计，可缩小“能破坏红黑树不变量”的代码范围。
    class RBTreeNode
    {
       private:
        enum Color
        {
            RED = 0,
            BLACK = 1,
            DOUBLE_BLACK = 2,
        };

        T key;        // 节点保存的值；T 至少需要支持 ==、< 和 > 比较。
        Color color;  // 颜色不是装饰，而是维护从根到叶子黑高一致的状态信息。

        RBTreeNode* parent;  // 父指针让旋转和删除修复可以向上回溯。
        RBTreeNode* left;
        RBTreeNode* right;
        // 节点需要在旋转后更新整棵树的 root，因此保存所属树指针。
        // 这是用每个节点多一个指针，换取节点内部操作根指针的便利。
        RBTree<T>* tree;

       public:
        RBTreeNode(const T key, RBTree<T>* tree);
        RBTreeNode(const T key, RBTreeNode* parent, RBTree<T>* tree, Color color);
        // virtual 允许通过基类指针析构派生对象；本项目并没有继承节点，实际可不必使用。
        // 保留它是尊重上游实现，学习时不要把“析构函数一律 virtual”当成规则。
        virtual ~RBTreeNode();

        // friend 只开放私有成员访问权，不代表继承。
        // 外层类在 C++ 中不会自动获得嵌套类的私有访问权，因此这里显式授权。
        friend class RBTree<T>;
        friend class iterator;

#ifdef DEBUG
        bool invariant();
        int invariantBlackNodes();
        void toString(ostream& buffer, const string& prefix, bool lastNode);
        void dumpNode(ofstream& graphFile);
#endif

        // const 表示该成员函数不会修改当前节点；inline 允许函数在头文件中被多处包含而不违反定义规则。
        // 编译器是否真的展开函数由优化器决定，inline 不是“强制提升性能”的命令。
        inline bool isBlack() const { return (this->color == BLACK); }
        inline void adjustInsert(RBTreeNode* insertNode);
        inline void adjustRemove();
        inline void leftRotate();
        inline void rightRotate();

        RBTreeNode* lookup(const T key);
        bool insert(const T key);
        void remove();
    }* root;  // 这种写法等价于先结束 RBTreeNode 类定义，再声明 RBTreeNode* root。

   public:
    // 这是提供给“界面层”的只读节点快照。
    //
    // 为什么不直接把 RBTreeNode 改成 public？
    // 因为界面一旦能修改 parent、left、right、color，就可能轻易破坏红黑树。
    // 快照只复制绘图需要的数据，不暴露真实指针，前端因此只能“看”，不能“改”。
    struct VisualizationNode
    {
        T key;
        bool is_black;
        int parent_index;
        int left_index;
        int right_index;

        VisualizationNode(const T& node_key, bool node_is_black, int node_parent_index)
            : key(node_key),
              is_black(node_is_black),
              parent_index(node_parent_index),
              left_index(-1),
              right_index(-1)
        {
        }
    };

    RBTree();
    // 当前类同样没有派生用法，virtual 不是必需；这是参考项目原有设计。
    virtual ~RBTree();

    bool contains(const T key);
    bool insert(const T key);
    bool remove(const T key);
    std::vector<VisualizationNode> visualization_snapshot() const;

#ifdef DEBUG
    bool invariant();
    void dumpTree(string dumpName = "dump");
    string toString();
#endif

    class iterator
    {
       private:
        RBTreeNode* node = nullptr;

       public:
        // 这些 typedef 是迭代器的“类型说明书”，标准库算法会通过它们了解迭代器。
        typedef T value_type;
        typedef const T& reference;
        typedef const T* pointer;
        typedef std::input_iterator_tag iterator_category;
        friend class RBTree<T>;

        // explicit 禁止把 RBTreeNode* 悄悄隐式转换成 iterator。
        // 例如 iterator it = node_ptr 会被拒绝，必须明确写 iterator(node_ptr)，意图更清楚。
        // 冒号后的 node(_node) 是成员初始化列表，在进入构造函数体之前初始化成员。
        explicit iterator(RBTreeNode* _node) : node(_node) {}
        // implicit copy constructor

        // operator++ 是运算符重载：让自定义迭代器可以像指针一样写 ++it。
        // 前置 ++ 返回引用；后置 ++ 需要保存旧值并按值返回，所以通常成本更高。
        iterator& operator++();
        inline iterator operator++(int)
        {
            iterator it = *this;
            ++(*this);
            return it;
        }

        inline bool operator==(const iterator& other) { return node == other.node; }
        inline bool operator!=(const iterator& other) { return !(*this == other); }

        inline reference operator*() { return node->key; }
        inline pointer operator->() { return &node->key; }
    };

    iterator begin();
    iterator end();
};

// Tree nodes
template <typename T>
RBTree<T>::RBTreeNode::RBTreeNode(const T key, RBTree<T>* tree)  // 只有键和树的构造函数
{
    this->left = NULL;
    this->right = NULL;
    this->parent = NULL;
    this->key = key;
    this->tree = tree;
    this->color = BLACK;
}

template <typename T>
RBTree<T>::RBTreeNode::RBTreeNode(const T key, RBTreeNode* parent, RBTree<T>* tree, Color color)  // 键，父母，树，颜色
{
    this->left = NULL;
    this->right = NULL;
    this->parent = parent;
    this->key = key;
    this->tree = tree;
    this->color = color;
}

template <typename T>
RBTree<T>::RBTreeNode::~RBTreeNode()  // 节点的析构函数
{
    if (this->left != NULL)
    {
        delete this->left;
    }

    if (this->right != NULL)
    {
        delete this->right;
    }
}

template <typename T>
typename RBTree<T>::RBTreeNode* RBTree<T>::RBTreeNode::lookup(const T key)
{
    // 返回类型前的 typename 告诉编译器：RBTree<T>::RBTreeNode 是依赖模板参数 T 的“类型”，
    // 不是静态变量。没有它，编译器在模板尚未实例化时无法正确解析这段语法。
    RBTreeNode* node = this;

    // 保持一个明确的搜索状态 node：目标更大就向右，目标更小就向左。
    // 循环结束只有两种状态：node 指向命中的节点，或为 NULL 表示不存在。
    while (node != NULL && node->key != key)
    {
        node = node->key < key ? node->right : node->left;
    }

    return node;
}

template <typename T>
bool RBTree<T>::RBTreeNode::insert(const T key)
{
    // 第一阶段只按 BST 规则寻找空位；第二阶段由 adjustInsert 恢复红黑性质。
    // 将“结构插入”和“颜色修复”解耦后，每个函数只处理一种约束。
    RBTreeNode* node = this;
    bool nodeInserted = false;

    while (!nodeInserted)
    {
        // 该实现把树当作集合，不保存重复键。
        if (node->key == key) return false;

        if (node->key < key)
        {
            if (node->right == NULL)
            {
                // 新节点染红不会立刻增加任一路径的黑高；代价是可能产生“红父红子”。
                node->right = new RBTreeNode(key, node, tree, RED);
                adjustInsert(node->right);
                nodeInserted = true;
            }
            else
            {
                node = node->right;
            }
        }
        else
        {
            if (node->left == NULL)
            {
                node->left = new RBTreeNode(key, node, tree, RED);
                adjustInsert(node->left);
                nodeInserted = true;
            }
            else
            {
                node = node->left;
            }
        }
    }

    return true;
}
// 为什么我们需要不断往上排查呢？
// 因为查父亲和uncle让它们不断一起被染黑其实就是在不断收敛的过程
// 它保证分叉的两端的黑高相同，如果只染一边就会不同
// 为什么要把祖父染成红色，因为父亲和uncle染成黑色了，我们需要保证在这个分支上黑红对冲
// 如果不怎么做，在更宏观的地方，两条分支的黑高就会不对等，红黑树的设计就失效了
template <typename T>
void RBTree<T>::RBTreeNode::adjustInsert(RBTreeNode* insertNode)
{
    // 插入修复的状态变量 node 表示“当前需要检查红冲突的子树根”。
    // 修复按终止条件、叔叔红、叔叔黑三类处理；只有叔叔红会把问题继续向祖先传播。
    RBTreeNode* node = insertNode;

    while (true)
    {
        if (node->parent == NULL)
        {
            // 情况 1：冲突传播到根。根染黑即可结束；所有路径同时多一个黑节点，黑高仍相等。
            node->color = BLACK;
            return;
        }
        else if (node->parent->color == BLACK)
        {
            // 情况 2：父亲为黑，没有红红冲突，结构和黑高均合法。
            return;
        }
        else
        {
#ifdef DEBUG
            // red nodes always have a parent
            assert(node->parent->parent != NULL);

            // the parent of red nodes is always black
            assert(node->parent->parent->color == BLACK);
#endif

            RBTreeNode* parent = node->parent;
            RBTreeNode* grand = node->parent->parent;
            RBTreeNode* uncle = (grand->left == parent) ? grand->right : grand->left;

            // 情况 3：父亲和叔叔都红。
            // 父、叔染黑，祖父染红；祖父子树内部黑高不变，但祖父可能与其父亲继续冲突。
            if (uncle != NULL && uncle->color == RED)
            {
                parent->color = BLACK;
                uncle->color = BLACK;
                grand->color = RED;

                // adjust the tree for the grand parent
                node = grand;
                continue;
            }
            else
            {
                // 情况 4：父红、叔黑（NULL 也按黑处理）。
                // 若 node-parent-grand 是折线，先旋转父亲把它拉直；随后旋转祖父并交换父祖颜色。

                if (grand->left != NULL && node == grand->left->right)
                {
                    parent->leftRotate();
                    node = node->left;  // 这个很关键
                    // 在父亲发生一次旋转之后，孩子被转了上来成为真正的父亲，成为锚点
                    // 如果旋转之后不把node移到底下的话，下一次旋转就会错乱，而且无法判断后面到底应该左旋还是右旋
                }
                else if (grand->right != NULL && node == grand->right->left)
                {
                    parent->rightRotate();
                    node = node->right;
                }

                // Update pointers after the rotation
                // 为什么需要更新，因为旋转完了之后，parent被转到了下面了，它只是有着parent的名字，但其实不是真正的parent了
                // node的parent才是真正的parent，因为bridge才代表正确的关系，以node为参照进行更改即可
                parent = node->parent;         // 这样从上到下的顺序再次变回grand -> parent -> node
                grand = node->parent->parent;  // 其实这句没有必要写。。

                // The node will not be a subtree of the grandparent
                if (node == parent->left)
                {
                    grand->rightRotate();
                }
                else
                {
                    grand->leftRotate();
                }
                // 旋转之后要父祖换色，或者换色之后再旋转祖父
                parent->color = BLACK;
                grand->color = RED;
            }
        }
    }
}

template <typename T>
void RBTree<T>::RBTreeNode::leftRotate()
{
#ifdef DEBUG
    // the right node will be the new parent
    assert(this->right != NULL);
#endif

    // 左旋终局：this 的右孩子 root 上升，this 下沉为 root 的左孩子。
    // 旋转前：parent -> this -> root，且 root 的左子树夹在 this 与 root 的键之间。
    RBTreeNode* root = this->right;

    // 先收养中间子树，再让 root 收养 this。顺序很重要，否则可能丢失 root->left。
    this->right = root->left;  // 这里的left非常重要，因为它的值的范围被严格限制在了this和root之间
    // 所以可以作为新的根
    // 这里不要有直线的概念，因为直线与否是更大的层面需要考虑的
    // 如果不是直线，则多次旋转，如果是直线，旋转一次就够了
    // 比如RR型对this进行一次左旋即可
    // 此时this的右指针就是nullptr，相当于断开和原来右孩子的连接
    // this原本只有右孩子，断开之后就是啥都没有了，变成叶子节点
    root->left = this;
    root->parent = this->parent;

    // 把原外部父亲指向新的局部根 root；若 this 原来是左孩子，root 仍占左槽位。

    // 这里一定要注意把出现的节点的各个成员都改了
    // 比如this和root的父亲左右孩子
    // 还要注意修改原根this的父亲和它们之间的桥梁，也就是孩子关系
    // 左旋和this的左子树没有关系
    if (this->parent != NULL)
    {
        if (this->parent->left == this)
        {
            this->parent->left = root;
        }
        else
        {
            this->parent->right = root;
        }
    }

    // 中间子树改挂到 this 右侧后，也必须反向更新它的 parent。
    if (this->right != NULL)
    {
        this->right->parent = this;
    }

    this->parent = root;

    // 若旋转发生在总根，外部父亲不存在，必须更新整棵树的 root。
    if (root->parent == NULL)
    {
        tree->root = root;
    }
}

template <typename T>
void RBTree<T>::RBTreeNode::rightRotate()
{
#ifdef DEBUG
    // the left node will be the new parent
    assert(this->left != NULL);
#endif

    // 右旋是左旋的镜像：this 的左孩子 root 上升，this 下沉为 root 的右孩子。
    RBTreeNode* root = this->left;

    this->left = root->right;
    root->right = this;
    root->parent = this->parent;

    // update the child link
    if (this->parent != NULL)
    {
        if (this->parent->left == this)
        {
            this->parent->left = root;
        }
        else
        {
            this->parent->right = root;
        }
    }

    // update the parent link
    if (this->left != NULL)
    {
        this->left->parent = this;
    }

    this->parent = root;

    // set the new root of the tree
    if (root->parent == NULL)
    {
        tree->root = root;
    }
}

template <typename T>
void RBTree<T>::RBTreeNode::remove()
{
    // 删除先把“两个孩子”转化为“至多一个孩子”，再处理颜色亏损。
    // 状态变量 node 才是最终从结构中摘除的节点，可能并不是最初的 this。
    RBTreeNode* node = this;

    if (this->left != NULL && this->right != NULL)
    {
        // For the 2 child case we will convert the problem into 1 or 0 childs
        // Therefore find the minimum element in the right subtree

        node = this->right;

        while (node->left != NULL)
        {
            node = node->left;
        }

        // 后继是右子树最小值。这里只复制 key，并不交换节点位置，可避免大量父子指针修改。
        this->key = node->key;
    }

    // 因为node是最小值了，所以它不可能还有左孩子，它最多只有一个右孩子。
    //  此时 node 至多有一个孩子；三目运算符选择那个非空孩子，二者都空则得到 NULL。
    RBTreeNode* child = (node->left == NULL) ? node->right : node->left;

    // 这里主要是把child和node连起来
    if (node->parent == NULL)
    {
        node->tree->root = child;
    }
    else if (node->parent->left == node)
    {
        node->parent->left = child;
    }
    else
    {
        node->parent->right = child;
    }

    // 反正我们要删除node，那就让node的父亲成为child的父亲
    // 此时node的parent依然连着parent
    if (child != NULL)
    {
        child->parent = node->parent;
    }

    // 删除红节点不会改变黑高；删除黑节点则必须补偿少掉的一个黑色。
    if (node->color == BLACK)
    {
        // When the child is red change the color to black
        if (child != NULL && child->color == RED)
        {
            child->color = BLACK;
        }
        else
        {
            // 被删节点与替代孩子都黑。此实现创建临时 DOUBLE_BLACK 哨兵，把抽象的“黑高亏 1”
            // 变成一个真实节点状态，修复完成后再移除。注意 (T)0 要求 T 能由 0 构造，削弱了泛型性。
            child = new RBTreeNode((T)0, node->parent, node->tree, DOUBLE_BLACK);

            // 把临时双黑节点挂回原槽位，使 adjustRemove 能通过 parent 找到兄弟。
            if (node->parent == NULL)
            {
                node->tree->root = child;
            }
            else if (node->parent->left == NULL)
            {
                node->parent->left = child;
            }
            else
            {
                node->parent->right = child;
            }

            child->adjustRemove();

            // Detach the pseudo node from the tree
            if (child->parent == NULL)
            {
                child->tree->root = NULL;
            }
            else if (child->parent->left == child)
            {
                child->parent->left = NULL;
            }
            else
            {
                child->parent->right = NULL;
            }

            delete child;
        }
    }

    // 节点析构会递归 delete 左右子树。摘除单个 node 前必须断开孩子，避免误删仍在树中的子树。
    node->left = NULL;
    node->right = NULL;
    delete node;
}

template <typename T>
void RBTree<T>::RBTreeNode::adjustRemove()
{
// node 表示当前承担“额外一个黑色”的位置。目标是消除额外黑色，或把它向根传播后在根吸收。
#ifdef DEBUG
    assert(this->color == DOUBLE_BLACK);
#endif

    RBTreeNode* node = this;

    while (true)
    {
        if (node->parent == NULL)
        {
            // node is the root node
            node->color = BLACK;
            return;
        }

        RBTreeNode* parent = node->parent;
        RBTreeNode* sibling = (node == node->parent->left) ? node->parent->right : node->parent->left;

        // 预处理：红兄弟不能直接套用后续黑兄弟规则。
        // 交换父兄颜色并旋转，把局面转化为“兄弟为黑”的标准形态。
        if (sibling->color == RED)
        {
            sibling->color = BLACK;
            parent->color = RED;

            if (node == parent->left)
            {
                parent->leftRotate();
                sibling = parent->right;
            }
            else
            {
                parent->rightRotate();
                sibling = parent->left;
            }
        }

        // 父黑、兄黑、兄弟两子也黑：兄弟染红可抵消 node 的额外黑，但父亲因此成为新的双黑位置。
        if (parent->color == BLACK && (sibling->left == NULL || sibling->left->color == BLACK) &&
            (sibling->right == NULL || sibling->right->color == BLACK))
        {
            sibling->color = RED;
            node = parent;
            continue;
        }

        // 父红、兄黑、兄弟两子黑：父亲的红可直接补偿亏损，父染黑、兄染红后结束。
        if (parent->color == RED && (sibling->left == NULL || sibling->left->color == BLACK) &&
            (sibling->right == NULL || sibling->right->color == BLACK))
        {
            sibling->color = RED;
            parent->color = BLACK;
            return;
        }

        // 近侄红、远侄黑：先旋转兄弟，把近侄变成新的黑兄弟，为最终旋转做准备。
        if (node == parent->left && (sibling->right == NULL || sibling->right->color == BLACK))
        {
            sibling->color = RED;
            sibling->left->color = BLACK;
            sibling->rightRotate();
            sibling = sibling->parent;

            // Black sibling with the siblings right child red
        }
        else if (node == parent->right && (sibling->left == NULL || sibling->left->color == BLACK))
        {
            sibling->color = RED;
            sibling->right->color = BLACK;
            sibling->leftRotate();
            sibling = sibling->parent;
        }

        // 最终形态：黑兄弟的远侄为红。围绕父亲旋转，并重新着色，一次消除双黑。
        sibling->color = parent->color;
        parent->color = BLACK;

        if (node == parent->left)
        {
            parent->leftRotate();
            sibling->right->color = BLACK;
        }
        else
        {
            parent->rightRotate();
            sibling->left->color = BLACK;
        }
        return;
    }
}

#ifdef DEBUG
template <typename T>
bool RBTree<T>::RBTreeNode::invariant()
{
    // 验证器把“我觉得修好了”转成可重复检查的客观条件。
    // 它检查红节点孩子、局部 BST 顺序、左右黑高，并递归检查所有后代。
    // If a node is red then both children are black
    bool invColor =
        (color == BLACK) || ((left == NULL || left->color == BLACK) && (right == NULL || right->color == BLACK));

    // Left nodes have a lower order and right nodes a higher order
    bool invOrder = (left == NULL || left->key < this->key) && (right == NULL || right->key > this->key);

    // Every path to a leaf node contains the same number of black nodes
    bool blackNodeCount = invariantBlackNodes() > -1;

    return invColor && invOrder && blackNodeCount && (left == NULL || left->invariant()) &&
           (right == NULL || right->invariant());
}

template <typename T>
int RBTree<T>::RBTreeNode::invariantBlackNodes()
{
    // Empty Nodes will be treated as black nodes
    int leftCount = (this->left == NULL) ? 1 : this->left->invariantBlackNodes();

    int rightCount = (this->right == NULL) ? 1 : this->right->invariantBlackNodes();

    // when the black node count differs -1 will be returned
    return (leftCount == rightCount && leftCount != -1) ? leftCount + this->color : -1;
}

template <typename T>
void RBTree<T>::RBTreeNode::toString(ostream& buffer, const string& prefix, bool lastNode)
{
    // print the current element and the children
    buffer << prefix << (lastNode ? "└── " : "├── ") << key << (color == RED ? " (R)" : " (B)") << endl;

    if (left != NULL)
    {
        left->toString(buffer, prefix + (lastNode ? "    " : "│   "), right == NULL);
    }

    if (right != NULL)
    {
        right->toString(buffer, prefix + (lastNode ? "    " : "│   "), true);
    }
}

template <typename T>
void RBTree<T>::RBTreeNode::dumpNode(ofstream& graphFile)
{
    graphFile << "\"" << key << "\" " << "[shape=circle, style=filled, fillcolor=";

    switch (color)
    {
        case RED:
            graphFile << "\"#EB0000\"";
            break;

        case BLACK:
            graphFile << "black";
            break;

        case DOUBLE_BLACK:
            graphFile << "black, peripheries=2";
            break;

        default:
            graphFile << "azure4";
    }

    graphFile << "]" << endl;

    if (left != NULL)
    {
        graphFile << key << " -> " << left->key << endl;
        left->dumpNode(graphFile);
    }

    if (right != NULL)
    {
        graphFile << key << " -> " << right->key << endl;
        right->dumpNode(graphFile);
    }
}
#endif

// tree
template <typename T>
RBTree<T>::RBTree()
{
    // 空树唯一状态：root == NULL。
    this->root = NULL;
}

template <typename T>
RBTree<T>::~RBTree()
{
    if (root != NULL)
    {
        delete root;
    }
}

template <typename T>
bool RBTree<T>::contains(const T key)
{
    // RBTree 负责空树边界；非空搜索委托给节点，实现接口层与算法层分工。
    if (root == NULL)
    {
        return false;
    }
    else
    {
        return root->lookup(key) != NULL;
    }
}

template <typename T>
bool RBTree<T>::insert(const T key)
{
    if (root == NULL)
    {
        root = new RBTreeNode(key, this);
        return true;
    }

    return root->insert(key);
}

template <typename T>
bool RBTree<T>::remove(const T key)
{
    if (root == NULL)
    {
        return false;
    }
    else
    {
        RBTreeNode* node = root->lookup(key);

        if (node == NULL)
        {
            return false;
        }
        else
        {
            node->remove();
            return true;
        }
    }
}

template <typename T>
std::vector<typename RBTree<T>::VisualizationNode> RBTree<T>::visualization_snapshot() const
{
    std::vector<VisualizationNode> snapshot;

    if (root == NULL)
    {
        return snapshot;
    }

    // work_nodes 与 snapshot 使用相同下标：
    // work_nodes 保存真实节点，仅在本函数内部短暂使用；
    // snapshot 保存复制后的安全数据，交给界面长期使用。
    std::vector<RBTreeNode*> work_nodes;
    work_nodes.push_back(root);
    snapshot.push_back(VisualizationNode(root->key, root->isBlack(), -1));

    for (std::size_t index = 0; index < work_nodes.size(); ++index)
    {
        RBTreeNode* current_node = work_nodes[index];

        if (current_node->left != NULL)
        {
            int child_index = static_cast<int>(work_nodes.size());
            work_nodes.push_back(current_node->left);
            snapshot.push_back(
                VisualizationNode(current_node->left->key, current_node->left->isBlack(), static_cast<int>(index)));
            snapshot[index].left_index = child_index;
        }

        if (current_node->right != NULL)
        {
            int child_index = static_cast<int>(work_nodes.size());
            work_nodes.push_back(current_node->right);
            snapshot.push_back(
                VisualizationNode(current_node->right->key, current_node->right->isBlack(), static_cast<int>(index)));
            snapshot[index].right_index = child_index;
        }
    }

    return snapshot;
}

// iterator
template <typename T>
typename RBTree<T>::iterator& RBTree<T>::iterator::operator++()
{
    // 注意：这个迭代器执行的是“后序遍历”，并非 std::set 常见的升序中序遍历。
    // 因而它只保证每个元素访问一次，不保证输出有序。
    RBTreeNode* node = this->node;

    // The root node is the last element
    if (node->parent == NULL)
    {
        this->node = NULL;
        return *this;
    }

    // Switch to the right sibling or bubble up in the tree
    if (node == node->parent->left && node->parent->right != NULL)
    {
        node = node->parent->right;
    }
    else
    {
        this->node = node->parent;
        return *this;
    }

    // Descend to the next leaf node
    while (true)
    {
        if (node->left != NULL)
        {
            node = node->left;
        }
        else if (node->right != NULL)
        {
            node = node->right;
        }
        else
        {
            this->node = node;
            return *this;
        }
    }
}

template <typename T>
typename RBTree<T>::iterator RBTree<T>::begin()
{
    // 后序遍历的第一个节点是从根尽量向左、再向右下降得到的叶子，不一定是最小键。
    RBTreeNode* node = root;

    if (node != NULL)
    {
        while (node->left != NULL)
        {
            node = node->left;
        }

        while (node->right != NULL)
        {
            node = node->right;
        }
    }

    return iterator(node);
}

template <typename T>
typename RBTree<T>::iterator RBTree<T>::end()
{
    return iterator(NULL);
}

#ifdef DEBUG
template <typename T>
bool RBTree<T>::invariant()
{
    // The root is empty or black
    return root == NULL || (root->isBlack() && root->invariant());
}

template <typename T>
void RBTree<T>::dumpTree(string dumpName)
{
    system("mkdir -p dump");
    ofstream graphFile;
    graphFile.open("dump/" + dumpName + ".gv");

    graphFile << "digraph G {" << endl;
    graphFile << "node [style=filled, penwidth=2, fontcolor=white, fontname=\"Arial Black\"];" << endl;
    graphFile << "graph [pad=\"0.1\", nodesep=\"1\", ranksep=\"1.5\"];" << endl;

    if (root != NULL)
    {
        root->dumpNode(graphFile);
    }

    graphFile << "}" << endl;
    graphFile.close();

    string graphizCall = "dot -Tpng dump/" + dumpName + ".gv -o dump/" + dumpName + ".png";
    string openCall = "xdg-open dump/" + dumpName + ".png";

    system(graphizCall.c_str());
    system("cd dump && rm *.gv");
    system(openCall.c_str());
}

template <typename T>
string RBTree<T>::toString()
{
    stringstream buffer;

    if (root == NULL)
    {
        buffer << "empty tree";
    }
    else
    {
        root->toString(buffer, "", true);
    }

    return buffer.str();
}
#endif

#endif /* RBTREE_H */
