int level_21() {
    const char* base_map =
        "............."
        ".           ."
        ".     *     ."
        ".     #     ."
        ".   O . O   ."
        ".           ."
        ".           ."
        ".    >B     ."
        ". ###^.  ## ."
        ". ...^O ##. ."
        ". .. ^<  .. ."
        ". ..     .. ."
        ". ..     .. ."
        "~~~~~~~~~~~~~";

    using St = State<14, 13, 3, 1, 9, 0>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
