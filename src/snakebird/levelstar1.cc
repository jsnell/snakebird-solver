#include "main.h"

int main() {
    const char* base_map =
        "........................."
        ".                       ."
        ".                       ."
        ".                       ."
        ".       .#####          ."
        ".       .               ."
        ".       .               ."
        ".       . ####          ."
        ". *          #          ."
        ".                       ."
        ".              B<< .    ."
        "....            0^      ."
        "....                    ."
        "....           G<<  .   ."
        "...            R<<<     ."
        ".               0       ."
        ".              .....    ."
        ".              .....    ."
        ".               ...     ."
        "~~~~~~~~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<20, 25, 0, 3, 4, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(75, search(st, map));
    return 0;
}
