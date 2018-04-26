int level_31() {
    const char* base_map =
        "................."
        ".               ."
        ".      ....     ."
        ".       ..      ."
        ".               ."
        ".          T    ."
        ".          .    ."
        ".        G .    ."
        ". *   T  ^      ."
        ".     .  ^<     ."
        ".     ......    ."
        ".       ..      ."
        "~~~~~~~~~~~~~~~~~";

    using St = State<13, 17, 0, 1, 4, 0, 1>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
