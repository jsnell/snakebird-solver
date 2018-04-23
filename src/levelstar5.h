int level_star5() {
    const char* base_map =
        ".............."
        ". .......... ."
        ".  0..... .  ."
        ".  0     *   ."
        ".  0 #   .   ."
        ".  . ..      ."
        ".  .T .      ."
        ".  .  .      ."
        ".  ..#.      ."
        ".    T       ."
        ".            ."
        ".   >>Rv     ."
        ".   ^B<<     ."
        ".   ....     ."
        ".    ..      ."
        "~~~~~~~~~~~~~~";

    using St = State<16, 14, 0, 2, 0, 1>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
