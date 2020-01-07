#include <cassert>
#include <iostream>
#include <cstring>
#include "index.h"

namespace open_spiel {
namespace universal_poker {
namespace abstracted_poker {
namespace hand_index{

void BasicHandIndexTests() {
    generalIndexer indexer_1(1);
    generalIndexer indexer_2(2);
    generalIndexer indexer_3(3);
    generalIndexer indexer_4(4);
    const hand_index_t size_per_round_gt[4] = {169, 1286792, 55190538, 2428287420};
    assert(indexer_1.getSize(1) == size_per_round_gt[0]);
    assert(indexer_2.getSize(1) == size_per_round_gt[0]);
    assert(indexer_2.getSize(2) == size_per_round_gt[1]);
    assert(indexer_3.getSize(1) == size_per_round_gt[0]);
    assert(indexer_3.getSize(2) == size_per_round_gt[1]);
    assert(indexer_3.getSize(3) == size_per_round_gt[2]);
    assert(indexer_4.getSize(1) == size_per_round_gt[0]);
    assert(indexer_4.getSize(2) == size_per_round_gt[1]);
    assert(indexer_4.getSize(3) == size_per_round_gt[2]);
    assert(indexer_4.getSize(4) == size_per_round_gt[3]);
    std::cout << "Size for round 1: " << indexer_4.getSize(1) << std::endl;
    std::cout << "Size for round 2: " << indexer_4.getSize(2) << std::endl;
    std::cout << "Size for round 3: " << indexer_4.getSize(3) << std::endl;
    std::cout << "Size for round 4: " << indexer_4.getSize(4) << std::endl;
    preflopIndexer indexer_p;
    indexer_p.print_table();
    std::cout << "Index for '5s9sAhKhTc': " << indexer_2.index("5s9sAhKhTc") << std::endl;
    assert(indexer_2.index("5s9sAhKhTc") == 1026452);
    std::cout << "Canonical hand for 1026452: " << indexer_2.canonicalHand(1026452) << std::endl;
    assert(indexer_2.canonicalHand(1026452) == "5s9sKhAhTd");
    std::cout << "Index for '2d9dKd7s7h4c': " << indexer_3.index("2d9dKd7s7h4c") << std::endl;
    assert(indexer_3.index("2d9dKd7s7h4c") == 47386893);
    std::cout << "Canonical hand for 47386893: " << indexer_3.canonicalHand(47386893) << std::endl;
    assert(indexer_3.canonicalHand(47386893) == "2s9sKs7h7d4c");
    std::cout << "Index for '3s9s4d6c9c3c8d': " << indexer_4.index("3s9s4d6c9c3c8d") << std::endl;
    assert(indexer_4.index("3s9s4d6c9c3c8d") == 1959686764);
    std::cout << "Canonical hand for 1959686764: " << indexer_4.canonicalHand(1959686764) << std::endl;
    assert(indexer_4.canonicalHand(1959686764) == "3s9s6h9h4d3h8d");
}

}   // namespace handIndex
}   // namespace abstracted_poker
}   // namespace universal_poker
}   // namespace open_spiel


int main(int argc, char **argv) {
    open_spiel::universal_poker::abstracted_poker::hand_index::BasicHandIndexTests();
}
