#pragma once
#include <vector>
#include <memory>
#include "../game/game_base.h"
#include <cassert>
#include <random>
namespace Maestro {
    using namespace std;

    template<typename TGame>
    class IMonteCarloSearch {
    protected:
        minstd_rand _rnd_eng;
    public:

        struct MoveVisit {
            Move<TGame> move;
            int visit_count;
        };

        struct MoveVisitProb {
            Move<TGame> move;
            float p;
        };

        virtual void simulate(int k) = 0;
        virtual vector<MoveVisit> get_moves() const = 0;
        virtual float get_value(Color color) const = 0;
        virtual TGame get_game_snapshot() const = 0;
        virtual void move(Move<TGame> move) = 0;

        Move<TGame> pick_move(float temp) {
            assert(temp >= 0.0f);

            if (temp == 0.0f) {
                auto moves = get_moves();
                int max_visit = -1;
                Move<TGame> max_move;
                assert(moves.size() > 0);
                for (auto& m : moves) {
                    if (m.visit_count > max_visit) {
                        max_move = m.move;
                    }
                }
                return max_move;
            }
            else {
                assert(temp >= 0.0f);
                uniform_real_distribution<float> dist;
                float r = dist(_rnd_eng);

                auto moves = get_moves_prob(temp);
                assert(moves.size() > 0);

                float accum = 0;
                int index = 0;
                for (auto& move : moves) {
                    accum += move.p;
                    if (accum > r) {
                        break;
                    }
                    index++;
                }

                return moves[index].move;
            }
        }

        void move_best() {
            auto moves = get_moves();
            int max_visit = -1;
            Move<TGame> max_move;
            assert(moves.size() > 0);
            for (auto& m : moves) {
                if (m.visit_count > max_visit) {
                    max_move = m.move;
                }
            }

            move(max_move);
        }

        vector<MoveVisitProb> get_moves_prob(float temp) const {
            auto moves = get_moves();
            vector<float> visit;
            float sum = 0;
            
            vector<MoveVisitProb> move_prob;

            for (auto& mv : moves) {
                float v = pow(mv.visit_count, 1 / temp);
                sum += v;
                move_prob.push_back(MoveVisitProb{ mv.move, v });
            }

            // normalize
            for (auto& mv : move_prob) {
                mv.p /= sum;
            }

            return move_prob;
        }

        void move_random(float temp) {
            assert(temp >= 0.0f);
            uniform_real_distribution<float> dist;
            float r = dist(_rnd_eng);

            auto moves = get_moves_prob();
            assert(moves.size() > 0);

            float accum = 0;
            int index = 0;
            for (auto& move : moves) {
                accum += move.p;
                if (accum > r) {
                    break;
                }
                index++;
            }

            move(moves[index].move);
        }

    };
}