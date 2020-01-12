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

#include <unistd.h>

#include <memory>
#include <random>
#include <open_spiel/games/abstracted_poker.h>

#include "open_spiel/abseil-cpp/absl/flags/flag.h"
#include "open_spiel/abseil-cpp/absl/flags/parse.h"
#include "open_spiel/abseil-cpp/absl/random/uniform_int_distribution.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/algorithms/external_sampling_mccfr.h"
#include "open_spiel/games/universal_poker/handIndex/index.h"
#include "abstracted_poker_search/abstracted_poker_search.h"

using open_spiel::universal_poker::abstracted_poker::AbstractedPokerSearchGame;
using open_spiel::universal_poker::abstracted_poker::AbstractedPokerSearchState;

ABSL_FLAG(bool, show_legals, false, "Show the legal moves.");

ABSL_FLAG(std::string, betting, "nolimit", "On of ‘limit’ or ‘nolimit’.");
ABSL_FLAG(int, numPlayers, 2, "Number of players.");
ABSL_FLAG(int, numRounds, 4, "Number of rounds.");
ABSL_FLAG(std::string, stack, "20000 20000", "Stack size of players.");
ABSL_FLAG(std::string, blind, "100 50", "Big blind and small blind.");
ABSL_FLAG(std::string, firstPlayer, "2 1 1 1", "First player in each round.");
ABSL_FLAG(int, numSuits, 4, "Number of suits.");
ABSL_FLAG(int, numRanks, 13, "Number of ranks.");
ABSL_FLAG(int, numHoleCards, 2, "Number of hole cards.");
ABSL_FLAG(std::string, numBoardCards, "0 3 1 1", "Number of board cards per round");

// for limit games only
ABSL_FLAG(std::string, raiseSize, "100 100", "Raise size for each round.");
ABSL_FLAG(std::string, maxRaises, "", "Max raise times for each round.");

ABSL_FLAG(std::string, bettingAbstraction, "fcpa", "Which actions are available to the player, 'fcpa' or 'fc'.");

void PrintLegalActions(const open_spiel::State &state,
                       open_spiel::Player player,
                       const std::vector<open_spiel::Action> &movelist) {
    std::cerr << "Legal moves for player " << player << ":" << std::endl;
    for (open_spiel::Action action : movelist) {
        std::cerr << "  " << state.ActionToString(player, action) << std::endl;
    }
}

int main(int argc, char **argv) {
    absl::ParseCommandLine(argc, argv);

    bool show_legals = absl::GetFlag(FLAGS_show_legals);

    open_spiel::GameParameters params;
    params["betting"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_betting));
    params["numPlayers"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_numPlayers));
    params["numRounds"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_numRounds));
    params["stack"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_stack));
    params["blind"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_blind));
    params["firstPlayer"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_firstPlayer));
    params["numSuits"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_numSuits));
    params["numRanks"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_numRanks));
    params["numHoleCards"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_numHoleCards));
    params["numBoardCards"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_numBoardCards));
    params["raiseSize"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_raiseSize));
    params["maxRaises"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_maxRaises));
    params["bettingAbstraction"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_bettingAbstraction));

    // Random number generator.
    int seed = 0;
    std::mt19937 rng(seed ? seed : time(0));

    // 交互式的game
    // Create the game.
