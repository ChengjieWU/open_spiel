// Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "open_spiel/games/abstracted_poker.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>
#include <fstream>

#include "open_spiel/abseil-cpp/absl/strings/str_cat.h"
#include "open_spiel/abseil-cpp/absl/strings/str_format.h"
#include "open_spiel/abseil-cpp/absl/strings/str_join.h"
#include "open_spiel/game_parameters.h"
#include "open_spiel/games/universal_poker/logic/card_set.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/games/universal_poker/handIndex/index.h"

namespace open_spiel {
namespace universal_poker {
namespace abstracted_poker {

const absl::string_view kEmptyString = "";

const GameType kGameType{
    /*short_name=*/"abstracted_poker",
    /*long_name=*/"Abstracted Poker",
    GameType::Dynamics::kSequential,
    GameType::ChanceMode::kExplicitStochastic,
    GameType::Information::kImperfectInformation,
    GameType::Utility::kZeroSum,
    GameType::RewardModel::kTerminal,
    /*max_num_players=*/10,
    /*min_num_players=*/2,
    /*provides_information_state_string=*/true,
    /*provides_information_state_tensor=*/true,
    /*provides_observation_string=*/true,
    /*provides_observation_tensor=*/true,
    /*parameter_specification=*/

    {// The ACPC code uses a specific configuration file to describe the game.
     // The following has been copied from ACPC documentation:
     //
     // Empty lines or lines with '#' as the very first character will be
     // ignored
     //
     // The Game definitions should start with "gamedef" and end with
     // "end gamedef" and can have the fields documented bellow (case is
     // ignored)
     //
     // If you are creating your own game definitions, please note that game.h
     // defines some constants for maximums in games (e.g., number of rounds).
     // These may need to be changed for games outside of the what is being run
     // for the Annual Computer Poker Competition.

     // The ACPC gamedef string.  When present, it will take precedence over
     // everything and no other argument should be provided.
     {"gamedef", GameParameter(std::string(kEmptyString))},
     // Instead of a single gamedef, specifying each line is also possible.
     // The documentation is adapted from project_acpc_server/game.cc.
     //
     // Number of Players (up to 10)
     {"numPlayers", GameParameter(2)},
     // Betting Type "limit" "nolimit"
     {"betting", GameParameter(std::string("nolimit"))},
     // The stack size for each player at the start of each hand (for
     // no-limit). It will be ignored on "limit".
     // TODO(author2): It's unclear what happens on limit. It defaults to
     // INT32_MAX for all players when not provided.
     {"stack", GameParameter(std::string("1200 1200"))},
     // The size of the blinds for each player (relative to the dealer)
     {"blind", GameParameter(std::string("100 100"))},
     // The size of raises on each round (for limit games only) as numrounds
     // integers. It will be ignored for nolimite games.
     {"raiseSize", GameParameter(std::string("100 100"))},
     // Number of betting rounds per hand of the game
     {"numRounds", GameParameter(2)},
     // The player that acts first (relative to the dealer) on each round
     {"firstPlayer", GameParameter(std::string("1 1"))},
     // maxraises - the maximum number of raises on each round. If not
     // specified, it will default to UINT8_MAX.
     {"maxRaises", GameParameter(std::string(kEmptyString))},
     // The number of different suits in the deck
     {"numSuits", GameParameter(4)},
     // The number of different ranks in the deck
     {"numRanks", GameParameter(6)},
     // The number of private cards to  deal to each player
     {"numHoleCards", GameParameter(1)},
     // The number of cards revealed on each round
     {"numBoardCards", GameParameter(std::string("0 1"))},
     // Specify which actions are available to the player, in both limit and
     // nolimit games. Available options are: "fc" for fold and check/call.
     // "fcpa" for fold, check/call, bet pot and all in (default).
     {"bettingAbstraction", GameParameter(std::string("fcpa"))}}};

std::shared_ptr<const Game> Factory(const GameParameters &params) {
  return std::shared_ptr<const Game>(new UniversalPokerGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

// Returns how many actions are available at a choice node (3 when limit
// and 4 for no limit).
// TODO(author2): Is that a bug? There are 5 actions? Is no limit means
// "bet bot" is added? or should "all in" be also added?
// Add half-pot bet, 4 -> 5
// Add off-abs raise, 5 -> 6
// Add 1*pot bet, 6 -> 7
// Add 2*pot bet, 7 -> 8
inline uint32_t GetMaxBettingActions(const acpc_cpp::ACPCGame &acpc_game) {
  return acpc_game.IsLimitGame() ? 3 : 8;
}

// namespace universal_poker
UniversalPokerState::UniversalPokerState(std::shared_ptr<const Game> game)
    : State(game),
      acpc_game_(
          static_cast<const UniversalPokerGame *>(game.get())->GetACPCGame()),
      acpc_state_(acpc_game_),
      deck_(/*num_suits=*/acpc_game_->NumSuitsDeck(),
            /*num_ranks=*/acpc_game_->NumRanksDeck()),
      hole_cards_(acpc_game_->GetNbPlayers()),
      cur_player_(kChancePlayerId),
      possibleActions_(ACTION_DEAL),
      betting_abstraction_(static_cast<const UniversalPokerGame *>(game.get())
                               ->betting_abstraction()) {}

std::string UniversalPokerState::ToString() const {
  std::ostringstream buf;
  buf << betting_abstraction_ << std::endl;
  for (int p = 0; p < acpc_game_->GetNbPlayers(); ++p) {
    buf << "P" << p << " Cards: " << hole_cards_[p].ToString()
        << std::endl;
  }
  buf << "BoardCards " << board_cards_.ToString() << std::endl;

  if (IsChanceNode()) {
    buf << "PossibleCardsToDeal " << deck_.ToString() << std::endl;
  }
  if (IsTerminal()) {
    for (int p = 0; p < acpc_game_->GetNbPlayers(); ++p) {
      buf << "P" << p << " Reward: " << GetTotalReward(p) << std::endl;
    }
  }
  buf << "Node type?: ";
  if (IsChanceNode()) {
    buf << "Chance node" << std::endl;
  } else if (IsTerminal()) {
    buf << "Terminal Node!" << std::endl;
  } else {
    buf << "Player node for player " << cur_player_ << std::endl;
  }

  buf << "PossibleActions (" << GetPossibleActionCount() << "): [";
  for (auto action : ALL_ACTIONS) {
    if (action & possibleActions_) {
      buf << ((action == ACTION_ALL_IN) ? " ACTION_ALL_IN " : "");
      buf << ((action == ACTION_BET) ? " ACTION_BET " : "");
      buf << ((action == ACTION_CHECK_CALL) ? " ACTION_CHECK_CALL " : "");
      buf << ((action == ACTION_FOLD) ? " ACTION_FOLD " : "");
      buf << ((action == ACTION_DEAL) ? " ACTION_DEAL " : "");
      buf << ((action == ACTION_BET_HALF_POT) ? " ACTION_BET_HALF_POT " : "");
      buf << ((action == ACTION_OFF_ABS) ? " ACTION_OFF_ABS " : "");
      buf << ((action == ACTION_BET_POT) ? " ACTION_BET_POT " : "");
      buf << ((action == ACTION_BET_DOUBLE_POT) ? " ACTION_BET_DOUBLE_POT " : "");
    }
  }
  buf << "]" << std::endl;
  buf << "Round: " << acpc_state_.GetRound() << std::endl;
  buf << "ACPC State: " << acpc_state_.ToString() << std::endl;
  buf << "Action Sequence: " << actionSequence_ << std::endl;

  return buf.str();
}

std::string UniversalPokerState::ActionToString(Player player,
                                                Action move) const {
    if (player == kChancePlayerId) {
        return absl::StrCat("player=", "chance", " move=", "d", " card=", move);
    }
    else {
        switch (move) {
            case open_spiel::universal_poker::abstracted_poker::ActionType::kFold:
                return absl::StrCat("player=", player, " move=", "f");
            case open_spiel::universal_poker::abstracted_poker::ActionType::kCall:
                return absl::StrCat("player=", player, " move=", "c");
            case open_spiel::universal_poker::abstracted_poker::ActionType::kBet:
                return absl::StrCat("player=", player, " move=", "r", " money=",
                                    potSize_);
            case open_spiel::universal_poker::abstracted_poker::ActionType::kAllIn:
                return absl::StrCat("player=", player, " move=", "r", " money=",
                                    allInSize_);
            case open_spiel::universal_poker::abstracted_poker::ActionType::kBetHalfPot:
                return absl::StrCat("player=", player, " move=", "r", " money=",
                                    halfpotSize_);
            case open_spiel::universal_poker::abstracted_poker::ActionType::kOffAbs:
                return absl::StrCat("player=", player, " move=", "r", " money=",
                                    offabsSize_);
            case open_spiel::universal_poker::abstracted_poker::ActionType::kBetPot:
                return absl::StrCat("player=", player, " move=", "r", " money=",
                                    betpotSize_);
            case open_spiel::universal_poker::abstracted_poker::ActionType::kBetDoublePot:
                return absl::StrCat("player=", player, " move=", "r", " money=",
                                    doublepotSize_);
            default:
                SpielFatalError("Invalid action in ActionToString!");
        }
    }
}

bool UniversalPokerState::IsTerminal() const {
  bool finished = cur_player_ == kTerminalPlayerId;
  assert(acpc_state_.IsFinished() || !finished);
  return finished;
}

bool UniversalPokerState::IsChanceNode() const {
  return cur_player_ == kChancePlayerId;
}

Player UniversalPokerState::CurrentPlayer() const {
  if (IsTerminal()) {
    return kTerminalPlayerId;
  }
  if (IsChanceNode()) {
    return kChancePlayerId;
  }

  return Player(acpc_state_.CurrentPlayer());
}

std::vector<double> UniversalPokerState::Returns() const {
  if (!IsTerminal()) {
    return std::vector<double>(NumPlayers(), 0.0);
  }

  std::vector<double> returns(NumPlayers());
  for (Player player = 0; player < NumPlayers(); ++player) {
    // Money vs money at start.
    returns[player] = GetTotalReward(player);
  }

  return returns;
}

void UniversalPokerState::InformationStateTensor(
    Player player, std::vector<double> *values) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);

  values->resize(game_->InformationStateTensorShape()[0]);
  std::fill(values->begin(), values->end(), 0.);

  // Layout of observation:
  //   my player number: num_players bits
  //   my cards: Initial deck size bits (1 means you have the card), i.e.
  //             MaxChanceOutcomes() = NumSuits * NumRanks
  //   public cards: Same as above, but for the public cards.
  //   NumRounds() round sequence: (max round seq length)*2 bits
  int offset = 0;

  // Mark who I am.
  (*values)[player] = 1;
  offset += NumPlayers();

  const logic::CardSet full_deck(acpc_game_->NumSuitsDeck(),
                                 acpc_game_->NumRanksDeck());
  const std::vector<uint8_t> deckCards = full_deck.ToCardArray();
  logic::CardSet holeCards = hole_cards_[player];
  // TODO(author2): it should be way more efficient to iterate over the cards
  // of the player, rather than iterating over all the cards.
  for (uint32_t i = 0; i < full_deck.NumCards(); i++) {
    (*values)[i + offset] = holeCards.ContainsCards(deckCards[i]) ? 1.0 : 0.0;
  }
  offset += full_deck.NumCards();

  // Public cards
  for (int i = 0; i < full_deck.NumCards(); ++i) {
    (*values)[i + offset] =
        board_cards_.ContainsCards(deckCards[i]) ? 1.0 : 0.0;
  }
  offset += full_deck.NumCards();

  const std::string actionSeq = GetActionSequence();
  const int length = actionSeq.length();
  SPIEL_CHECK_LT(length, game_->MaxGameLength());

  for (int i = 0; i < length; ++i) {
    SPIEL_CHECK_LT(offset + i + 1, values->size());
    if (actionSeq[i] == 'c') {
      // Encode call as 10.
      (*values)[offset + (2 * i)] = 1;
      (*values)[offset + (2 * i) + 1] = 0;
    } else if (actionSeq[i] == 'p') {
      // Encode raise as 01.
      (*values)[offset + (2 * i)] = 0;
      (*values)[offset + (2 * i) + 1] = 1;
    } else if (actionSeq[i] == 'a') {
      // Encode raise as 01.
      (*values)[offset + (2 * i)] = 1;
      (*values)[offset + (2 * i) + 1] = 1;
    } else if (actionSeq[i] == 'h') {
      // Encode raise as 01.
      (*values)[offset + (2 * i)] = 1;
      (*values)[offset + (2 * i) + 1] = 1;
    } else if (actionSeq[i] == 'b') {
      // Encode raise as 01.
      (*values)[offset + (2 * i)] = 0;
      (*values)[offset + (2 * i) + 1] = 0;
    } else if (actionSeq[i] == 'w') {
      // Encode raise as 01.
      (*values)[offset + (2 * i)] = 0;
      (*values)[offset + (2 * i) + 1] = 0;
    } else if (actionSeq[i] == 't') {
      // Encode raise as 01.
      (*values)[offset + (2 * i)] = 0;
      (*values)[offset + (2 * i) + 1] = 0;
    } else if (actionSeq[i] == 'f') {
      // Encode fold as 00.
      // TODO(author2): Should this be 11?
      (*values)[offset + (2 * i)] = 0;
      (*values)[offset + (2 * i) + 1] = 0;
    } else if (actionSeq[i] == 'd') {
      (*values)[offset + (2 * i)] = 0;
      (*values)[offset + (2 * i) + 1] = 0;
    } else {
      SPIEL_CHECK_EQ(actionSeq[i], 'd');
    }
  }

  // Move offset up to the next round: 2 bits per move.
  offset += game_->MaxGameLength() * 2;
  SPIEL_CHECK_EQ(offset, game_->InformationStateTensorShape()[0]);
}

void UniversalPokerState::ObservationTensor(Player player,
                                            std::vector<double> *values) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, NumPlayers());

