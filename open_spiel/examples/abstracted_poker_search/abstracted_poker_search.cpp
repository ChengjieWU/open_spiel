//
// Created by wuchengjie on 2020/1/12.
//

#include "abstracted_poker_search.h"

#include <memory>

namespace open_spiel::universal_poker::abstracted_poker {

AbstractedPokerSearchState::AbstractedPokerSearchState(
        std::shared_ptr<const Game> game, Player AI, std::shared_ptr<std::mt19937> rng)
    : state_(game), AI(AI), rng_(rng) {}

std::unique_ptr<AbstractedPokerSearchState> AbstractedPokerSearchState::Clone() const {
    // return std::unique_ptr<AbstractedPokerSearchState>(new AbstractedPokerSearchState(*this));
    return std::make_unique<AbstractedPokerSearchState>(*this);
}

bool AbstractedPokerSearchState::IsTerminal() const {
    return state_.IsTerminal();
}

bool AbstractedPokerSearchState::IsChanceNode() const {
    return state_.IsChanceNode();
}

Player AbstractedPokerSearchState::CurrentPlayer() const {
    return state_.CurrentPlayer();
}

std::vector<std::pair<Action, double>> AbstractedPokerSearchState::ChanceOutcomes() const {
    return state_.ChanceOutcomes();
}

std::string AbstractedPokerSearchState::InformationStateString() const {
    return state_.InformationStateString(CurrentPlayer());
}

std::string AbstractedPokerSearchState::ObservationString() const {
    return state_.ObservationString(CurrentPlayer());
}

std::string AbstractedPokerSearchState::PlayingString() const {
    return state_.PlayingString(CurrentPlayer());
}

std::string AbstractedPokerSearchState::ToString() const {
    return state_.ToString();
}

std::vector<Action> AbstractedPokerSearchState::LegalActions() const {
    return state_.LegalActions();
}

std::unique_ptr<AbstractedPokerSearchState> AbstractedPokerSearchState::ChildStep(Action action) const {
    std::unique_ptr<AbstractedPokerSearchState> child = Clone();
    child->state_.ApplyAction(action);
    return child;
}

std::unique_ptr<AbstractedPokerSearchState> AbstractedPokerSearchState::ChildPass() {
    std::unique_ptr<AbstractedPokerSearchState> child = Clone();
    while (!child->IsTerminal() && child->CurrentPlayer() != child->AI) {
        if (child->IsChanceNode()) {
            child = child->ChildStep(child->GetChanceAction());
        }
        else if (child->CurrentPlayer() >= 0) {
            child = child->ChildStep(child->GetOpponentAction());
        }
    }
    return child;
}

Action AbstractedPokerSearchState::GetChanceAction() {
    SPIEL_CHECK_TRUE(IsChanceNode());
    Action action = SampleAction(ChanceOutcomes(), dist_(*rng_)).first;
    return action;
}

Action AbstractedPokerSearchState::GetOpponentAction() {
    SPIEL_CHECK_FALSE(IsChanceNode() || IsTerminal());
    SPIEL_CHECK_GE(CurrentPlayer(), 0);
    // TODO: So far, we print all information and get input by cmd
    std::cout << state_.PlayingString(CurrentPlayer()) << std::endl;

    if (state_.FoldIsValid()) {
        std::cout << "0: f; ";
    }
    if (state_.CallIsValid()) {
        std::cout << "1: c; ";
    }
    int32_t min_bet, max_bet;
    bool valid_to_raise = state_.GetValidToRaise(&min_bet, &max_bet);
    if (valid_to_raise) {
        std::cout << "valid raise interval: " << min_bet << ", " << max_bet << std::endl;
    }

    Action ret;
    std::cin >> ret;
    if (ret == 0 || ret == 1) {
        return ret;
    }
    else if (ret >= min_bet && ret <= max_bet) {
        if (state_.AddOffAbsInformationStateRaise(state_.InformationStateString(CurrentPlayer()), ret)) {
            return ActionType::kOffAbs;
        }
        SpielFatalError("Error! Off abs action already exists for this infostate!");
    }
    SpielFatalError("Illegal input!");
}

std::unique_ptr<AbstractedPokerSearchState> AbstractedPokerSearchState::Child(Action action) {
    std::unique_ptr<AbstractedPokerSearchState> child = ChildStep(action);
    child = child->ChildPass();
    return child;
}

std::unique_ptr<AbstractedPokerSearchState> AbstractedPokerSearchGame::NewInitialState() const {
    std::unique_ptr<AbstractedPokerSearchState> state =
            std::unique_ptr<AbstractedPokerSearchState>(new AbstractedPokerSearchState(
                    game_->shared_from_this(), AI, rng_));
    state = state->ChildPass();
    return state;
}

AbstractedPokerSearchGame::AbstractedPokerSearchGame(const GameParameters &params, Player AI, int seed)
    : AI(AI), rng_(new std::mt19937(seed ? seed : time(nullptr))) {
    // NOTE: these two are equivalent
    // game_ = std::shared_ptr<UniversalPokerGame>(new UniversalPokerGame(params));
    game_ = std::make_shared<UniversalPokerGame>(params);
}

}