//    std::cerr << "Creating game..\n" << std::endl;
//    std::shared_ptr<AbstractedPokerSearchGame> game(new AbstractedPokerSearchGame(params, 0, seed));
//    std::shared_ptr<AbstractedPokerSearchState> state = game->NewInitialState();
//
//    while (!state->IsTerminal()) {
//        std::cerr << "AI turn\n";
//        std::cerr << state->ObservationString() << std::endl;
//        std::cerr << state->InformationStateString() << std::endl;
//        for (auto a : state->LegalActions()) {
//            std::cerr << a << " ";
//        }
//        std::cerr << std::endl;
//
//        open_spiel::Action ai;
//        std::cin >> ai;
//
//        state = state->Child(ai);
//    }

    // 用于 Abstracted Poker 的测试
    // Create the game.
    std::cerr << "Creating game..\n" << std::endl;
    std::shared_ptr<const open_spiel::Game> game = open_spiel::LoadGame("abstracted_poker", params);
    if (!game) {
        std::cerr << "problem with loading game, exiting..." << std::endl;
        return -1;
    }
    std::unique_ptr<open_spiel::State> state = game->NewInitialState();

    std::vector<open_spiel::Action> validActions = state->LegalActions();
    std::cout << "LegalActions: ";
    for (open_spiel::Action a : validActions) {
        std::cout << state->ActionToString(a) << " ";
    }
    std::cout << "Finished" << std::endl;

    state = state->Child(0);
    validActions = state->LegalActions();
    std::cout << "LegalActions: ";
    for (open_spiel::Action a : validActions) {
        std::cout << state->ActionToString(a) << " ";
    }
    std::cout << "Finished" << std::endl;

    state = state->Child(0);
    state = state->Child(1);
    state = state->Child(2);
    state = state->Child(3);

    std::cout << state->InformationStateString() << std::endl;
    std::cout << state->ObservationString() << std::endl;
    validActions = state->LegalActions();
    std::cout << "LegalActions: ";
    for (open_spiel::Action a : validActions) {
        std::cout << state->ActionToString(a) << " ";
    }
    std::cout << "Finished" << std::endl;

    state = state->Child(2);
    std::cout << state->InformationStateString() << std::endl;
    std::cout << state->ObservationString() << std::endl;
    validActions = state->LegalActions();
    std::cout << "LegalActions: ";
    for (open_spiel::Action a : validActions) {
        std::cout << state->ActionToString(a) << " ";
    }
    std::cout << "Finished" << std::endl;

    state = state->Child(2);
    std::cout << state->InformationStateString() << std::endl;
    validActions = state->LegalActions();
    std::cout << "LegalActions: ";
    for (open_spiel::Action a : validActions) {
        std::cout << state->ActionToString(a) << " ";
    }
    std::cout << "Finished" << std::endl;

    state = state->Child(6);
    std::cout << state->InformationStateString() << std::endl;
    validActions = state->LegalActions();
    std::cout << "LegalActions: ";
    for (open_spiel::Action a : validActions) {
        std::cout << state->ActionToString(a) << " ";
    }
    std::cout << "Finished" << std::endl;

    state = state->Child(4);
    std::cout << state->InformationStateString() << std::endl;
    validActions = state->LegalActions();
    std::cout << "LegalActions: ";
    for (open_spiel::Action a : validActions) {
        std::cout << state->ActionToString(a) << " ";
    }
    std::cout << "Finished" << std::endl;


//     open_spiel::algorithms::ExternalSamplingMCCFRSolver solver(*game);
//     std::cerr << "Starting ExternalSamplingMCCFR on " << game->GetType().short_name
//               << "..." << std::endl;
//     for (int i = 0; i < 1; i++) {
//         solver.RunIteration();
//     }

//    std::cerr << "Starting new game..." << std::endl;
//    std::unique_ptr<open_spiel::State> state = game->NewInitialState();
//
//    std::cerr << "Initial state:" << std::endl;
//    std::cerr << "State:" << std::endl
//              << state->ToString() << std::endl;
//
//    while (!state->IsTerminal()) {
//        std::cerr << "player " << state->CurrentPlayer() << std::endl;
//
//        if (state->IsChanceNode()) {
//            // Chance node; sample one according to underlying distribution.
//            std::vector<std::pair<open_spiel::Action, double>> outcomes =
//                state->ChanceOutcomes();
//            open_spiel::Action action =
//                open_spiel::SampleAction(
//                    outcomes, std::uniform_real_distribution<double>(0.0, 1.0)(rng))
//                    .first;
//            std::cerr << "sampled outcome: "
//                      << state->ActionToString(open_spiel::kChancePlayerId, action)
//                      << std::endl;
//            state->ApplyAction(action);
//        } else if (state->IsSimultaneousNode()) {
//            // open_spiel::Players choose simultaneously?
//            std::vector<open_spiel::Action> joint_action;
//            std::vector<double> infostate;
//
//            // Sample a action for each player
//            for (auto player = open_spiel::Player{0}; player < game->NumPlayers();
//                 ++player) {
//                std::vector<open_spiel::Action> actions = state->LegalActions(player);
//                if (show_legals) {
//                    PrintLegalActions(*state, player, actions);
//                }
//
//                absl::uniform_int_distribution<> dis(0, actions.size() - 1);
//                open_spiel::Action action = actions[dis(rng)];
//                joint_action.push_back(action);
//                std::cerr << "player " << player << " chose "
//                          << state->ActionToString(player, action) << std::endl;
//            }
//
//            state->ApplyActions(joint_action);
//        } else {
//            // Decision node, sample one uniformly.
//            auto player = state->CurrentPlayer();
//            std::vector<open_spiel::Action> actions = state->LegalActions();
//            if (show_legals) {
//                PrintLegalActions(*state, player, actions);
//            }
//
//            absl::uniform_int_distribution<> dis(0, actions.size() - 1);
//            auto action = actions[dis(rng)];
//            std::cerr << "chose action: " << state->ActionToString(player, action)
//                      << std::endl;
//            state->ApplyAction(action);
//        }
//
//        std::cerr << "State: " << std::endl
//                  << state->ToString() << std::endl;
//    }
//
//    auto returns = state->Returns();
//    for (auto p = open_spiel::Player{0}; p < game->NumPlayers(); p++) {
//        std::cerr << "Final return to player " << p << " is " << returns[p]
//                  << std::endl;
//    }
}