  values->resize(game_->ObservationTensorShape()[0]);
  std::fill(values->begin(), values->end(), 0.);

  // Layout of observation:
  //   my player number: num_players bits
  //   my cards: Initial deck size bits (1 means you have the card), i.e.
  //             MaxChanceOutcomes() = NumSuits * NumRanks
  //   public cards: Same as above, but for the public cards.
  //   the contribution of each player to the pot. num_players integers.
  int offset = 0;

  // Mark who I am.
  (*values)[player] = 1;
  offset += NumPlayers();

  const logic::CardSet full_deck(acpc_game_->NumSuitsDeck(),
                                 acpc_game_->NumRanksDeck());
  const std::vector<uint8_t> all_cards = full_deck.ToCardArray();
  logic::CardSet holeCards = hole_cards_[player];

  for (uint32_t i = 0; i < full_deck.NumCards(); i++) {
    (*values)[i + offset] = holeCards.ContainsCards(all_cards[i]) ? 1.0 : 0.0;
  }
  offset += full_deck.NumCards();

  for (uint32_t i = 0; i < full_deck.NumCards(); i++) {
    (*values)[i + offset] =
        board_cards_.ContainsCards(all_cards[i]) ? 1.0 : 0.0;
  }
  offset += full_deck.NumCards();

