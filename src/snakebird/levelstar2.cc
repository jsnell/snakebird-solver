#include "main.h"

int main() {
    const char* base_map =
        "..................."
        ".     ...         ."
        ".   .......       ."
        ".   . O O ..      ."
        ".  ..O.O.O.. ...  ."
        ".  .OOOOOOO...... ."
        ". .. .O.O. R<< *. ."
        ". ..OOOOOOO.....  ."
        ". ...O.O.O....    ."
        ".   . O O .       ."
        ".    ......       ."
        ".    ......       ."
        ".    ...          ."
        "~~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<14, 19, 26, 1, 29, 0>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(60, search(st, map));
    return 0;
}
