#include "main.h"

int main() {
    const char* base_map =
        ".................."
        ".              * ."
        ".                ."
        ".  >>R        ...."
        ". >^G<<     ~~~..."
        ".......~~~  ~    ."
        ". .....  ~~.~    ."
        "~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<8, 18, 0, 2, 5>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(29, search(st, map));
    return 0;
}
