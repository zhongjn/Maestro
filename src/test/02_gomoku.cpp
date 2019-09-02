#include "test.h"
#include "../game/game_gomoku.h"

using namespace Maestro;

TEST_CASE(gomoku_simple1) {
    Gomoku gomoku;
    using M = Move<Gomoku>;

    cout << "hash0: " << gomoku.get_hash() << endl;
    gomoku.move(M{ 0,0 });
    expect(gomoku.black.get(0, 0), "black");


    cout << "hash1: " << gomoku.get_hash() << endl;
    gomoku.move(M{ 5,0 });
    expect(gomoku.white.get(5, 0), "white");

    cout << "hash2: " << gomoku.get_hash() << endl;
    gomoku.move(M{ 1,1 });
    expect(gomoku.black.get(1, 1), "black");

    cout << "hash3: " << gomoku.get_hash() << endl;
    gomoku.move(M{ 5,1 });
    expect(gomoku.white.get(5, 1), "white");

    expect(gomoku.get_color() == Color::A, "color");
    Status status1 = gomoku.get_status();
    expect(!status1.end, "should not end");

    gomoku.move(M{ 2,2 }); // black
    gomoku.move(M{ 5,2 }); // white
    gomoku.move(M{ 3,3 }); // black
    gomoku.move(M{ 5,3 }); // white
    gomoku.move(M{ 4,4 }); // black

    Status status2 = gomoku.get_status();
    expect(status2.end, "should end");
    expect(status2.winner == Color::A, "A(black) should win");

    cout << "hashN: " << gomoku.get_hash() << endl;
}

TEST_CASE(gomoku_simple2) {
    Gomoku gomoku;
    using M = Move<Gomoku>;

    gomoku.move(M{ 14,0 }); // black
    gomoku.move(M{ 5,0 }); // white
    gomoku.move(M{ 13,1 }); // black
    gomoku.move(M{ 5,1 }); // white

    expect(gomoku.get_color() == Color::A, "color");
    Status status1 = gomoku.get_status();
    expect(!status1.end, "should not end");

    gomoku.move(M{ 12,2 }); // black
    gomoku.move(M{ 5,2 }); // white
    gomoku.move(M{ 11,3 }); // black
    gomoku.move(M{ 5,3 }); // white
    gomoku.move(M{ 10,4 }); // black

    Status status2 = gomoku.get_status();
    expect(status2.end, "should end");
    expect(status2.winner == Color::A, "A(black) should win");
}

TEST_CASE(gomoku_could_transfer) {
    Gomoku g1;
    g1.black.set(2, 2, true);
    g1.black.set(4, 3, true);

    Gomoku g2 = g1;
    g2.black.set(5, 6, true);
    expect(g1.could_transfer_to(g1), "could transfer to self");
    expect(g2.could_transfer_to(g2), "could transfer to self");

    expect(g1.could_transfer_to(g2), "could transfer");
    expect(!g2.could_transfer_to(g1), "could not transfer");
}