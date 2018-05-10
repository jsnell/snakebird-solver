int level_void() {
    const char* base_map =
        "................."
        ".    ....       ."
        ".     ....      ."
        ".     .O.  O    ."
        ".     .O.       ."
        ".   0           ."
        ".   B<          ."
        ".  >GR<         ."
        ".  ....#        ."
        ".  ...          ."
        ".   ...      *  ."
        ".    .          ."
        ".   .      #    ."
        ".          .    ."
        ".          .    ."
        ".          .    ."
        ".          .    ."
        ".          .    ."
        "~~~~~~~~~~~~~~~~~";

    using St = State<19, 17, 3, 3, 5, 1>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
