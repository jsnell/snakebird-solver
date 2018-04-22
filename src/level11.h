bool level_11() {
    const char* base_map =
        ".............."
        ".       ~~   ."
        ".    ~~~.~~  ."
        ".   ~......~ ."
        ".   ..  .... ."
        ".   .. O O . ."
        ".    .O.O O. ."
        ". * >G O  .. ."
        ".   ^... O.. ."
        ".   ^<<....  ."
        ".     ^ ..   ."
        ".     ^      ."
        "~~~~~~~~~~~~~~";

    using St = State<13, 14, 7, 1, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
