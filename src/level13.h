int level_13() {
    const char* base_map =
        "................."
        ".               ."
        ".               ."
        ".     ~~~~      ."
        ".     ...~      ."
        ".     ...~   .  ."
        ".            .  ."
        ".   *       B<< ."
        ".            .^ ."
        ".            .  ."
        ". ...       G<< ."
        ".  ...       .^ ."
        ".            .^ ."
        ".            .  ."
        ".            .  ."
        "~~~~~~~~~~~~~~~~~";

    using St = State<16, 17, 0, 2, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
