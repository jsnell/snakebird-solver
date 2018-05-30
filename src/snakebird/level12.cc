#include "main.h"

int main() {
    const char* base_map =
        "..............."
        ".    .        ."
        ".    .        ."
        ".    ~        ."
        ".   v B       ."
        ".   >>^       ."
        ".    .        ."
        ".         ~   ."
        ".           * ."
        ". ~.~  ~~~.   ."
        ".        ..   ."
        ".             ."
        ". O   ~       ."
        ".    .~O      ."
        ".    ...      ."
        ".    ...      ."
        "~~~~~~~~~~~~~~~";

    using St = State<Setup<17, 15, 2, 1, 7>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(52, search(st, map));
    return 0;
}
