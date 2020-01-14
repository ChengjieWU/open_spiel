//
// Created by wuchengjie on 2020/1/12.
//

#ifndef OPEN_SPIEL_ABSTRACTED_POKER_SEARCH_H
#define OPEN_SPIEL_ABSTRACTED_POKER_SEARCH_H

#include "open_spiel/games/abstracted_poker.h"

namespace open_spiel::universal_poker::abstracted_poker {


class AbstractedPokerSearchGame;


class AbstractedPokerSearchState {
protected:
    UniversalPokerState state_;
    Player AI;
    std::shared_ptr<std::mt19937> rng_;
    std::uniform_real_distribution<double> dist_;
    std::unique_ptr<AbstractedPokerSearchState> ChildStep(Action action) const;

public:
    explicit AbstractedPokerSearchState(std::shared_ptr<const Game> game, Player AI,  std::shared_ptr<std::mt19937> rng);
    AbstractedPokerSearchState(const AbstractedPokerSearchState &) = default;
    std::unique_ptr<AbstractedPokerSearchState> Clone() const;

    bool IsTerminal() const;
    bool IsChanceNode() const;
    Player CurrentPlayer() const;
    std::vector<std::pair<Action, double>> ChanceOutcomes() const;
    std::string InformationStateString() const;
    std::string ObservationString() const;
    std::string PlayingString() const;
    std::vector<Action> LegalActions() const;
    std::string ToString() const;

    std::unique_ptr<AbstractedPokerSearchState> ChildPass();
    std::unique_ptr<AbstractedPokerSearchState> Child(Action action);

    Action GetChanceAction();
    Action GetOpponentAction();
};


class AbstractedPokerSearchGame {
protected:
    std::shared_ptr<UniversalPokerGame> game_;
    Player AI;
    std::shared_ptr<std::mt19937> rng_;

public:
    explicit AbstractedPokerSearchGame(const GameParameters &params, Player AI, int seed);
    std::unique_ptr<AbstractedPokerSearchState> NewInitialState() const;
};


}   // namespace open_spiel::universal_poker::abstracted_poker

#endif //OPEN_SPIEL_ABSTRACTED_POKER_SEARCH_H
