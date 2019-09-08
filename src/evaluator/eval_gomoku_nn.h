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

        Evaluation<Gomoku> evaluate(const Gomoku& game, minstd_rand& rand_eng)
        {
            // TODO @wsh
            // load image data saved in binary
            vector<Gomoku*> gs;
            gs.push_back((Gomoku*)&game);

            return evaluate(gs, rand_eng)[0];
        }
        
        vector<Evaluation<Gomoku>> evaluate(const vector<Gomoku*>& games, minstd_rand& rand_eng)
        {
            std::vector<torch::jit::IValue> inputs;
            std::vector<int> rot_types;
            inputs.reserve(games.size());
            rot_types.reserve(games.size());
            at::Tensor in_data = torch::zeros({ (int)games.size(), 2, 15, 15 });
            float* d = (float*)in_data.data<float>();
            const Gomoku::HalfBoard * b0, * b1;
            size_t index;
            for (int i = 0; i < games.size(); ++i) {
                b0 = &(games[i]->black);
                b1 = &(games[i]->white);
                if (games[i]->get_color() == Color::B) {
                    b0 = &(games[i]->white);
                    b1 = &(games[i]->black);
                }
                rot_types.push_back(rand_eng() % 8);
                for (int r = 0; r < BOARD_SIZE; ++r) {
                    for (int c = 0; c < BOARD_SIZE; ++c) {
                        index = rot(r * BOARD_SIZE + c, rot_types[i]);
                        d[2 * i * BOARD_SIZE * BOARD_SIZE + index] = b0->get(r, c) ? 1 : 0;
                        d[(2 * i + 1) * BOARD_SIZE * BOARD_SIZE + index] = b1->get(r, c) ? 1 : 0;
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
                    index = rot(m.row * BOARD_SIZE + m.col, rot_types[i]);
                    e.p.push_back(MovePrior<Gomoku>{m, 
                        pdata[BOARD_SIZE * BOARD_SIZE * i + index]});
                }
                e.v = vdata[i];
                ret.push_back(e);
            }
            return ret;
        }

    private:
        size_t rot(size_t index, int rot_type)
        {
            size_t ret;
            assert(index < 225);
            switch (rot_type) {
            case 0:
                ret = index;
                break;
            case 1:
                ret = _index_arr[0][index];
                break;
            case 2:
                ret = _index_arr[1][index];
                break;
            case 3:
                ret = _index_arr[2][index];
                break;
            case 4:
                ret = _index_arr[3][index];
                break;
            case 5:
                ret = _index_arr[3][_index_arr[0][index]];
                break;
            case 6:
                ret = _index_arr[3][_index_arr[1][index]];
                break;
            case 7:
                ret = _index_arr[3][_index_arr[2][index]];
                break;
            default:
                assert(0);
                break;
            }
            return ret;
        }

        torch::jit::script::Module _module;
        size_t _index_arr[4][BOARD_SIZE * BOARD_SIZE] = {
            // y轴对称
            {14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 
            29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 
            44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 
            59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 
            74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 
            89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 
            104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 
            119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 
            134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120, 
            149, 148, 147, 146, 145, 144, 143, 142, 141, 140, 139, 138, 137, 136, 135, 
            164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 
            179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 
            194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 
            209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 
            224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210},
            // x轴对称
            {210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 
            195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 
            180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 
            165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 
            150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 
            135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 
            120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 
            105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 
            90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 
            75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 
            60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 
            45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 
            30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
            // 中心对称
            {224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 
            209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 
            194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 
            179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 
            164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 
            149, 148, 147, 146, 145, 144, 143, 142, 141, 140, 139, 138, 137, 136, 135, 
            134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120, 
            119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 
            104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 
            89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 
            74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 
            59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 
            44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 
            29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 
            14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
            // 主对角线对称
            {0, 15, 30, 45, 60, 75, 90, 105, 120, 135, 150, 165, 180, 195, 210, 
            1, 16, 31, 46, 61, 76, 91, 106, 121, 136, 151, 166, 181, 196, 211, 
            2, 17, 32, 47, 62, 77, 92, 107, 122, 137, 152, 167, 182, 197, 212, 
            3, 18, 33, 48, 63, 78, 93, 108, 123, 138, 153, 168, 183, 198, 213, 
            4, 19, 34, 49, 64, 79, 94, 109, 124, 139, 154, 169, 184, 199, 214, 
            5, 20, 35, 50, 65, 80, 95, 110, 125, 140, 155, 170, 185, 200, 215, 
            6, 21, 36, 51, 66, 81, 96, 111, 126, 141, 156, 171, 186, 201, 216, 
            7, 22, 37, 52, 67, 82, 97, 112, 127, 142, 157, 172, 187, 202, 217, 
            8, 23, 38, 53, 68, 83, 98, 113, 128, 143, 158, 173, 188, 203, 218, 
            9, 24, 39, 54, 69, 84, 99, 114, 129, 144, 159, 174, 189, 204, 219, 
            10, 25, 40, 55, 70, 85, 100, 115, 130, 145, 160, 175, 190, 205, 220, 
            11, 26, 41, 56, 71, 86, 101, 116, 131, 146, 161, 176, 191, 206, 221, 
            12, 27, 42, 57, 72, 87, 102, 117, 132, 147, 162, 177, 192, 207, 222,
            13, 28, 43, 58, 73, 88, 103, 118, 133, 148, 163, 178, 193, 208, 223, 
            14, 29, 44, 59, 74, 89, 104, 119, 134, 149, 164, 179, 194, 209, 224}
        };
    };
}
