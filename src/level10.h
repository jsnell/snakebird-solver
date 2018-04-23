int level_10() {
    const char* base_map =
        ".............."
        ".  ...       ."
        ". .... *     ."
        ".    .       ."
        ".  O .   v.. ."
        ".      R<<.  ."
        ".   .... ..  ."
        ".    ... .   ."
        ".      . O   ."
        ".      . ..  ."
        ".      . ..  ."
        ".     .. ..  ."
        ".     ....   ."
        "~~~~~~~~~~~~~~";

    using St = State<14, 14, 2, 1, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