  // Adding the contribution of each players to the pot.
  for (auto p = Player{0}; p < NumPlayers(); p++) {
    (*values)[offset + p] = acpc_state_.Ante(p);
  }
  offset += NumPlayers();
  SPIEL_CHECK_EQ(offset, game_->ObservationTensorShape()[0]);
}

std::string UniversalPokerState::InformationStateString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, acpc_game_->GetNbPlayers());
  const uint32_t pot = acpc_state_.MaxSpend() *
                       (acpc_game_->GetNbPlayers() - acpc_state_.NumFolded());
  std::vector<int> money;
  for (auto p = Player{0}; p < acpc_game_->GetNbPlayers(); p++) {
    money.emplace_back(acpc_state_.Money(p));
  }
  std::vector<std::string> sequences;
  for (auto r = 0; r <= acpc_state_.GetRound(); r++) {
    sequences.emplace_back(acpc_state_.BettingSequence(r));
  }

  // We apply infoset abstraction here!

  // Debug output
//   std::cerr << "*************************************************\n";
//   std::cerr << acpc_state_.GetRound() << std::endl;
//   std::cerr << "hole cards: " << hole_cards_[player].ToString() << std::endl;
//   std::cerr << "board cards: " << board_cards_.ToString() << std::endl;
//   std::cerr << "hole cards + board cards: " << hole_cards_[player].ToString() + board_cards_.ToString() << std::endl;
   uint64_t cards_index = 0;    // TODO: we simply return 0 if not valid status
   if (hole_cards_[player].ToString().size() == 4 && (board_cards_.ToString().size() == 0 || board_cards_.ToString().size() >= 6))
       cards_index = GetIndex(acpc_state_.GetRound() + 1, hole_cards_[player].ToString() + board_cards_.ToString());
