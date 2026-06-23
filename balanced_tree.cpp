#include "memory.h"

struct BTreeNode {
    uint64_t base_addr; // key - start of allocation 3. pointer size? pointer acesses
    size_t size; // how many bytes this allocation covers
    uint8_t  tag; // the tag for this allocation
    int height; // for AVL balancing
    BTreeNode* left;
    BTreeNode* right;

	BTreeNode(uint64_t addr, size_t sz, uint8_t t) : base_addr(addr), size(sz), tag(t), height(1), left(nullptr), right(nullptr) {}
};

class BalancedTree : public Memory {
public:
	BalancedTree(size_t mem_size) : Memory(mem_size), root_(nullptr) {}

	~BalancedTree() { destroy(root_); }

	uint64_t alloc(size_t size, uint8_t tag) override {
		uint64_t addr = Memory::alloc(size, tag);
		if (addr == INVALID) return INVALID;
		root_ = insert(root_, addr, size, tag);

        // update peak after every insert
        size_t current = get_overhead_bytes();
        if (current > stats_.peak_overhead_bytes)
            stats_.peak_overhead_bytes = current;

		return addr;
	}

	void free(uint64_t addr) override {
		root_ = remove(root_, addr);
		Memory::free(addr);
	}

	bool check_tag(uint64_t addr, uint8_t expected) override {
		BTreeNode* node = find(root_, addr);
		stats_.tag_accesses++;
		stats_.total_accesses++;
		return node && node->tag == expected;
	}

	size_t get_overhead_bytes() override {
		return count_nodes(root_) * sizeof(BTreeNode);
	}

    void print_overhead_bytes() {
        std::cout << "Overhead bytes: " << get_overhead_bytes() << "\n";
    }
	std::string name() const override { return "BalancedTree"; }
	AccessStats get_stats() const override { return stats_; }
    void reset_stats() override { stats_ = AccessStats(); }

private:
    BTreeNode* root_;

    int height(BTreeNode* node) {
        stats_.tag_accesses++;
        stats_.total_accesses++;
        return node ? node->height : 0;
    }

    void update_height(BTreeNode* node) {
        stats_.tag_accesses++;
        stats_.total_accesses++;
        if (node)
            node->height = 1 + std::max(height(node->left), height(node->right));
    }

    int balance_factor(BTreeNode* node) {
        return node ? height(node->left) - height(node->right) : 0;
    }

    BTreeNode* rotate_right(BTreeNode* b) {
        BTreeNode* a  = b->left;
        BTreeNode* y  = a->right; 

        a->right = b;
        b->left  = y;

        update_height(b); // Update height of b first since it's now a child of a
        update_height(a); 

        stats_.tag_accesses += 2;
        stats_.total_accesses += 2;

        return a;
    }

    BTreeNode* rotate_left(BTreeNode* a) {
        BTreeNode* b  = a->right;
        BTreeNode* y  = b->left;

        b->left  = a;
        a->right = y;

        update_height(a);
        update_height(b);

        stats_.tag_accesses += 2;
        stats_.total_accesses += 2;

        return b;
    }

    BTreeNode* rebalance(BTreeNode* node) {
        update_height(node);
        int bf = balance_factor(node);

        // Left heavy
        if (bf > 1) {
            // Left-right case: double rotation needed
            if (balance_factor(node->left) < 0) {
                node->left = rotate_left(node->left);
                stats_.tag_accesses++;
                stats_.total_accesses++;
            }
            return rotate_right(node);
        }

        // Right heavy
        if (bf < -1) {
            // Right-left case: double rotation needed
            if (balance_factor(node->right) > 0) {  
                node->right = rotate_right(node->right);
                stats_.tag_accesses++;
                stats_.total_accesses++;
            }
            return rotate_left(node);
        }

        return node;
    }

    BTreeNode* insert(BTreeNode* node, uint64_t base_addr, size_t size, uint8_t tag) {
        if (!node)
            return new BTreeNode(base_addr, size, tag);

        if (base_addr < node->base_addr) {
            node->left = insert(node->left,  base_addr, size, tag);
            stats_.tag_accesses++;
            stats_.total_accesses++;
        } else if (base_addr > node->base_addr) {
            node->right = insert(node->right, base_addr, size, tag);
            stats_.tag_accesses++;
            stats_.total_accesses++;
        }

        return rebalance(node);
    }

    BTreeNode* detach_min(BTreeNode* node, BTreeNode*& min_out) {
        if (!node->left) {
            min_out = node;
            return node->right;
        }
        node->left = detach_min(node->left, min_out);
        stats_.tag_accesses++;
        stats_.total_accesses++;
        return rebalance(node);
    }

    BTreeNode* remove(BTreeNode* node, uint64_t base_addr) {
        if (!node) return nullptr;

        if (base_addr < node->base_addr) {
            node->left  = remove(node->left,  base_addr);
            stats_.tag_accesses++;
            stats_.total_accesses++;
        } else if (base_addr > node->base_addr) {
            node->right = remove(node->right, base_addr);
            stats_.tag_accesses++;
            stats_.total_accesses++;
        } else {

            // Case 1: leaf node
            if (!node->left && !node->right) {
                stats_.tag_accesses += 2;
                stats_.total_accesses += 2;
                delete node;
                return nullptr;
            }

            // Case 2: one child
            if (!node->left) {
                BTreeNode* right = node->right;
                stats_.tag_accesses++;
                stats_.total_accesses++;
                delete node;
                return right;
            }
            if (!node->right) {
                BTreeNode* left = node->left;
                stats_.tag_accesses++;
                stats_.total_accesses++;
                delete node;
                return left;
            }

            // Case 3: two children replace this node's data with its in-order successor (smallest node in right subtree), then remove that successor
            BTreeNode* successor  = nullptr;
            node->right = detach_min(node->right, successor);

            // Copy successor's allocation data into current node
            node->base_addr = successor->base_addr;
            node->size = successor->size;
            node->tag = successor->tag;

            stats_.tag_accesses += 4;
            stats_.total_accesses += 4;

            delete successor;
        }

        return rebalance(node);
    }

    BTreeNode* find(BTreeNode* node, uint64_t addr) {
        if (!node) return nullptr;

        stats_.tag_accesses += 2;
        stats_.total_accesses += 2;

        if (addr >= node->base_addr && addr <  node->base_addr + node->size)
            return node;

        stats_.tag_accesses++;
        stats_.total_accesses++;
        
        if (addr < node->base_addr)
            return find(node->left,  addr);

        return find(node->right, addr);
    }

    int count_nodes(BTreeNode* node) {
        if (!node) return 0;
        stats_.tag_accesses += 2;
        stats_.total_accesses += 2;
        return 1 + count_nodes(node->left) + count_nodes(node->right);
    }

    void destroy(BTreeNode* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);

        stats_.tag_accesses += 2;
        stats_.total_accesses += 2;

        delete node;
    }
};