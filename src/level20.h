bool level_20() {
    const char* base_map =
        "................."
        ".               ."
        ".           ..  ."
        ".          .... ."
        ".   >B     .O . ."
        ". * >G    O   . ."
        ".   ..   . .... ."
        ".   ..~      .. ."
        ".   ....  .  .. ."
        ".   ...   .  .. ."
        "~~~~~~~~~~~~~~~~~";

    using St = State<11, 17, 2, 2, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