//   std::cerr << "cards index: " << cards_index << std::endl;
//   std::string cards_string = GetCanonicalHand(acpc_state_.GetRound() + 1, cards_index);
//   std::string hole_cards_abs_ = cards_string.substr(0, 4);
//   std::string board_cards_abs_ = cards_string.substr(4);
//   std::cerr << "cards_string: " << cards_string << std::endl;
//   std::cerr << hole_cards_abs_ << std::endl;
//   std::cerr << board_cards_abs_ << std::endl;

   // TODO: we use a fake cluster result here!
   int cluster_index = GetCluster(acpc_state_.GetRound() + 1, cards_index);

   return absl::StrFormat(
       "[Round %i][Player: %i][Pot: %i][Money: %s][InfoAbs: %i][Sequences: %s]",
       acpc_state_.GetRound(), CurrentPlayer(), pot, absl::StrJoin(money, " "),
       cluster_index,
       absl::StrJoin(sequences, "|"));
}

std::string UniversalPokerState::ObservationString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, acpc_game_->GetNbPlayers());
  std::string result;

  const uint32_t pot = acpc_state_.MaxSpend() *
                       (acpc_game_->GetNbPlayers() - acpc_state_.NumFolded());
  absl::StrAppend(&result, "[Round ", acpc_state_.GetRound(),
                  "][Player: ", CurrentPlayer(), "][Pot: ", pot, "][Money:");
  for (auto p = Player{0}; p < acpc_game_->GetNbPlayers(); p++) {
    absl::StrAppend(&result, " ", acpc_state_.Money(p));
  }
  // Add the player's private cards
  if (player != kChancePlayerId) {
    absl::StrAppend(&result, "[Private: ", hole_cards_[player].ToString(), "]");
  }
  // Adding the contribution of each players to the pot
  absl::StrAppend(&result, "[Ante:");
  for (auto p = Player{0}; p < num_players_; p++) {
    absl::StrAppend(&result, " ", acpc_state_.Ante(p));
  }
  absl::StrAppend(&result, "]");

  return result;
}

std::unique_ptr<State> UniversalPokerState::Clone() const {
  return std::unique_ptr<State>(new UniversalPokerState(*this));
}

std::vector<std::pair<Action, double>> UniversalPokerState::ChanceOutcomes()
    const {
  SPIEL_CHECK_TRUE(IsChanceNode());
  std::vector<uint8_t> available_cards = deck_.ToCardArray();
  const int num_cards = available_cards.size();
  const double p = 1.0 / num_cards;

  // We need to convert std::vector<uint8_t> into std::vector<Action>.
  std::vector<std::pair<Action, double>> outcomes;
  outcomes.reserve(num_cards);
  for (const auto &card : available_cards) {
    outcomes.push_back({Action{card}, p});
  }
  return outcomes;
}

std::vector<Action> UniversalPokerState::LegalActions() const {
  if (IsChanceNode()) {
    std::vector<uint8_t> available_cards = deck_.ToCardArray();
    std::vector<Action> actions;
    actions.reserve(available_cards.size());
    for (const auto &card : available_cards) {
      actions.push_back(card);
    }
    return actions;
  }
  std::vector<Action> legal_actions;
  if (ACTION_FOLD & possibleActions_) legal_actions.push_back(kFold);
  if (ACTION_CHECK_CALL & possibleActions_) legal_actions.push_back(kCall);
  if (ACTION_BET & possibleActions_) legal_actions.push_back(kBet);
  if (ACTION_ALL_IN & possibleActions_) legal_actions.push_back(kAllIn);
  if (ACTION_BET_HALF_POT & possibleActions_) legal_actions.push_back(kBetHalfPot);
  if (ACTION_OFF_ABS & possibleActions_) legal_actions.push_back(kOffAbs);
  if (ACTION_BET_POT & possibleActions_) legal_actions.push_back(kBetPot);
  if (ACTION_BET_DOUBLE_POT & possibleActions_) legal_actions.push_back(kBetDoublePot);
  return legal_actions;
}

// We first deal the cards to each player, dealing all the cards to the first
// player first, then the second player, until all players have their private
// cards.
void UniversalPokerState::DoApplyAction(Action action_id) {
  if (IsChanceNode()) {
    // In chance nodes, the action_id is exactly the card being dealt.
    uint8_t card = action_id;
    deck_.RemoveCard(card);
    actionSequence_ += 'd';

    // Check where to add this card
    for (int p = 0; p < acpc_game_->GetNbPlayers(); ++p) {
      if (hole_cards_[p].NumCards() < acpc_game_->GetNbHoleCardsRequired()) {
        hole_cards_[p].AddCard(card);
        _CalculateActionsAndNodeType();
        return;
      }
    }

    if (board_cards_.NumCards() <
        acpc_game_->GetNbBoardCardsRequired(acpc_state_.GetRound())) {
      board_cards_.AddCard(card);
      _CalculateActionsAndNodeType();
      return;
    }
  } else {
    int action_int = static_cast<int>(action_id);
    if (action_int == kFold) {
      ApplyChoiceAction(ACTION_FOLD);
      return;
    }
    if (action_int == kCall) {
      ApplyChoiceAction(ACTION_CHECK_CALL);
      return;
    }
    if (action_int == kBet) {
      ApplyChoiceAction(ACTION_BET);
      return;
    }
    if (action_int == kAllIn) {
      ApplyChoiceAction(ACTION_ALL_IN);
      return;
    }
    if (action_int == kBetHalfPot) {
      ApplyChoiceAction(ACTION_BET_HALF_POT);
      return;
    }
    if (action_int == kOffAbs) {
      ApplyChoiceAction(ACTION_OFF_ABS);
      return;
    }
    if (action_int == kBetPot) {
      ApplyChoiceAction(ACTION_BET_POT);
      return;
    }
    if (action_int == kBetDoublePot) {
      ApplyChoiceAction(ACTION_BET_DOUBLE_POT);
      return;
    }
    SpielFatalError(absl::StrFormat("Action not recognized: %i", action_id));
  }
}

