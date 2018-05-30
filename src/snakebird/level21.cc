#include "main.h"

int main() {
    const char* base_map =
        "............."
        ".           ."
        ".     *     ."
        ".     #     ."
        ".   O . O   ."
        ".           ."
        ".           ."
        ".    >B     ."
        ". ###^.  ## ."
        ". ...^O ##. ."
        ". .. ^<  .. ."
        ". ..     .. ."
        ". ..     .. ."
        "~~~~~~~~~~~~~";

    using St = State<Setup<14, 13, 3, 1, 9, 0>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(39, search(st, map));
    return 0;
}
