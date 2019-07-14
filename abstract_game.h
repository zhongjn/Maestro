#pragma once
#include <vector>

namespace Maestro {
    enum class Player {
        None = 0,
        A = 1,
        B = 2
    };

    // 当前状态
    struct Status {
        bool end;
        Player winner;
    };

    // 继承时传入子类类型到TSelf，实现静态多态
    template<typename TSelf, typename TObsv, typename TMov>
    class IGame {
        int steps = 0;

    public:

        void move(TMov mov) {
            return static_cast<TSelf*>(this)->move(mov);
        }

        TObsv observation(Player pov) const {
            return static_cast<TSelf*>(this)->observation(pov);
        }

        Status status() const {
            return static_cast<TSelf*>(this)->status();
        }
    };

    template<typename TMov>
    struct MovePrior {
        TMov move;
        float p;
    };

    // （p，v）
    template<typename TMov>
    struct Evaluation {
        std::vector<MovePrior<TMov>> p;
        float v;
    };

    // 传入自身实现静态多态
    template<typename TSelf, typename TObsv, typename TMov>
    class IEvaluator {
    public:
        Evaluation<TMov> evaluate(const TObsv& obsv) {
            return static_cast<TSelf*>(this)->evaluate(obsv);
        }
    };
}