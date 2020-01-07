#ifndef __HANDINDEX_INDEX_H__
#define __HANDINDEX_INDEX_H__

#include <cassert>
#include <cstdio>
#include <sstream>

#define _Bool bool

extern "C" {
#include "hand-isomorphism/src/hand_index.h"
}

namespace open_spiel {
namespace universal_poker {
namespace abstracted_poker {
namespace handIndex{


/* preflopIndexer */
struct preflopIndexer {
private:
    hand_indexer_t preflop_indexer;
    hand_index_t size;
public:
    preflopIndexer();

    void print_table();

    hand_index_t index(std::string);

    std::string canonicalHand(card_t);

    hand_index_t getSize();
};


/* generalIndexer */
struct generalIndexer {
private:
    int round;
    hand_indexer_t general_indexer;
    hand_index_t size[4];
    int cards_num[4];
public:
    explicit generalIndexer(int);

    // void print_table();
    hand_index_t index(std::string);

    std::string canonicalHand(hand_index_t);

    hand_index_t getSize(int);

    int getCardsNum(int);
};

}
}
}
}


#endif
