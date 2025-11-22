#include <algorithm>
#include <thread>

#include "evaluate.h"
#include "misc.h"
#include "search.h"
#include "usi.h"

struct MovePicker {
    // 静止探索で使用
    MovePicker(const Position &pos_, Square recapSq) : pos(pos_) {
        if (pos.in_check())
            endMoves = generateMoves<EVASIONS>(pos, currentMoves);
        else
            endMoves = generateMoves<RECAPTURES>(pos, currentMoves, recapSq);
        }

    Move nextMove() {
        if (currentMoves == endMoves)
            return MOVE_NONE;
        return *currentMoves++;
    }

private:
    const Position &pos;
    ExtMove moves[MAX_MOVES], *currentMoves = moves, *endMoves = moves;
};

namespace Search {
    // 探索開始局面で思考対象とする指し手の集合。
    RootMoves rootMoves;

    // 持ち時間設定など。
    LimitsType Limits;

    // 今回のgoコマンドでの探索ノード数。
    uint64_t Nodes;

    // ベータカットが検出された回数
    Value BetaCut;

    // 探索中にこれがtrueになったら探索を即座に終了すること。
    bool Stop;

} // namespace Search

// 起動時に呼び出される。時間のかからない探索関係の初期化処理はここに書くこと。
void Search::init() {}

// isreadyコマンドの応答中に呼び出される。時間のかかる処理はここに書くこと。
void Search::clear() {}

// 同じ関数名で引数が異なる関数をオーバーロードという。
Value search(Position &pos, int depth, int ply_from_root);
Value search(Position &pos, Value alpha, Value beta, int depth, int ply_from_root);

// 探索を開始する
void Search::start_thinking(const Position &rootPos, StateListPtr &states, LimitsType limits) {
    Limits = limits;
    rootMoves.clear();
    Nodes = 0;
    Stop = false;

    for (Move move : MoveList<LEGAL_ALL>(rootPos))
        rootMoves.emplace_back(move);

    ASSERT_LV3(states.get());

    Position *pos_ptr = const_cast<Position *>(&rootPos);
    search(*pos_ptr);
}

// 探索本体
void Search::search(Position &pos) {
    // 探索で返す指し手
    Move bestMove = MOVE_RESIGN;
    int rootDepth = 1;

    if (rootMoves.size() == 0) {
        // 合法手が存在しない
        Stop = true;
        goto END;
    }

    /* ここから探索部を記述する */
    {
        /* 時間制御 */
        Color us = pos.side_to_move();
        std::thread *timerThread = nullptr;

        // 今回は秒読み以外の設定は考慮しない
        s64 endTime = Limits.byoyomi[us] - 150;

        timerThread = new std::thread([&] {
            while (Time.elapsed() < endTime && !Stop)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            Stop = true;
        });

        /*
        Value maxValue = -VALUE_INFINITE;
        StateInfo si;
        for (int i = 0; i < rootMoves.size(); ++i) {
            Move move = rootMoves[i].pv[0];           // 合法手のi番目. スライド4枚目参照
            pos.do_move(move, si);                    // 局面を1手進める
            Value value = (-1) * Eval::evaluate(pos); // 評価関数を呼び出す
            pos.undo_move(move);                      // 局面を1手戻す

            if (value > maxValue) {
                maxValue = value;
                bestMove = move;
            }
        }
        */

        // 開始4手までは乱数を加える.
        if(pos.game_ply() < 4) {
            size_t i = rand() % rootMoves.size();
            bestMove = rootMoves[i].pv[0];
            goto END;
        }
        
        while(!Stop) {

            Nodes = 0;
            Value alpha = -VALUE_INFINITE;
            Value beta = VALUE_INFINITE;

            StateInfo si;

            for(int i=0; i<rootMoves.size(); ++i) {                
                
                BetaCut = VALUE_ZERO;

                Move move = rootMoves[i].pv[0];
                pos.do_move(move, si);

                Value value = -1 * search(pos, -beta, -alpha, rootDepth - 1, 0);
                ++Nodes;

                pos.undo_move(move);

                if(Stop) {
                    break;
                }

                if(value > alpha) {
                    alpha = value;
                }

                rootMoves[i].score = value;
                rootMoves[i].BetaCut = BetaCut;

            }

            // 合法手を評価値の高い順に並び替える.
            std::stable_sort(rootMoves.begin(), rootMoves.end(), [](const RootMove &a, const RootMove &b) {
                return a.BetaCut > b.BetaCut;
            });
            
            // デバッグ用
            
            for(int i=0; i<rootMoves.size(); ++i) {
                std::cout << rootMoves[i].pv[0] << " " << rootMoves[i].score << " " << rootMoves[i].BetaCut << std::endl;
                if(i == rootMoves.size()-1) {
                    std::cout << std::endl;
                }
            }
            

            rootDepth += 2;
        }

        bestMove = rootMoves[0].pv[0];

        // タイマースレッド終了
        Stop = true;
        if (timerThread != nullptr) {
            timerThread->join();
            delete timerThread;
        }
    }
    /* 探索部ここまで */

END:;
    std::cout << "bestmove " << bestMove << std::endl;
    // デバッグ用
    if(rootMoves.size() > 0) {
        std::cout << "info nodes " << Nodes << std::endl;
        std::cout << "info depth " << rootDepth << std::endl;
        std::cout << "info score cp " << rootMoves[0].score << std::endl;
    }
}

// nega-max
Value search(Position &pos, int depth, int ply_from_root) {
    if (depth <= 0) {
        return Eval::evaluate(pos);
    }

    Value maxValue = -VALUE_INFINITE;
    StateInfo si;
    int moveCount = 0;

    // 千日手の検出
    RepetitionState draw_type = pos.is_repetition();
    if (draw_type != REPETITION_NONE)
        return draw_value(draw_type, pos.side_to_move());

    for(ExtMove m : MoveList<LEGAL>(pos)) {
        pos.do_move(m.move, si);
        ++moveCount;

        Value value = -1 * search(pos, depth - 1, ply_from_root + 1);
        Search::Nodes++;

        if(Search::Stop)
            return VALUE_ZERO;

        pos.undo_move(m.move);

        if(maxValue < value) {
            maxValue = value;
        }
    }

    if(moveCount == 0) {
        return mated_in(ply_from_root);
    }

    return maxValue;
}

// nega-max with alpha-beta pruning
Value search(Position &pos, Value alpha, Value beta, int depth, int ply_from_root) {
    if (depth <= 0) {
        return Eval::evaluate(pos);
    }

    StateInfo si;
    int moveCount = 0;

    // 千日手の検出
    RepetitionState draw_type = pos.is_repetition();
    if (draw_type != REPETITION_NONE)
        return draw_value(draw_type, pos.side_to_move());

    for(ExtMove m : MoveList<LEGAL>(pos)) {
        pos.do_move(m.move, si);
        ++moveCount;

        Value value = -1 * search(pos, -beta, -alpha, depth - 1, ply_from_root + 1);
        Search::Nodes++;

        if(Search::Stop)
            return VALUE_ZERO;

        pos.undo_move(m.move);

        // alpha値の更新
        if(value > alpha) {
            alpha = value;
            
            // beta枝刈り
            if(alpha >= beta) {
                Search::BetaCut++;
                break;
            }
        }
    }

    if(moveCount == 0) {
        return mated_in(ply_from_root);
    }

    return alpha;
}