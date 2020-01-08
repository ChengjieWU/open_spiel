#include <cassert>
#include <cstdio>
#include <sstream>

#define _Bool bool

#include <iostream>
#include "index.h"

namespace open_spiel {
namespace universal_poker {
namespace abstracted_poker {
namespace hand_index {


/* --------------------------------------------------------------------------- */
std::string kSuitChars_index = "shdc";
std::string kRankChars_index = "23456789TJQKA";


preflopIndexer::preflopIndexer() {
    const uint8_t cards_per_round[1] = {2};
    hand_indexer_init(1, cards_per_round, &preflop_indexer);
    this->size = hand_indexer_size(&preflop_indexer, 0);
}

void preflopIndexer::print_table() const{
    uint8_t cards[7];
    printf("preflop table:\n");
    printf(" ");
    for (uint_fast32_t i = 0; i < RANKS; ++i) {
        printf("  %c ", RANK_TO_CHAR[RANKS - 1 - i]);
    }
    printf("\n");
    for (uint_fast32_t i = 0; i < RANKS; ++i) {
        printf("%c", RANK_TO_CHAR[RANKS - 1 - i]);
        for (uint_fast32_t j = 0; j < RANKS; ++j) {
            cards[0] = deck_make_card(0, RANKS - 1 - j);
            cards[1] = deck_make_card(j <= i, RANKS - 1 - i);

            hand_index_t index = hand_index_last(&preflop_indexer,
                                                 cards);
            printf(" %3" PRIhand_index, index);
        }
        printf("\n");
    }
}

hand_index_t preflopIndexer::index(std::string cardString) const{
    uint8_t cards[7];
    assert(cardString.size() <= 2);
    for (int i = 0; i < cardString.size(); i += 2) {
        char rankChr = cardString[i];
        char suitChr = cardString[i + 1];
        card_t rank = (card_t) (kRankChars_index.find(rankChr));
        card_t suit = (card_t) (kSuitChars_index.find(suitChr));
        cards[i / 2] = deck_make_card(suit, rank);
    }
    hand_index_t index = hand_index_last(&(this->preflop_indexer), cards);
    return index;
}

std::string preflopIndexer::canonicalHand(hand_index_t cardId) const {
    assert(cardId < size);
    uint8_t cards[7];
    hand_unindex(&preflop_indexer, 0, cardId, cards);

    std::ostringstream result;
    for (int i = 0; i < 2; i++) {
        card_t suit = deck_get_suit(cards[i]);
        card_t rank = deck_get_rank(cards[i]);
        result << kRankChars_index[rank] << kSuitChars_index[suit];
    }
    return result.str();
}

hand_index_t preflopIndexer::getSize() const {
    return size;
}

generalIndexer::generalIndexer(int r) {
    const uint8_t cards_per_round[4] = {2, 3, 1, 1};
    cards_num[0] = cards_per_round[0];
    for (int i = 1; i < 4; i++) {
        cards_num[i] = cards_num[i - 1] + cards_per_round[i];
    }
    assert(r >= 1 && r <= 4);
    round = r;
    assert(hand_indexer_init(round, cards_per_round, &general_indexer));
    for (int i = 0; i < round; i++) {
        size[i] = (long long)hand_indexer_size(&general_indexer, i);
    }
}

hand_index_t generalIndexer::index(std::string cardString) const {
    assert(cardString.size() <= 2 * 7);
    uint8_t cards[7];
    for (int i = 0; i < cardString.size(); i += 2) {
        char rankChr = cardString[i];
        char suitChr = cardString[i + 1];
        card_t rank = (card_t) (kRankChars_index.find(rankChr));
        card_t suit = (card_t) (kSuitChars_index.find(suitChr));
        cards[i / 2] = deck_make_card(suit, rank);
    }
    return hand_index_last(&general_indexer, cards);
}

std::string generalIndexer::canonicalHand(hand_index_t cardId) const {
    // std::cerr << "IN CAL: " << cardId << std::endl;
    assert(cardId < size[round - 1]);
    uint8_t cards[7];
    hand_unindex(&general_indexer, round - 1, cardId, cards);

    std::ostringstream result;
    for (int i = 0; i < cards_num[round - 1]; i++) {
        card_t suit = deck_get_suit(cards[i]);
        card_t rank = deck_get_rank(cards[i]);
        result << kRankChars_index[rank] << kSuitChars_index[suit];
    }
    // std::cerr << result.str() << std::endl;
    return result.str();
}

hand_index_t generalIndexer::getSize(int r) const {
    assert(r >= 1 && r <= round);
    return size[r - 1];
}

int generalIndexer::getCardsNum(int r) const {
    assert(r >= 1 && r <= round);
    return cards_num[r - 1];
}

}   // hand_index
}   // abstracted_poker
}   // namespace universal_poker
}   // namespace open_spiel
