bool level_37() {
    const char* base_map =
        ".............."
        ".       .    ."
        ".       .    ."
        ". .....      ."
        ". .   .      ."
        ".   T   .    ."
        ".   .   .    ."
        ".   .G<<T  * ."
        ".   .>>R.    ."
        ".   .....    ."
        "~~~~~~~~~~~~~~";

    using St = State<11, 14, 0, 2, 0, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
