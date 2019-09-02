#pragma once
#include "../game/game_gomoku.h"
#include <dlpack/dlpack.h>
#include <tvm/runtime/module.h>
#include <tvm/runtime/registry.h>
#include <tvm/runtime/packed_func.h>
#include <iostream>
#include <fstream>
#include <io.h>

namespace Maestro {
    class NNGomokuEvaluator final : public IEvaluator<Gomoku> {
    public:
        NNGomokuEvaluator(std::string lib_path, std::string json_path, std::string params_path)
        {
            tvm::runtime::Module mod_dylib;

            if (_access(lib_path.c_str(), 0) != 0) {
                printf("cannot find module!\n");
            }
            mod_dylib = tvm::runtime::Module::LoadFromFile(lib_path);

            std::ifstream json_in(json_path, std::ios::in);
            std::string json_data((std::istreambuf_iterator<char>(json_in)), std::istreambuf_iterator<char>());
            json_in.close();

            // parameters in binary
            std::ifstream params_in(params_path, std::ios::binary);
            std::string params_data((std::istreambuf_iterator<char>(params_in)), std::istreambuf_iterator<char>());
            params_in.close();

            TVMByteArray params_arr;
            params_arr.data = params_data.c_str();
            params_arr.size = params_data.length();
            _dinfo.dtype_code = kDLFloat;
            _dinfo.dtype_bits = 32;
            _dinfo.dtype_lanes = 1;
            _dinfo.device_type = kDLCPU;
            _dinfo.device_id = 0;

            _module = (*tvm::runtime::Registry::Get("tvm.graph_runtime.create"))
                (json_data, mod_dylib, _dinfo.device_type, _dinfo.device_id);

            // get the function from the module(load patameters)
            tvm::runtime::PackedFunc load_params = _module.GetFunction("load_params");
            load_params(params_arr);
            // get the function from the module(set input data)
            _fsetin = _module.GetFunction("set_input");
            // get the function from the module(run it)
            _frun = _module.GetFunction("run");
            // get the function from the module(get output data)
            _fgetout = _module.GetFunction("get_output");
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
            DLTensor* x, * p, * v;
            int in_ndim = 4;
            int64_t in_shape[4] = { games.size(), 2, BOARD_SIZE, BOARD_SIZE };
            TVMArrayAlloc(in_shape, in_ndim, 
                _dinfo.dtype_code, _dinfo.dtype_bits, _dinfo.dtype_lanes, _dinfo.device_type, _dinfo.device_id, &x);
            int p_ndim = 2;
            int64_t p_shape[2] = { games.size(), BOARD_SIZE * BOARD_SIZE, };
            TVMArrayAlloc(p_shape, p_ndim, 
                _dinfo.dtype_code, _dinfo.dtype_bits, _dinfo.dtype_lanes, _dinfo.device_type, _dinfo.device_id, &p);
            int v_ndim = 2;
            int64_t v_shape[2] = { games.size(), 1, };
            TVMArrayAlloc(v_shape, v_ndim, 
                _dinfo.dtype_code, _dinfo.dtype_bits, _dinfo.dtype_lanes, _dinfo.device_type, _dinfo.device_id, &v);

            float* d = (float*)x->data;
            const Gomoku::HalfBoard * b0, * b1;

            for (int i = 0; i < games.size(); ++i) {
                const Gomoku::HalfBoard* b0 = &(games[i]->black), * b1 = &(games[i]->white);
                if (games[i]->get_color() == Color::B) {
                    b0 = &(games[i]->white);
                    b1 = &(games[i]->black);
                }
                for (int r = 0; r < BOARD_SIZE; ++r) {
                    for (int c = 0; c < BOARD_SIZE; ++c) {
                        d[i * BOARD_SIZE * BOARD_SIZE + r * BOARD_SIZE + c] = b0->get(r, c) ? 1 : 0;
                        d[(i + 1) * BOARD_SIZE * BOARD_SIZE + r * BOARD_SIZE + c] = b1->get(r, c) ? 1 : 0;
                    }
                }
            }

            _fsetin("input.1", x);
            _frun();
            _fgetout(0, p);
            _fgetout(1, v);

            // get output
            float* out = (float*)p->data;
            vector<Evaluation<Gomoku>> ret;
            for (int i = 0; i < games.size(); ++i) {
                vector<Move<Gomoku>> ms = games[i]->get_all_legal_moves();
                Evaluation<Gomoku> e;
                e.p.reserve(ms.size());
                for (Move<Gomoku>& m : ms) {
                    e.p.push_back(MovePrior<Gomoku>{m, out[m.row * 15 + m.col]});
                }
                e.v = *(float*)v->data;
                ret.push_back(e);
            }

            TVMArrayFree(x);
            TVMArrayFree(p);
            TVMArrayFree(v);
            return ret;
        }

    private:
        tvm::runtime::Module _module;
        tvm::runtime::PackedFunc _fsetin;
        tvm::runtime::PackedFunc _frun;
        tvm::runtime::PackedFunc _fgetout;
        struct {
            int dtype_code;
            int dtype_bits;
            int dtype_lanes;
            int device_type;
            int device_id;
        } _dinfo;
    };
}