double UniversalPokerState::GetTotalReward(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, acpc_game_->GetNbPlayers());
  // Copy Board Cards and Hole Cards
  uint8_t holeCards[10][3], boardCards[7], nbHoleCards[10];

  for (size_t p = 0; p < hole_cards_.size(); ++p) {
    auto cards = hole_cards_[p].ToCardArray();
    for (size_t c = 0; c < cards.size(); ++c) {
      holeCards[p][c] = cards[c];
    }
    nbHoleCards[p] = cards.size();
  }

  auto bc = board_cards_.ToCardArray();
  for (size_t c = 0; c < bc.size(); ++c) {
    boardCards[c] = bc[c];
  }

  acpc_state_.SetHoleAndBoardCards(holeCards, boardCards, nbHoleCards,
                                   /*nbBoardCards=*/bc.size());

  return acpc_state_.ValueOfState(player);
}

HistoryDistribution UniversalPokerState::GetHistoriesConsistentWithInfostate()
    const {
  // This is only implemented for 2 players.
  if (acpc_game_->GetNbPlayers() != 2) return {};

  logic::CardSet is_cards;
  const logic::CardSet &our_cards = hole_cards_[cur_player_];
  for (uint8_t card : our_cards.ToCardArray()) is_cards.AddCard(card);
  for (uint8_t card : board_cards_.ToCardArray()) is_cards.AddCard(card);
  logic::CardSet fresh_deck(/*num_suits=*/acpc_game_->NumSuitsDeck(),
                            /*num_ranks=*/acpc_game_->NumRanksDeck());
  for (uint8_t card : is_cards.ToCardArray()) fresh_deck.RemoveCard(card);
  const int hand_size = acpc_game_->GetNbHoleCardsRequired();
  HistoryDistribution dist;
  for (uint8_t hole_card1 : fresh_deck.ToCardArray()) {
    logic::CardSet subset_deck = fresh_deck;
    subset_deck.RemoveCard(hole_card1);
    for (uint8_t hole_card2 : subset_deck.ToCardArray()) {
      if (hole_card1 < hole_card2) continue;
      std::unique_ptr<State> root = game_->NewInitialState();
      if (cur_player_ == 0) {
        for (uint8_t card : our_cards.ToCardArray()) root->ApplyAction(card);
        root->ApplyAction(hole_card1);
        root->ApplyAction(hole_card2);
      } else if (cur_player_ == 1) {
        root->ApplyAction(hole_card1);
        root->ApplyAction(hole_card2);
        for (uint8_t card : our_cards.ToCardArray()) root->ApplyAction(card);
      }
      SPIEL_CHECK_FALSE(root->IsChanceNode());
      dist.first.push_back(std::move(root));
    }
  }
  dist.second.resize(dist.first.size(),
                     1. / static_cast<double>(dist.first.size()));
  return dist;
}

uint64_t UniversalPokerState::GetIndex(int round, std::string hand) const {
    switch (round) {
        case 1:
            return dynamic_cast<const UniversalPokerGame *>(game_.get())->preflop_indexer.index(hand);
        case 2:
            return dynamic_cast<const UniversalPokerGame *>(game_.get())->flop_indexer.index(hand);
        case 3:
            return dynamic_cast<const UniversalPokerGame *>(game_.get())->turn_indexer.index(hand);
        case 4:
            return dynamic_cast<const UniversalPokerGame *>(game_.get())->river_indexer.index(hand);
        default:
            SpielFatalError("Round not supported!");
    }
}

std::string UniversalPokerState::GetCanonicalHand(int round, uint64_t  card_id) const {
    switch (round) {
        case 1:
            return dynamic_cast<const UniversalPokerGame *>(game_.get())->preflop_indexer.canonicalHand(card_id);
        case 2:
            return dynamic_cast<const UniversalPokerGame *>(game_.get())->flop_indexer.canonicalHand(card_id);
        case 3:
            return dynamic_cast<const UniversalPokerGame *>(game_.get())->turn_indexer.canonicalHand(card_id);
        case 4:
            return dynamic_cast<const UniversalPokerGame *>(game_.get())->river_indexer.canonicalHand(card_id);
        default:
            SpielFatalError("Round not supported!");
    }
}

std::vector<int32_t> UniversalPokerState::GetLegalRaises() const {
    std::vector<Action> legal_actions = LegalActions();
    std::vector<int32_t> ret;
    for (auto a : legal_actions) {
        switch (a) {
            case open_spiel::universal_poker::abstracted_poker::ActionType::kBet:
                ret.push_back(potSize_);
                break;
            case open_spiel::universal_poker::abstracted_poker::ActionType::kAllIn:
                ret.push_back(allInSize_);
                break;
            case open_spiel::universal_poker::abstracted_poker::ActionType::kBetHalfPot:
                ret.push_back(halfpotSize_);
                break;
            case open_spiel::universal_poker::abstracted_poker::ActionType::kOffAbs:
                ret.push_back(offabsSize_);
                break;
            case open_spiel::universal_poker::abstracted_poker::ActionType::kBetPot:
                ret.push_back(betpotSize_);
                break;
            case open_spiel::universal_poker::abstracted_poker::ActionType::kBetDoublePot:
                ret.push_back(doublepotSize_);
                break;
            default:
                break;
        }
    }
    return ret;
}

bool UniversalPokerState::CheckInOffAbsInformationState(const std::string info_string) const {
    bool in_game = dynamic_cast<const UniversalPokerGame *>(game_.get())->CheckInOffAbsInformationState(info_string);
    bool in_state = off_abs_information_state_action_.find(info_string) != off_abs_information_state_action_.end();
    return in_game || in_state;
}

