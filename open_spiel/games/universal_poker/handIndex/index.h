#ifndef HANDINDEX_INDEX_H_
#define HANDINDEX_INDEX_H_

#include <sstream>

#define _Bool bool

extern "C" {
#include "hand-isomorphism/src/hand_index.h"
}

namespace open_spiel {
namespace universal_poker {
namespace abstracted_poker {
namespace hand_index{


/* preflopIndexer */
class preflopIndexer {
private:
    hand_indexer_t preflop_indexer;
    hand_index_t size;
public:
    preflopIndexer();
    void print_table();
    hand_index_t index(std::string);
    std::string canonicalHand(hand_index_t);
    hand_index_t getSize();
};


/* generalIndexer */
class generalIndexer {
private:
    int round;
    hand_indexer_t general_indexer;
    hand_index_t size[4];
    int cards_num[4];
public:
    explicit generalIndexer(int);
    hand_index_t index(std::string);
    std::string canonicalHand(hand_index_t);
    hand_index_t getSize(int);
    int getCardsNum(int);
};

}   // hand_index
}   // abstracted_poker
}   // namespace universal_poker
}   // namespace open_spiel

#endif
