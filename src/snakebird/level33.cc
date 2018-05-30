#include "main.h"

int main() {
    const char* base_map =
        "..............."
        ".             ."
        ".    *        ."
        ".             ."
        ".    ###..... ."
        ".    O   T. . ."
        ".    .##  O . ."
        ".        .  . ."
        ".    .      . ."
        ". #   T     . ."
        ".        G< . ."
        ".   ##.#..^ . ."
        ".   ... ..  . ."
        "~~~~~~~~~~~~~~~";

    using St = State<Setup<14, 15, 2, 1, 5, 0, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(42, search(st, map));
    return 0;
}
