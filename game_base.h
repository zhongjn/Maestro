#pragma once
#include <vector>

namespace Maestro {
    enum class Player {
        None = 0,
        A = 1,
        B = 2
    };

    // ��ǰ״̬
    struct Status {
        bool end;
        Player winner;
    };


    template<typename TGame>
    struct Observation {

    };

    template<typename TGame>
    struct Move {

    };

    template<typename TGame>
    class IGame {
        int steps = 0;
    public:
        // using TMov = _TMov;
        virtual void move(Move<TGame> mov) = 0;
        virtual Observation<TGame> get_obsv(Player pov) const = 0;
        virtual Player get_player() const = 0;
        virtual Status get_status() const = 0;
        virtual size_t get_hash() const = 0;
    };

    template<typename TGame>
    struct MovePrior {
        Move<TGame> move;
        float p;
    };

    // ��p��v��
    template<typename TGame>
    struct Evaluation {
        std::vector<MovePrior<TGame>> p;
        float v;
    };

    template<typename TGame>
    class IEvaluator {
    public:
        virtual Evaluation<TGame> evaluate(const Observation<TGame>& obsv) = 0;
    };

    // Sample here
    class SampleGame;

    template<>
    struct Observation<SampleGame> {

    };

    template<>
    struct Move<SampleGame> {

    };

    class SampleGame : public IGame<SampleGame> {
    public:
        void move(Move<SampleGame> mov) {}
        Observation<SampleGame> get_obsv(Player pov) const { return Observation<SampleGame>(); }
        Player get_player() const { return Player::None; }
        Status get_status() const { return Status(); }
        size_t get_hash() const { return 0; }
    };

    class SampleGameEvaluator : public IEvaluator<SampleGame> {
    public:
        Evaluation<SampleGame> evaluate(const Observation<SampleGame>& obsv) {
            return Evaluation<SampleGame>();
        }
    };
}