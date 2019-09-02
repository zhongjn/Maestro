#pragma once
#include "../game/game_gomoku.h"
#include <torch/script.h>
#include <torch/torch.h>
#include <iostream>
#include <fstream>
#include <io.h>

namespace Maestro {
    class NNGomokuEvaluator final : public IEvaluator<Gomoku> {
        const bool USE_GPU = true;
    public:
        NNGomokuEvaluator(std::string lib_path)
        {
            if (_access(lib_path.c_str(), 0) != 0) {
                printf("Cannot find %s\n", lib_path.c_str());
            }
            /*
            _module = torch::jit::load(lib_path.c_str());
            _module.to(torch::kCUDA);
            */
            
            try {
                _module = torch::jit::load(lib_path.c_str());
                _module.to(USE_GPU ? torch::kCUDA : torch::kCPU);
            } catch (c10::Error& e) {
                std::string s(e.what());
                printf("ERROR: %s\n", s.c_str());
            }
        }

        Evaluation<Gomoku> evaluate(const Gomoku& game)
        {
            // TODO @wsh
            // load image data saved in binary
            vector<Gomoku*> gs;
            gs.push_back((Gomoku*)&game);

            return evaluate(gs)[0];
        }
        
        vector<Evaluation<Gomoku>> evaluate(const vector<Gomoku*>& games)
        {
            std::vector<torch::jit::IValue> inputs;
            auto in_data = torch::zeros({ (int)games.size(), 2, 15, 15 });
            float* d = (float*)in_data.data<float>();
            const Gomoku::HalfBoard * b0, * b1;

            for (int i = 0; i < games.size(); ++i) {
                b0 = &(games[i]->black);
                b1 = &(games[i]->white);
                if (games[i]->get_color() == Color::B) {
                    b0 = &(games[i]->white);
                    b1 = &(games[i]->black);
                }
                for (int r = 0; r < BOARD_SIZE; ++r) {
                    for (int c = 0; c < BOARD_SIZE; ++c) {
                        d[2 * i * BOARD_SIZE * BOARD_SIZE + r * BOARD_SIZE + c] = b0->get(r, c) ? 1 : 0;
                        d[(2 * i + 1) * BOARD_SIZE * BOARD_SIZE + r * BOARD_SIZE + c] = b1->get(r, c) ? 1 : 0;
                    }
                }
            }
            inputs.push_back(in_data.to(USE_GPU ? torch::kCUDA : torch::kCPU));
            // Execute the model and turn its output into a tensor.
            torch::jit::Stack output = _module.forward(inputs).toTuple()->elements();

            // get output
            auto tp = output[0].toTensor().to(torch::kCPU), tv = output[1].toTensor().to(torch::kCPU);
            float* pdata = tp.data<float>();
            float* vdata = tv.data<float>();
            vector<Evaluation<Gomoku>> ret;
           
            for (int i = 0; i < games.size(); ++i) {
                vector<Move<Gomoku>> ms = games[i]->get_all_legal_moves();
                Evaluation<Gomoku> e;
                e.p.reserve(ms.size());
                for (Move<Gomoku>& m : ms) {
                    e.p.push_back(MovePrior<Gomoku>{m, 
                        pdata[BOARD_SIZE * BOARD_SIZE * i + m.row * BOARD_SIZE + m.col]});
                }
                e.v = vdata[i];
                ret.push_back(e);
            }
            return ret;
        }

    private:
        torch::jit::script::Module _module;
    };
}
