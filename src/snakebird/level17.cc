#include "main.h"

int main() {
    const char* base_map =
        "............."
        ".           ."
        ".           ."
        ".     .~    ."
        ".     .~    ."
        ".      ~G<< ."
        ".       >R^ ."
        ". ..~  ~..^ ."
        ". ..~~ ~..  ."
        ".  .~  ~.   ."
        ".      *    ."
        ".      ~    ."
        ".     .~    ."
        ".     .~    ."
        ".    ~~~    ."
        "~~~~~~~~~~~~~";

    using St = State<Setup<16, 13, 0, 2, 5>>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    EXPECT_EQ(68, search(st, map));
    return 0;
}
