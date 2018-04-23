bool level_40() {
    const char* base_map =
        "......................"
        ".                    ."
        ".           *        ."
        ".                    ."
        ".                    ."
        ".    .               ."
        ".     00             ."
        ".    11              ."
        ".    .               ."
        ".    >v              ."
        ".    .>>B            ."
        ".   G<<<<            ."
        ".    .               ."
        ".    .               ."
        ".    .               ."
        "~~~~~~~~~~~~~~~~~~~~~~";

    using St = State<16, 22, 0, 2, 0, 2>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
