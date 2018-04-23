int level_30() {
    const char* base_map =
        "..............."
        ".             ."
        ".    *        ."
        ".             ."
        ".    .    .T. ."
        ".   T         ."
        ".  B<<<   .   ."
        ". .. .    .   ."
        ".  . .    .   ."
        ".  . .    .   ."
        "~~~~~~~~~~~~~~~";

    using St = State<11, 15, 0, 1, 0, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
