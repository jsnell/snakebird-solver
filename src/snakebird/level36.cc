#include "main.h"

int main() {
    const char* base_map =
        "...................."
        ".                  ."
        ".  ....        ... ."
        ".  ....        ... ."
        ".   ..             ."
        ". .             *  ."
        ". .                ."
        ". . >>R   #    ... ."
        ". T>^G<<     ###   ."
        ". ......##T  #     ."
        ".   ..   ###.#     ."
        ".           .      ."
        "~~~~~~~~~~~~~~~~~~~~";

    using St = State<Setup<13, 20, 0, 2, 6, 0, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(29, search(st, map));
    return 0;
}
