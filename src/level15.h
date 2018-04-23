int level_15() {
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

    using St = State<13, 15, 0, 2, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