int32_t UniversalPokerState::GetOffAbsInformationStateRaise(const std::string info_string) const {
    /* Search in State first, if not found, search in Game! */
    auto iter = off_abs_information_state_action_.find(info_string);
    if (iter != off_abs_information_state_action_.end()) {
        return iter->second;
    }
    return dynamic_cast<const UniversalPokerGame *>(game_.get())->GetOffAbsInformationStateRaise(info_string);
}

bool UniversalPokerState::AddOffAbsInformationStateRaise(std::string info_string, int32_t raise) {
    /* Since we get a const Game pointer, we can only add it to state. */
    std::pair<std::map<std::string, int32_t>::iterator,bool> ret;
    ret = off_abs_information_state_action_.insert(std::pair<std::string, int32_t>(info_string, raise));
    return ret.second;
}

int UniversalPokerState::GetCluster(int round, uint64_t card_id) const {
    return dynamic_cast<const UniversalPokerGame *>(game_.get())->GetCluster(round, card_id);
}

/**
 * Universal Poker Game Constructor
 * @param params
 */
UniversalPokerGame::UniversalPokerGame(const GameParameters &params)
    : Game(kGameType, params),
      gameDesc_(parseParameters(params)),
      acpc_game_(gameDesc_),
      preflop_indexer(1), flop_indexer(2), turn_indexer(3), river_indexer(4) {
  max_game_length_ = MaxGameLength();
  SPIEL_CHECK_TRUE(max_game_length_.has_value());
  std::string betting_abstraction =
      ParameterValue<std::string>("bettingAbstraction");
  if (betting_abstraction == "fc") {
    betting_abstraction_ = BettingAbstraction::kFC;
  } else if (betting_abstraction == "fcpa") {
    betting_abstraction_ = BettingAbstraction::kFCPA;
  } else {
    SpielFatalError(absl::StrFormat("bettingAbstraction: %s not supported.",
                                    betting_abstraction));
  }
  // TODO: we hard code file_name and length here!
//  turn_cluster_ = ReadCluster("/home/maoyh/results/turn_cluster.bin", 55190538);
//  river_cluster_ = ReadCluster("/home/maoyh/results/river_cluster.bin", 2428287420);
}

std::unique_ptr<State> UniversalPokerGame::NewInitialState() const {
  return std::unique_ptr<State>(new UniversalPokerState(shared_from_this()));
}

std::vector<int> UniversalPokerGame::InformationStateTensorShape() const {
  // One-hot encoding for player number (who is to play).
  // 2 slots of cards (total_num_cards bits each): private card, public card
  // Followed by maximum game length * 2 bits each (call / raise)
  const int num_players = acpc_game_.GetNbPlayers();
  const int gameLength = MaxGameLength();
  const int total_num_cards = MaxChanceOutcomes();

  return {num_players + 2 * total_num_cards + 2 * gameLength};
}

std::vector<int> UniversalPokerGame::ObservationTensorShape() const {
  // One-hot encoding for player number (who is to play).
  // 2 slots of cards (total_cards_ bits each): private card, public card
  // Followed by the contribution of each player to the pot
  const int num_players = acpc_game_.GetNbPlayers();
  const int total_num_cards = MaxChanceOutcomes();
  return {2 * (num_players + total_num_cards)};
}

double UniversalPokerGame::MaxUtility() const {
  // In poker, the utility is defined as the money a player has at the end of
  // the game minus then money the player had before starting the game.
  // The most a player can win *per opponent* is the most each player can put
  // into the pot, which is the raise amounts on each round times the maximum
  // number raises, plus the original chip they put in to play.

  return (double)acpc_game_.StackSize(0) * (acpc_game_.GetNbPlayers() - 1);
}

double UniversalPokerGame::MinUtility() const {
  // In poker, the utility is defined as the money a player has at the end of
  // the game minus then money the player had before starting the game.
  // The most any single player can lose is the maximum number of raises per
  // round times the amounts of each of the raises, plus the original chip they
  // put in to play.
  return -1. * (double)acpc_game_.StackSize(0);
}

int UniversalPokerGame::MaxChanceOutcomes() const {
  return acpc_game_.NumSuitsDeck() * acpc_game_.NumRanksDeck();
}

int UniversalPokerGame::NumPlayers() const { return acpc_game_.GetNbPlayers(); }

int UniversalPokerGame::NumDistinctActions() const {
  return GetMaxBettingActions(acpc_game_);
}

std::shared_ptr<const Game> UniversalPokerGame::Clone() const {
  return std::shared_ptr<const Game>(new UniversalPokerGame(*this));
}

int UniversalPokerGame::MaxGameLength() const {
  // We cache this as this is very slow to calculate.
  if (max_game_length_) return *max_game_length_;

  // Make a good guess here because bruteforcing the tree is far too slow
  // One Terminal Action
  int length = 1;

  // Deal Actions
  length += acpc_game_.GetTotalNbBoardCards() +
            acpc_game_.GetNbHoleCardsRequired() * acpc_game_.GetNbPlayers();

  // Check Actions
  length += (NumPlayers() * acpc_game_.NumRounds());

  // Bet Actions
  double maxStack = 0;
  double maxBlind = 0;
  for (uint32_t p = 0; p < NumPlayers(); p++) {
    maxStack =
        acpc_game_.StackSize(p) > maxStack ? acpc_game_.StackSize(p) : maxStack;
    maxBlind =
        acpc_game_.BlindSize(p) > maxStack ? acpc_game_.BlindSize(p) : maxBlind;
  }

  while (maxStack > maxBlind) {
    maxStack /= 2.0;         // You have always to bet the pot size
    length += NumPlayers();  // Each player has to react
  }

  return length;
}

