int level_26() {
    const char* base_map =
        "................"
        ".          ... ."
        ".          ....."
        ".          #...."
        ".          #   ."
        ".    O     # * ."
        ". B<           ."
        ".  R       #...."
        ".  ^       #...."
        ".  ^0      #...."
        ".  .########.. ."
        ".  ...  ........"
        "~~~~~~~~~~~~~~~~";

    using St = State<13, 16, 1, 2, 0, 1>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
