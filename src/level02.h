int level_02() {
    const char* base_map =
        "..........."
        "....      ."
        "....      ."
        "....  *   ."
        ". O.   v  ."
        ".     G< O."
        ".  .....  ."
        "........  ."
        "........  ."
        "........  ."
        "........  ."
        "~~~~~~~~~~~";

    using St = State<12, 11, 2, 1, 5>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