bool UniversalPokerGame::CheckInOffAbsInformationState(const std::string info_string) const {
    return off_abs_information_state_action_.find(info_string) != off_abs_information_state_action_.end();
}

int32_t UniversalPokerGame::GetOffAbsInformationStateRaise(const std::string info_string) const {
    auto iter = off_abs_information_state_action_.find(info_string);
    if (iter == off_abs_information_state_action_.end()) {
        SpielFatalError("InformationState not found in Off-Abs!");
    }
    return iter->second;
}

bool UniversalPokerGame::AddOffAbsInformationStateRaise(const std::string info_string, const int32_t raise) {
    std::pair<std::map<std::string, int32_t>::iterator,bool> ret;
    ret = off_abs_information_state_action_.insert(std::pair<std::string, int32_t>(info_string, raise));
    return ret.second;
}

std::vector<int> UniversalPokerGame::ReadCluster(std::string file_name, uint64_t length) {
    std::ifstream rf(file_name, std::ios::in | std::ios::binary);
    if (!rf) {
        SpielFatalError("Cannot load cluster file!");
    }
    std::vector<int > ret;
    char ch;
    for (uint64_t i = 0; i < length; i++) {
        rf.read((char *) &ch, sizeof(char));
        int num=ch;
        if (num<0) num+=256;
        ret.push_back(num);
    }
    return ret;
}

int UniversalPokerGame::GetCluster(int round, uint64_t card_id) const {
    return  (int)card_id % 200;
    switch (round) {
        case 1:
            return (int)card_id;
        case 2:
            return (int)card_id % 200; // TODO: implement this!
        case 3:
            return turn_cluster_[card_id];
        case 4:
            return river_cluster_[card_id];
        default:
            SpielFatalError("Round should between 1 & 4!");
    }
}

/**
 * Parses the Game Paramters and makes a gameDesc out of it
 * @param map
 * @return
 */
std::string UniversalPokerGame::parseParameters(const GameParameters &map) {
  if (map.find("gamedef") != map.end()) {
    // We check for sanity that all parameters are empty
    if (map.size() != 1) {
      std::vector<std::string> game_parameter_keys;
      game_parameter_keys.reserve(map.size());
      for (auto const &imap : map) {
        game_parameter_keys.push_back(imap.first);
      }
      SpielFatalError(
          absl::StrCat("When loading a 'universal_poker' game, the 'gamedef' "
                       "field was present, but other fields were present too: ",
                       absl::StrJoin(game_parameter_keys, ", "),
                       "gamedef is exclusive with other paraemters."));
    }
    std::string retreived_gamedef = ParameterValue<std::string>("gamedef");
    std::cerr << "gamedef directly passed for Universal Poker:\n"
              << retreived_gamedef << std::endl;
    return retreived_gamedef;
  }

  std::string generated_gamedef = "GAMEDEF\n";

  absl::StrAppend(
      &generated_gamedef, ParameterValue<std::string>("betting"), "\n",
      "numPlayers = ", ParameterValue<int>("numPlayers"), "\n",
      "numRounds = ", ParameterValue<int>("numRounds"), "\n",
      "numsuits = ", ParameterValue<int>("numSuits"), "\n",
      "firstPlayer = ", ParameterValue<std::string>("firstPlayer"), "\n",
      "numRanks = ", ParameterValue<int>("numRanks"), "\n",
      "numHoleCards = ", ParameterValue<int>("numHoleCards"), "\n",
      "numBoardCards = ", ParameterValue<std::string>("numBoardCards"), "\n");

  std::string max_raises = ParameterValue<std::string>("maxRaises");
  if (max_raises != kEmptyString) {
    absl::StrAppend(&generated_gamedef, "maxRaises = ", max_raises, "\n");
  }

  if (ParameterValue<std::string>("betting") == "limit") {
    std::string raise_size = ParameterValue<std::string>("raiseSize");
    if (raise_size != kEmptyString) {
      absl::StrAppend(&generated_gamedef, "raiseSize = ", raise_size, "\n");
    }
  } else if (ParameterValue<std::string>("betting") == "nolimit") {
    std::string stack = ParameterValue<std::string>("stack");
    if (stack != kEmptyString) {
      absl::StrAppend(&generated_gamedef, "stack = ", stack, "\n");
    }
  } else {
    SpielFatalError(absl::StrCat("betting should be limit or nolimit, not ",
                                 ParameterValue<std::string>("betting")));
  }

  absl::StrAppend(&generated_gamedef,
                  "blind = ", ParameterValue<std::string>("blind"), "\n");
  absl::StrAppend(&generated_gamedef, "END GAMEDEF\n");
  std::cerr << "Generated gamedef for Universal Poker:\n"
            << generated_gamedef << std::endl;
  return generated_gamedef;
}

const char *actions = "0df0c000p0000000a000000000000000h0000000000000000000000000000000b000000000000000000000000000000000000000000000000000000000000000w0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000t";

void UniversalPokerState::ApplyChoiceAction(ActionType action_type) {
  SPIEL_CHECK_GE(cur_player_, 0);  // No chance not terminal.
  assert((possibleActions_ & action_type) > 0);

  actionSequence_ += (char)actions[action_type];
  switch (action_type) {
    case ACTION_FOLD:
      acpc_state_.DoAction(acpc_cpp::ACPCState::ACPCActionType::ACPC_FOLD, 0);
      break;
    case ACTION_CHECK_CALL:
      acpc_state_.DoAction(acpc_cpp::ACPCState::ACPCActionType::ACPC_CALL, 0);
      break;
    case ACTION_BET:
      acpc_state_.DoAction(acpc_cpp::ACPCState::ACPCActionType::ACPC_RAISE,
                           potSize_);
      break;
    case ACTION_ALL_IN:
      acpc_state_.DoAction(acpc_cpp::ACPCState::ACPCActionType::ACPC_RAISE,
                           allInSize_);
      break;
    case ACTION_BET_HALF_POT:
      acpc_state_.DoAction(acpc_cpp::ACPCState::ACPCActionType::ACPC_RAISE,
              halfpotSize_);
      break;
    case ACTION_OFF_ABS:
      acpc_state_.DoAction(acpc_cpp::ACPCState::ACPCActionType::ACPC_RAISE,
              offabsSize_);
      break;
    case ACTION_BET_POT:
      acpc_state_.DoAction(acpc_cpp::ACPCState::ACPCActionType::ACPC_RAISE,
              betpotSize_);
      break;
    case ACTION_BET_DOUBLE_POT:
      acpc_state_.DoAction(acpc_cpp::ACPCState::ACPCActionType::ACPC_RAISE,
              doublepotSize_);
      break;
    case ACTION_DEAL:
    default:
      assert(false);
      break;
  }

  _CalculateActionsAndNodeType();
}

