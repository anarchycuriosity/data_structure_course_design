// ---------------------------------------------------------------------
// MIT License
// Copyright (c) 2017 Henrik Peters
// See LICENSE file in the project root for full license information.
// ---------------------------------------------------------------------
#ifndef RBTREE_H
#define RBTREE_H

#include <iterator>
#include <vector>

// 这份代码为什么声明和实现全部写在头文件里？
// 因为RBTree不是一个已经确定类型的类，而是一张等待T被替换的模板。
// 当外面写RBTree<int>时，编译器才开始制造int版本的树，这时它必须同时看见成员函数的完整实现。
// 如果只把声明留在这里、把模板实现藏进普通cpp，编译器制造RBTree<int>时就会找不到函数体。
//
// 阅读时先不要同时啃颜色修复和模板语法，可以按这个顺序：
// 1. contains、insert、remove：先看使用者能做什么。
// 2. lookup和普通insert：先把它当普通二叉搜索树。
// 3. leftRotate、rightRotate：理解父子指针怎么换位置。
// 4. adjustInsert、adjustRemove：最后看颜色为什么需要跟着结构一起修复。

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
    // 为什么节点类放在RBTree内部而且还是private？
    // 因为外面真正需要的是“插入一个值”和“删除一个值”，不是随便改某个节点的颜色和孩子。
    // 一旦外部能直接写node->left或者node->color，红黑树的五条性质就完全无法保证。
    class RBTreeNode
    {
       private:
        enum Color
        {
            RED = 0,
            BLACK = 1,
            DOUBLE_BLACK = 2,
        };

        T key;        // T虽然可以换成别的类型，但是这个类型必须能进行==、<、>比较
        Color color;  // 颜色不是界面颜色，它是判断黑高和红红冲突时真正参与算法的状态

        RBTreeNode* parent;  // 修复过程需要不断往祖先走，所以节点必须知道自己的父亲
        RBTreeNode* left;
        RBTreeNode* right;
        // 为什么每个节点还要保存tree？
        // 普通旋转只改局部父子关系，但是当被旋转的节点正好是总根时，还必须修改RBTree自己的root。
        // 节点拿着所属树的地址，旋转到总根时才能写tree->root。
        RBTree<T>* tree;

       public:
        RBTreeNode(const T key, RBTree<T>* tree);
        RBTreeNode(const T key, RBTreeNode* parent, RBTree<T>* tree, Color color);
        // 这里的virtual不是红黑树算法需要的。
        // 只有通过基类指针删除派生类对象时，虚析构才是必须的；当前节点没有继承层次，所以它属于上游保留写法。
        virtual ~RBTreeNode();

        // friend不是继承，它只是允许指定的类直接访问本类private成员。
        // RBTree需要读写节点的key、color和指针，iterator也需要读取key，所以这里单独给它们权限。
        friend class RBTree<T>;
        friend class iterator;

#ifdef DEBUG
        bool invariant();
        int invariantBlackNodes();
        void toString(ostream& buffer, const string& prefix, bool lastNode);
        void dumpNode(ofstream& graphFile);
#endif

        // isBlack后面的const是在承诺：这个检查只看颜色，不会修改当前节点。
        // inline不等于强制把函数展开，它更重要的作用是允许这种定义放在头文件中被多个cpp包含。
        inline bool isBlack() const { return (this->color == BLACK); }
        inline void adjustInsert(RBTreeNode* insertNode);
        inline void adjustRemove();
        inline void leftRotate();
        inline void rightRotate();

        RBTreeNode* lookup(const T key);
        bool insert(const T key);
        void remove();
    }* root;  // 这里看起来很挤，其实就是结束RBTreeNode类之后，顺便声明RBTreeNode* root

   public:
    // 为什么画图还需要一个VisualizationNode？
    // 因为真正的RBTreeNode是private，直接公开它虽然省事，但是前端也会获得修改颜色和指针的能力。
    // 这里把键值、颜色、父子关系复制成下标，相当于给前端一张照片。
    // 照片可以看出树长什么样，但是不能反过来改动真正的树。
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
    // RBTree当前也没有作为基类使用，所以这里的virtual同样不是算法必需，只是保留上游接口。
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
        // 这些typedef是在告诉标准库：这个迭代器指向什么类型、解引用得到什么、属于哪一级迭代器。
        typedef T value_type;
        typedef const T& reference;
        typedef const T* pointer;
        typedef std::input_iterator_tag iterator_category;
        friend class RBTree<T>;

        // explicit的作用是禁止RBTreeNode*在不知不觉中变成iterator。
        // 加上它以后必须明确写iterator(node_ptr)，不能写iterator it = node_ptr。
        // node(_node)是成员初始化列表，构造函数体还没开始执行时，node就已经初始化完成。
        explicit iterator(RBTreeNode* _node) : node(_node) {}
        // 这里没有手写拷贝构造函数，所以编译器会生成默认拷贝构造函数。

        // operator++让自定义iterator也可以写++it和it++。
        // 前置++直接移动自己并返回自己；后置++必须先保留旧副本，所以通常多一次复制。
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

