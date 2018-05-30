#include "main.h"

int main() {
    const char* base_map =
        ".............."
        ".      *     ."
        ".            ."
        ".            ."
        ".            ."
        ".            ."
        ".            ."
        ".   R<<      ."
        ".   00       ."
        ".   00       ."
        ". 112233     ."
        ". 112233 B<< ."
        ". .....  ..  ."
        ". .......... ."
        ". ..... .... ."
        "~~~~~~~~~~~~~~";

    using St = State<Setup<16, 14, 0, 2, 3, 4>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(45, search(st, map));
    return 0;
}