void UniversalPokerState::_CalculateActionsAndNodeType() {
  possibleActions_ = 0;

  if (acpc_state_.IsFinished()) {
    if (acpc_state_.NumFolded() >= acpc_game_->GetNbPlayers() - 1) {
      // All players except one has fold.
      cur_player_ = kTerminalPlayerId;
    } else {
      if (board_cards_.NumCards() <
          acpc_game_->GetNbBoardCardsRequired(acpc_state_.GetRound())) {
        cur_player_ = kChancePlayerId;
        possibleActions_ = ACTION_DEAL;
        return;
      }
      // Showdown!
      cur_player_ = kTerminalPlayerId;
    }

  } else {
    // Check for sth to deal
    // 1. We still need to deal cards if a player still has missing cards.
    // Because we deal from 0 to num_players - 1, we can just check the last
    // player.
    if (hole_cards_[acpc_game_->GetNbPlayers() - 1].NumCards() <
        acpc_game_->GetNbHoleCardsRequired()) {
      cur_player_ = kChancePlayerId;
      possibleActions_ = ACTION_DEAL;
      return;
    }
    // 2. We need to deal a public card.
    if (board_cards_.NumCards() <
        acpc_game_->GetNbBoardCardsRequired(acpc_state_.GetRound())) {
      cur_player_ = kChancePlayerId;
      possibleActions_ = ACTION_DEAL;
      return;
    }

    // Check for CHOICE Actions
    cur_player_ = acpc_state_.CurrentPlayer();
    if (acpc_state_.IsValidAction(
            acpc_cpp::ACPCState::ACPCActionType::ACPC_FOLD, 0)) {
      possibleActions_ |= ACTION_FOLD;
    }
    if (acpc_state_.IsValidAction(
            acpc_cpp::ACPCState::ACPCActionType::ACPC_CALL, 0)) {
      possibleActions_ |= ACTION_CHECK_CALL;
    }

    int32_t min_bet = 0;
    potSize_ = 0;   // TODO: 这个是bet现有的currentPot
    allInSize_ = 0;
    halfpotSize_ = 0;   // raise 0.5 * currentPot
    offabsSize_ = 0;
    betpotSize_ = 0;    // raise 1 * currentPot
    doublepotSize_ = 0; // raise 2 * currentPot

    bool valid_to_raise = acpc_state_.RaiseIsValid(&min_bet, &allInSize_);
    if (betting_abstraction_ == BettingAbstraction::kFC) return;
    if (valid_to_raise) {
      // It's always valid to bet the pot.
      possibleActions_ |= ACTION_BET;
      if (acpc_game_->IsLimitGame()) {
        potSize_ = 0;
      } else {
        int32_t currentPot =
            acpc_state_.MaxSpend() *
            (acpc_game_->GetNbPlayers() - acpc_state_.NumFolded());
        potSize_ = currentPot > min_bet ? currentPot : min_bet;
        potSize_ = allInSize_ < potSize_ ? allInSize_ : potSize_;
        if (allInSize_ > potSize_) {
          possibleActions_ |= ACTION_ALL_IN;
        }
        halfpotSize_ = acpc_state_.MaxSpend() + int(currentPot / 2);
        if (halfpotSize_ >= min_bet && halfpotSize_ < allInSize_) {
          possibleActions_ |= ACTION_BET_HALF_POT;
        }
        betpotSize_ = acpc_state_.MaxSpend() + currentPot;
        if (betpotSize_ >= min_bet && betpotSize_ < allInSize_) {
          possibleActions_ |= ACTION_BET_POT;
        }
        doublepotSize_ = acpc_state_.MaxSpend() + 2 * currentPot;
        if (doublepotSize_ >= min_bet && doublepotSize_ < allInSize_) {
          possibleActions_ |= ACTION_BET_DOUBLE_POT;
        }
        if (CheckInOffAbsInformationState(InformationStateString(cur_player_))) {
          offabsSize_ = GetOffAbsInformationStateRaise(InformationStateString(cur_player_));
          if (offabsSize_ >= min_bet && offabsSize_ < allInSize_) {
              possibleActions_ |= ACTION_OFF_ABS;
          }
        }
      }
    }
  }
}

const int UniversalPokerState::GetPossibleActionCount() const {
  // _builtin_popcount(int) function is used to count the number of one's
  return __builtin_popcount(possibleActions_);
}

std::ostream &operator<<(std::ostream &os, const BettingAbstraction &betting) {
  switch (betting) {
    case BettingAbstraction::kFC: {
      os << "BettingAbstration: FC";
      break;
    }
    case BettingAbstraction::kFCPA: {
      os << "BettingAbstration: FCPA";
      break;
    }
    default:
      SpielFatalError("Unknown betting abstraction.");
      break;
  }
  return os;
}

}  // namespace abstracted_poker
}  // namespace universal_poker
}  // namespace open_spiel