// 下面开始写RBTreeNode每个成员函数的实现。
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
    // 为什么返回类型前还要写typename？
    // RBTree<T>::RBTreeNode依赖T，在T还没确定时，编译器不能肯定后面这个名字代表类型还是静态成员。
    // typename就是提前告诉编译器：别把它当变量或者乘法表达式，它确定是一个类型。
    RBTreeNode* node = this;

    // node表示我现在站在哪个节点。
    // key更大就往右走，key更小就往左走；循环结束时要么正好找到，要么走到了NULL。
    while (node != NULL && node->key != key)
    {
        node = node->key < key ? node->right : node->left;
    }

    return node;
}

template <typename T>
bool RBTree<T>::RBTreeNode::insert(const T key)
{
    // 插入被拆成两件事：
    // 先完全按照二叉搜索树规则找到空位，再让adjustInsert处理颜色和旋转。
    // 如果边找位置边修颜色，当前节点到底代表“搜索位置”还是“冲突位置”会非常混乱。
    RBTreeNode* node = this;
    bool nodeInserted = false;

    while (!nodeInserted)
    {
        // key相同就返回false，因为这棵树按集合处理，不保存重复值。
        if (node->key == key) return false;

        if (node->key < key)
        {
            if (node->right == NULL)
            {
                // 为什么新节点先染红？
                // 染黑会让这一条路径立刻多一个黑节点；染红不会改变黑高，只可能制造红父红子的局部冲突。
                // 局部冲突可以通过换色和旋转修复，比整条路径黑高失衡更容易控制。
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
    // node不是永远指最初插入的节点，它表示“当前这一轮需要检查冲突的位置”。
    // 叔叔为红时问题会被换色推到祖父，所以node也要跟着向上移动。
    RBTreeNode* node = insertNode;

    while (true)
    {
        if (node->parent == NULL)
        {
            // node已经走到根就直接染黑。
            // 根出现在所有路径上，所以所有路径同时增加一个黑节点，彼此黑高仍然相等。
            node->color = BLACK;
            return;
        }
        else if (node->parent->color == BLACK)
        {
            // 父亲是黑色时没有红红相连，新节点又没有改变黑高，所以不用继续修。
            return;
        }
        else
        {
#ifdef DEBUG
            // 当前父亲是红色，而根一定是黑色，所以红色父亲上面一定还存在祖父。
            assert(node->parent->parent != NULL);

            // 修复开始前树是合法的，因此这个红色父亲的父亲原本一定是黑色。
            assert(node->parent->parent->color == BLACK);
#endif

            RBTreeNode* parent = node->parent;
            RBTreeNode* grand = node->parent->parent;
            RBTreeNode* uncle = (grand->left == parent) ? grand->right : grand->left;

            // 父亲和叔叔都红时，两边可以一起染黑，保证左右同时增加一个黑节点。
            // 祖父再染红抵消这一层增加的黑色，但是祖父可能和更上面的红色父亲发生新冲突。
            if (uncle != NULL && uncle->color == RED)
            {
                parent->color = BLACK;
                uncle->color = BLACK;
                grand->color = RED;

                // 冲突已经被推到祖父，下一轮应该从祖父继续检查。
                node = grand;
                continue;
            }
            else
            {
                // 叔叔为黑时不能只换色，否则左右黑高会不一样，所以必须借助旋转改变结构。
                // 如果node、parent、grand形成折线，就先旋转parent把折线拉成直线，再旋转grand。

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

                // 前一次旋转已经改变了身份，所以这里不能继续相信旧的parent和grand变量。
                // 为什么需要更新，因为旋转完了之后，parent被转到了下面了，它只是有着parent的名字，但其实不是真正的parent了
                // node的parent才是真正的parent，因为bridge才代表正确的关系，以node为参照进行更改即可
                parent = node->parent;         // 这样从上到下的顺序再次变回grand -> parent -> node
                grand = node->parent->parent;  // 其实这句没有必要写。。

                // node在parent左边说明最终是LL形，反之就是RR形。
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
    // 左旋要求右孩子存在，因为右孩子马上要上升成这一小块的新根。
    assert(this->right != NULL);
#endif

    // 左旋最后想得到的结构是：右孩子root上升，this下沉到root左边。
    // root原来的左子树数值在this和root之间，所以它只能改挂到this右边，不能丢掉。
    RBTreeNode* root = this->right;

    // 一定先让this接住root的左子树，再让root的left改成this。
    // 如果顺序反过来，原来的root->left已经被覆盖，中间子树就再也找不到了。
    this->right = root->left;  // 这里的left非常重要，因为它的值的范围被严格限制在了this和root之间
    // 所以可以作为新的根
    // 这里不要有直线的概念，因为直线与否是更大的层面需要考虑的
    // 如果不是直线，则多次旋转，如果是直线，旋转一次就够了
    // 比如RR型对this进行一次左旋即可
    // 此时this的右指针就是nullptr，相当于断开和原来右孩子的连接
    // this原本只有右孩子，断开之后就是啥都没有了，变成叶子节点
    root->left = this;
    root->parent = this->parent;

    // 局部里面旋转完还不够，外面的父亲原来指向this，现在必须改成指向root。

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

    // 中间子树虽然被this接住了，但是它自己的parent还指着旧位置，所以反向指针也要补上。
    if (this->right != NULL)
    {
        this->right->parent = this;
    }

    this->parent = root;

    // root没有父亲说明这次旋转发生在整棵树顶端，此时还必须修改tree->root。
    if (root->parent == NULL)
    {
        tree->root = root;
    }
}

template <typename T>
void RBTree<T>::RBTreeNode::rightRotate()
{
#ifdef DEBUG
    // 右旋要求左孩子存在，因为左孩子马上要上升成这一小块的新根。
    assert(this->left != NULL);
#endif

    // 右旋完全是左旋的镜像：左孩子root上升，this下沉到root右边。
    RBTreeNode* root = this->left;

    this->left = root->right;
    root->right = this;
    root->parent = this->parent;

    // 外部父亲原来指向this，旋转后应该改为指向新的局部根root。
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

    // root原来的右子树现在挂到this左边，它自己的parent也要改成this。
    if (this->left != NULL)
    {
        this->left->parent = this;
    }

    this->parent = root;

    // 如果新局部根已经没有父亲，说明它同时也是整棵树的新根。
    if (root->parent == NULL)
    {
        tree->root = root;
    }
}

template <typename T>
void RBTree<T>::RBTreeNode::remove()
{
    // 删除两个孩子的节点很麻烦，所以先找后继，把问题转成删除“至多一个孩子”的节点。
    // node表示最后真正从指针结构里摘掉的节点，它不一定还是最初收到remove的this。
    RBTreeNode* node = this;

    if (this->left != NULL && this->right != NULL)
    {
        // 两个孩子时去右子树找最小值，也就是中序遍历中的下一个节点。

        node = this->right;

        while (node->left != NULL)
        {
            node = node->left;
        }

        // 这里只把后继的key复制到this，不交换两个完整节点。
        // 如果交换节点，parent、left、right和颜色都要一起处理，指针关系会复杂很多。
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

    // 红节点不计入黑高，所以删红节点不会让路径少黑色。
    // 删黑节点才需要考虑怎样把少掉的黑色补回来。
    if (node->color == BLACK)
    {
        // 黑节点只有一个红孩子时，让红孩子染黑就正好补回被删除的一个黑色。
        if (child != NULL && child->color == RED)
        {
            child->color = BLACK;
        }
        else
        {
            // 被删节点是黑色，替代位置又没有红孩子可以直接补偿，这条路径就少了一个黑色。
            // 代码创建一个临时DOUBLE_BLACK节点，把“这里欠一个黑色”变成真正能沿父指针移动的状态。
            // 注意(T)0要求T可以由0构造，所以这个实现并不是对任意类型都完全通用。
            child = new RBTreeNode((T)0, node->parent, node->tree, DOUBLE_BLACK);

            // 临时节点必须挂回被删除节点原来的位置。
            // 只有这样adjustRemove才能通过parent判断自己是左孩子还是右孩子，并找到真正的兄弟。
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

            // 双黑修复结束以后，临时节点已经没有意义，需要再从树上摘掉。
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

    // RBTreeNode析构时会继续delete左右子树。
    // 这里只想删除node自己，所以必须先把left和right断开，否则仍留在树中的孩子也会被递归删除。
    node->left = NULL;
    node->right = NULL;
    delete node;
}

template <typename T>
void RBTree<T>::RBTreeNode::adjustRemove()
{
// node表示当前哪一个位置背着“额外一个黑色”。
// 修复要么在局部把这个额外黑色抵消，要么继续把它往父亲方向推，最后由根吸收。
#ifdef DEBUG
    assert(this->color == DOUBLE_BLACK);
#endif

    RBTreeNode* node = this;

    while (true)
    {
        if (node->parent == NULL)
        {
            // 双黑已经推到根时，根直接恢复成普通黑色就可以结束。
            node->color = BLACK;
            return;
        }

        RBTreeNode* parent = node->parent;
        RBTreeNode* sibling = (node == node->parent->left) ? node->parent->right : node->parent->left;

        // 兄弟为红时，它的孩子一定是黑色。
        // 先交换父亲和兄弟颜色并旋转，目的不是直接结束，而是把局面转成后面统一处理的黑兄弟。
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

        // 父亲、兄弟和两个侄子都是黑色时，兄弟染红相当于兄弟那条路径少一个黑色。
        // 两边暂时重新相等，但是亏损被推到了父亲，所以node要继续向上走。
        if (parent->color == BLACK && (sibling->left == NULL || sibling->left->color == BLACK) &&
            (sibling->right == NULL || sibling->right->color == BLACK))
        {
            sibling->color = RED;
            node = parent;
            continue;
        }

        // 父亲是红色时就有一个可以直接拿来补偿的颜色。
        // 父亲染黑、兄弟染红之后，两边黑高重新相等，而且不需要继续往上推。
        if (parent->color == RED && (sibling->left == NULL || sibling->left->color == BLACK) &&
            (sibling->right == NULL || sibling->right->color == BLACK))
        {
            sibling->color = RED;
            parent->color = BLACK;
            return;
        }

        // 近侄红、远侄黑还不能直接围绕父亲做最终旋转。
        // 先旋转兄弟，把红色近侄送到外侧，转成远侄为红的标准情况。
        if (node == parent->left && (sibling->right == NULL || sibling->right->color == BLACK))
        {
            sibling->color = RED;
            sibling->left->color = BLACK;
            sibling->rightRotate();
            sibling = sibling->parent;

            // 当前分支处理的是镜像情况：黑兄弟的右孩子为红。
        }
        else if (node == parent->right && (sibling->left == NULL || sibling->left->color == BLACK))
        {
            sibling->color = RED;
            sibling->right->color = BLACK;
            sibling->leftRotate();
            sibling = sibling->parent;
        }

        // 黑兄弟的远侄为红时已经到最终形态。
        // 围绕父亲旋转，再让新的局部根继承原父亲颜色，就能一次把双黑消掉。
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
    // invariant不是修复函数，它只是检查现在这棵树到底合不合法。
    // 红节点不能连红孩子、左小右大、左右黑高相同，这三类条件都满足后再递归检查孩子。
    // 如果当前节点是红色，它的两个非空孩子都必须是黑色。
    bool invColor =
        (color == BLACK) || ((left == NULL || left->color == BLACK) && (right == NULL || right->color == BLACK));

    // 左孩子必须比自己小，右孩子必须比自己大。
    bool invOrder = (left == NULL || left->key < this->key) && (right == NULL || right->key > this->key);

    // 左右两边到叶子的黑节点数量必须相同。
    bool blackNodeCount = invariantBlackNodes() > -1;

    return invColor && invOrder && blackNodeCount && (left == NULL || left->invariant()) &&
           (right == NULL || right->invariant());
}

template <typename T>
int RBTree<T>::RBTreeNode::invariantBlackNodes()
{
    // NULL叶子按照红黑树定义也算黑色，所以空位置的黑高从1开始。
    int leftCount = (this->left == NULL) ? 1 : this->left->invariantBlackNodes();

    int rightCount = (this->right == NULL) ? 1 : this->right->invariantBlackNodes();

    // 左右黑高不同就返回-1，让错误状态一路向上传播。
    return (leftCount == rightCount && leftCount != -1) ? leftCount + this->color : -1;
}

template <typename T>
void RBTree<T>::RBTreeNode::toString(ostream& buffer, const string& prefix, bool lastNode)
{
    // 先输出自己，再递归输出左右孩子，prefix负责保留树枝缩进。
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

// 下面开始实现RBTree本身，不再是单个节点的内部操作。
template <typename T>
RBTree<T>::RBTree()
{
    // 新树没有任何节点，所以唯一需要建立的状态就是root为NULL。
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
    // 空树没有节点可以调用lookup，所以RBTree先处理root为NULL。
    // 非空时再把真正的向下搜索交给根节点。
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

    // work_nodes和snapshot为什么必须共用同一个下标？
    // 因为snapshot不能保存真实指针，只能用数字表示父子关系。
    // 当真实节点在work_nodes下标为3时，它复制出来的数据也放到snapshot[3]，这样孩子只需要记住数字3。
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

// 下面实现迭代器。这个迭代器使用后序遍历，不是std::set那种升序遍历。
template <typename T>
typename RBTree<T>::iterator& RBTree<T>::iterator::operator++()
{
    // 当前node表示迭代器现在停在哪里。
    // 后序遍历顺序是左子树、右子树、根，所以它只保证每个节点访问一次，不保证键值升序。
    RBTreeNode* node = this->node;

    // 后序遍历最后才访问根，所以走到没有父亲的根以后，下一个位置就是end。
    if (node->parent == NULL)
    {
        this->node = NULL;
        return *this;
    }

    // 如果刚走完父亲的左子树而且右兄弟存在，就转去右子树；否则说明父亲该被访问了。
    if (node == node->parent->left && node->parent->right != NULL)
    {
        node = node->parent->right;
    }
    else
    {
        this->node = node->parent;
        return *this;
    }

    // 进入右子树以后继续尽量向左、再向右下降，找到下一棵子树最先访问的叶子。
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
    // begin要找后序遍历第一个节点，所以从根开始尽量向左走。
    // 如果某一层没有左孩子但有右孩子，就继续走右边，直到落到叶子。
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
    // 整棵树为空时天然合法；非空时先保证根为黑，再递归检查所有节点。
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
