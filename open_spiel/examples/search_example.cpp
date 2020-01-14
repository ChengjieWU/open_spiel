//
// Created by wuchengjie on 2020/1/14.
//


#include <memory>

#include "open_spiel/abseil-cpp/absl/flags/flag.h"
#include "open_spiel/abseil-cpp/absl/flags/parse.h"
#include "open_spiel/abseil-cpp/absl/random/uniform_int_distribution.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/games/universal_poker/handIndex/index.h"
#include "abstracted_poker_search/abstracted_poker_search.h"

using open_spiel::universal_poker::abstracted_poker::AbstractedPokerSearchGame;
using open_spiel::universal_poker::abstracted_poker::AbstractedPokerSearchState;

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

ABSL_FLAG(bool, readCluster, false, "Whether to read infostate cluster abstraction");

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
    params["readCluster"] = open_spiel::GameParameter(absl::GetFlag(FLAGS_readCluster));

    // Random number generator.
    int seed = 0;

    // 交互式的game
    // Create the game.
    std::cerr << "Creating game..\n" << std::endl;
    std::shared_ptr<AbstractedPokerSearchGame> game(new AbstractedPokerSearchGame(params, 0, seed));
    std::shared_ptr<AbstractedPokerSearchState> state = game->NewInitialState();

    while (!state->IsTerminal()) {
        std::cerr << "AI turn\n";
        std::cerr << state->PlayingString() << std::endl;
        std::cerr << state->InformationStateString() << std::endl;
        for (auto a : state->LegalActions()) {
            std::cerr << a << " ";
        }
        std::cerr << std::endl;

        open_spiel::Action ai;
        std::cin >> ai;

        state = state->Child(ai);
    }
    std::cerr << state->ToString() << std::endl;

}
