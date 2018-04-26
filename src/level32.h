int level_32() {
    const char* base_map =
        ".............."
        ".            ."
        ".         .  ."
        ".    .    .  ."
        ".  * .   0T  ."
        ".    .B< ... ."
        ".    T>R . . ."
        ". ......##   ."
        ".  .  .      ."
        "~~~~~~~~~~~~~~";

    using St = State<10, 14, 0, 2, 2, 1, 1>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
