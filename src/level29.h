bool level_29() {
    const char* base_map =
        ".............."
        ".      *     ."
        ".            ."
        ".            ."
        ".            ."
        ".            ."
        ".            ."
        ".   R<<      ."
        ".   00       ."
        ".   00       ."
        ". 112233     ."
        ". 112233 B<< ."
        ". .....  ..  ."
        ". .......... ."
        ". ..... .... ."
        "~~~~~~~~~~~~~~";

    using St = State<16, 14, 0, 2, 0, 4>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
