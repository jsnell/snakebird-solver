#include "main.h"

int main() {
    const char* base_map =
        "...................."
        ".                  ."
        ".     *        .   ."
        ".              .   ."
        ".                  ."
        ".  >B    G<   .    ."
        ".  ^O    O^   ..   ."
        ".     .       ..   ."
        ".     .       .. . ."
        ".     .       .. . ."
        "~~~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<11, 20, 2, 2, 5, 0>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(34, search(st, map));
    return 0;
}
