bool level_00() {
    const char* base_map =
        "........................"
        ".        ..     *      ."
        ".        ....          ."
        ".         ..           ."
        ".         .   O .      ."
        ".   >>R   . .   ..   O ."
        ".........   ..       ..."
        "........................";


    using St = State<8, 24, 2, 1, 5>;
    St::Map map(base_map);
    St st(map);
    st.print(map);

    return search(st, map);
}
