#include "main.h"

int main() {
    const char* base_map =
        "..............."
        ".             ."
        ".             ."
        ".      .      ."
        ".      .      ."
        ".      .    * ."
        ".             ."
        ".   .     .   ."
        ".  >>R        ."
        ". .^ v        ."
        ". .G<<        ."
        ". ....        ."
        "~~~~~~~~~~~~~~~";

    using St = State<Setup<13, 15, 0, 2, 4>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(34, search(st, map));
    return 0;
}
