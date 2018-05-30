#include "main.h"

int main() {
    const char* base_map =
        "........................"
        ".        ..     *      ."
        ".        ....          ."
        ".         ..           ."
        ".         .   O .      ."
        ".   >>R   . .   ..   O ."
        ".........   ..       ..."
        "........................";


    using St = State<Setup<8, 24, 2, 1, 5>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(29, search(st, map));
    return 0;
}
