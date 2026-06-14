#include "interface.h"

class TwoLevelTable : ITaggingImpl {
public:
    TwoLevelTable();
    ~TwoLevelTable();

    void  tag_alloc(void* ptr, size_t size, uint8_t tag) override;
    bool  check_tag(void* ptr, uint8_t expected_tag) override;
    void  free_alloc(void* ptr) override;
    size_t get_overhead_bytes() const override;
    std::string name() const override { return "TwoLevelTable"; }

private:

};