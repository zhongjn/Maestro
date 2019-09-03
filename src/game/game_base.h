#pragma once
#include <vector>
#include <cassert>
#include <string>
#include <random>

namespace Maestro {
    using namespace std;

    enum class Color : int {
        None = 0,
        A = 1,
        B = 2
    };

    inline Color another_color(Color color) {
        assert(color != Color::None);
        return Color(3 - int(color));
    }

    // 当前状态
    struct Status {
        bool end = false;
        Color winner = Color::None;
    };

    template<typename TGame>
    struct Move {
        bool operator==(const TGame& another) const { return false; }
    };

    template<typename TGame>
    class IGame {
        int steps = 0;
    public:
        // using TMov = _TMov;
        virtual void move(Move<TGame> mov) = 0;
        virtual Color get_color() const = 0;
        virtual Status get_status() const = 0;
        virtual size_t get_hash() const = 0;
        virtual bool is_legal_move(Move<TGame> m) const = 0;
        virtual vector<Move<TGame>> get_all_legal_moves() const = 0;
        virtual bool could_transfer_to(const TGame& another) const = 0;
        virtual bool operator==(const TGame& another) const = 0;
        virtual string to_string() const { return "not implemented"; }
    };

    template<typename TGame>
    struct MovePrior {
        Move<TGame> move;
        float p;
    };

    // （p，v）
    template<typename TGame>
    struct Evaluation {
        std::vector<MovePrior<TGame>> p;
        float v = 0;
        void take_top(int k) {
            if (p.size() > k) {
                struct MoveCompare {
                    bool cmp(const MovePrior<TGame>& m1, const MovePrior<TGame>& m2) {
                        return m1.p > m2.p;
                    }
                };
                sort(p.begin(), p.end(), MoveCompare());
                p.resize(k);
            }
        }
    };

    template<typename TGame>
    class IEvaluator {
    public:
        // 注意：
        // 评估出来的v，是站在game的当前方的视角看的（即将落子的那一方）
        virtual Evaluation<TGame> evaluate(const TGame& game, minstd_rand& rand_eng) = 0;
        virtual vector<Evaluation<TGame>> evaluate(const vector<TGame*>& games, minstd_rand& rand_eng) {
            vector<Evaluation<TGame>> evals;
            for (auto* g : games) {
                evals.push_back(evaluate(*g, rand_eng));
            }
            return evals;
        }
    };

    // Sample here
    class SampleGame;

    template<>
    struct Move<SampleGame> {
        bool operator==(const Move<SampleGame>& another) const { return true; }
    };

    class SampleGame final : public IGame<SampleGame> {
    public:
        void move(Move<SampleGame> mov) override {}
        Color get_color() const override { return Color::None; }
        Status get_status() const override { return Status(); }
        size_t get_hash() const override { return 0; }
        bool could_transfer_to(const SampleGame& another) const override { return true; }
        bool is_legal_move(Move<SampleGame> m) const override { return false; }
        vector<Move<SampleGame>> get_all_legal_moves() const override { return vector<Move<SampleGame>>(); }
        bool operator==(const SampleGame& another) const override { return true; }
        SampleGame() = default;
    };

    class SampleGameEvaluator final : public IEvaluator<SampleGame> {
    public:
        Evaluation<SampleGame> evaluate(const SampleGame& game) {
            return Evaluation<SampleGame>();
        }
    };

}