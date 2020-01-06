#include <cassert>
#include <cstdio>
#include <sstream>

#define _Bool bool

extern "C" {
#include "hand-isomorphism/src/hand_index.h"
}

#include "index.h"

namespace open_spiel {
namespace universal_poker {
namespace abstracted_poker {
namespace handIndex {


/* --------------------------------------------------------------------------- */
std::string kSuitChars = "shdc";
std::string kRankChars = "23456789TJQKA";


preflopIndexer::preflopIndexer() {
    const uint8_t cards_per_round[1] = {2};
    hand_indexer_init(1, cards_per_round, &(this->preflop_indexer));
    this->size = hand_indexer_size(&(this->preflop_indexer), 0);
}

void preflopIndexer::print_table() {
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

            hand_index_t index = hand_index_last(&(this->preflop_indexer),
                                                 cards);
            printf(" %3" PRIhand_index, index);
        }
        printf("\n");
    }
}

hand_index_t preflopIndexer::index(std::string cardString) {
    uint8_t cards[7];
    assert(cardString.size() <= 2);
    for (int i = 0; i < cardString.size(); i += 2) {
        char rankChr = cardString[i];
        char suitChr = cardString[i + 1];
        card_t rank = (card_t) (kRankChars.find(rankChr));
        card_t suit = (card_t) (kSuitChars.find(suitChr));
        cards[i / 2] = deck_make_card(suit, rank);
    }
    hand_index_t index = hand_index_last(&(this->preflop_indexer), cards);
    return index;
}

std::string preflopIndexer::canonicalHand(card_t cardId) {
    assert(cardId < this->size);
    uint8_t cards[7];
    hand_unindex(&(this->preflop_indexer), 0, cardId, cards);

    std::ostringstream result;
    for (int i = 0; i < 2; i++) {
        card_t suit = deck_get_suit(cards[i]);
        card_t rank = deck_get_rank(cards[i]);
        result << kRankChars[rank] << kSuitChars[suit];
    }
    return result.str();
}

hand_index_t preflopIndexer::getSize() {
    return this->size;
}

generalIndexer::generalIndexer(uint8_t round) {
    const uint8_t cards_per_round[4] = {2, 3, 1, 1};
    this->cards_num[0] = cards_per_round[0];
    for (int i = 1; i < 4; i++) {
        this->cards_num[i] = this->cards_num[i - 1] + cards_per_round[i];
    }

    assert(round >= 1 && round <= 4);
    this->round = round;
    hand_indexer_init(this->round, cards_per_round, &(this->general_indexer));
    for (int i = 0; i < this->round; i++) {
        this->size[i] = hand_indexer_size(&(this->general_indexer), i);
    }
}

hand_index_t generalIndexer::index(std::string cardString) {
    assert(cardString.size() <= 2 * 7);
    uint8_t cards[7];
    for (int i = 0; i < cardString.size(); i += 2) {
        char rankChr = cardString[i];
        char suitChr = cardString[i + 1];
        card_t rank = (card_t) (kRankChars.find(rankChr));
        card_t suit = (card_t) (kSuitChars.find(suitChr));
        cards[i / 2] = deck_make_card(suit, rank);
    }
    return hand_index_last(&(this->general_indexer), cards);
}

std::string generalIndexer::canonicalHand(card_t cardId) {
    assert(cardId < this->size[this->round - 1]);
    uint8_t cards[7];
    hand_unindex(&(this->general_indexer), this->round - 1, cardId, cards);

    std::ostringstream result;
    for (int i = 0; i < this->cards_num[this->round - 1]; i++) {
        card_t suit = deck_get_suit(cards[i]);
        card_t rank = deck_get_rank(cards[i]);
        result << kRankChars[rank] << kSuitChars[suit];
    }
    return result.str();
}

hand_index_t generalIndexer::getSize(int round) {
    assert(round >= 1 && round <= this->round);
    return this->size[round - 1];
}

uint8_t generalIndexer::getCardsNum(int round) {
    assert(round >= 1 && round <= this->round);
    return this->cards_num[round - 1];
}

}
}
}
}
