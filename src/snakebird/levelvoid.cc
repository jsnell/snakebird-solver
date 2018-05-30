#include "main.h"

int main() {
    const char* base_map =
        "................."
        ".    ....       ."
        ".     ....      ."
        ".     .O.  O    ."
        ".     .O.       ."
        ".   0           ."
        ".   B<          ."
        ".  >GR<         ."
        ".  ....#        ."
        ".  ...          ."
        ".   ...      *  ."
        ".    .          ."
        ".   .      #    ."
        ".          .    ."
        ".          .    ."
        ".          .    ."
        ".          .    ."
        ".          .    ."
        "~~~~~~~~~~~~~~~~~";

    using St = State<Setup<19, 17, 3, 3, 5, 1>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(52, search(st, map));
    return 0;
}
