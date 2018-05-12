int level_05() {
    const char* base_map =
        "........"     
        ".   *  ."
        ". .    ."
        ".O  O. ."
        ".      ."
        ".~.  . ."
        ". >R~~ ."
        ". ^.~  ."
        "~~~~~~~~";        

    using St = State<Setup<9, 8, 2, 1, 5>>;
    St::Map map(base_map);
    St st(map);

    st.print(map);

    return search(st, map);
}
