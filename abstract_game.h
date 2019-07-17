#pragma once
#include <vector>

namespace Maestro {
    enum class Player {
        None = 0,
        A = 1,
        B = 2
    };

    // µ±Ç°×´Ì¬
    struct Status {
        bool end;
        Player winner;
    };

    template<typename TObsv, typename TMov>
    class IGame {
        int steps = 0;
    public:
        virtual void move(TMov mov) = 0;
        virtual TObsv get_obsv(Player pov) const = 0;
        virtual Player get_player() const = 0;
        virtual Status get_status() const = 0;
        virtual int get_hash() const = 0;
    };

    template<typename TMov>
    struct MovePrior {
        TMov move;
        float p;
    };

    // £¨p£¬v£©
    template<typename TMov>
    struct Evaluation {
        std::vector<MovePrior<TMov>> p;
        float v;
    };

    template<typename TObsv, typename TMov>
    class IEvaluator {
    public:
        virtual Evaluation<TMov> evaluate(const TObsv& obsv) = 0;
    };
}