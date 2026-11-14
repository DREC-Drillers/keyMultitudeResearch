#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <tree/bTree.hpp>
#include <vector>

#include <gtest/gtest.h>

namespace {

struct Record {
    explicit Record(int data, int identity = -1) : value(data), tag(identity) {}
    int value;
    int tag;
};

struct ByValueDescending {
    bool operator()(const Record& left, const Record& right) const {
        return left.value > right.value;
    }
};

TEST(BTreeQueriesTest, ComparatorEquivalenceAndEveryStoredRecord) {
    general_functions::BTree<Record, 2, ByValueDescending> tree;
    std::vector<Record> reference;
    EXPECT_EQ(tree.countKeysLessThan(Record(0)), 0U);
    EXPECT_EQ(tree.countKeysLessThanOrEqual(Record(0)), 0U);
    EXPECT_THROW(tree.getKeyBySortedIndex(0), std::out_of_range);

    for (int index = 0; index < 400; ++index) {
        const Record key(index % 13, index);
        tree.insert(key);
        reference.push_back(key);
    }

    ASSERT_TRUE(tree.validateInvariants());
    std::sort(reference.begin(), reference.end(), ByValueDescending{});
    std::vector<int> tags;

    for (std::size_t index = 0; index < reference.size(); ++index) {
        const auto& actual = tree.getKeyBySortedIndex(index);
        ASSERT_EQ(actual.value, reference[index].value);
        tags.push_back(actual.tag);
    }

    std::sort(tags.begin(), tags.end());

    for (std::size_t index = 0; index < tags.size(); ++index) {
        EXPECT_EQ(tags[index], static_cast<int>(index));
    }

    for (int value = -1; value <= 13; ++value) {
        const auto less =
            std::count_if(reference.begin(),
                          reference.end(),
                          [value](const Record& key) { return key.value > value; });
        const auto lessOrEqual =
            std::count_if(reference.begin(),
                          reference.end(),
                          [value](const Record& key) { return key.value >= value; });
        EXPECT_EQ(tree.countKeysLessThan(Record(value)),
                  static_cast<std::size_t>(less));
        EXPECT_EQ(tree.countKeysLessThanOrEqual(Record(value)),
                  static_cast<std::size_t>(lessOrEqual));
    }

    EXPECT_THROW(tree.getKeyBySortedIndex(tree.size()), std::out_of_range);
}

} // namespace
