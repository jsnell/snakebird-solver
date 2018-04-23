int level_45() {
    const char* base_map =
        ".............."
        ".            ."
        ".      0     ."
        ".     #1     ."
        ".      1     ."
        ".    . .   * ."
        ".    #       ."
        ".    #       ."
        ". >>R  #B<<  ."
        ". ^.##O ..   ."
        ".  ..   ..   ."
        ".  .     .   ."
        ".     O      ."
        ".            ."
        ".            ."
        ".     .      ."
        ".    #.#     ."
        "~~~~~~~~~~~~~~";

    using St = State<18, 14, 2, 2, 6, 2>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
