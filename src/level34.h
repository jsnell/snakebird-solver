int level_34() {
    const char* base_map =
        "..........."
        ".         ."
        ".   ..T.. ."
        ". * .. .. ."
        ".      0  ."
        ".  .   >B ."
        ".   T  G< ."
        ".   ..... ."
        "~~~~~~~~~~~";

    using St = State<9, 11, 0, 2, 0, 1>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
