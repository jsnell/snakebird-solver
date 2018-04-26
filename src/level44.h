int level_44() {
    const char* base_map =
        ".............."
        ".            ."
        ".    T       ."
        ".      O   * ."
        ".  #O        ."
        ".    #       ."
        ".   >R T     ."
        ".   G<       ."
        ".  #....  #. ."
        ".   .. .   . ."
        ".    . .   . ."
        "~~~~~~~~~~~~~~";

    using St = State<12, 14, 2, 2, 2, 0, 1>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
